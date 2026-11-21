/**
 * @file lv_port_disp.c
 * @brief LVGL Display Port for SSD1322 OLED (256x64, 4-bit Grayscale)
 */

#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "lv_port_disp.h" // 与工程目录里的 lv_port_disp.h 保持一致，不要删除
#include <stdbool.h>
#include "ssd1322.h"

/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES    256
#define MY_DISP_VER_RES    64

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_disp_drv_t disp_drv;

/* Framebuffer mirror (256x64, 4-bit): 2 pixels per byte (high nibble = even pixel).
 * Keeps the true value of the 4-pixel-boundary padding pixels so 1px-wide objects
 * (e.g. the separators) are not overwritten with 0x00 on partial redraws. */
static uint8_t fb_gray[MY_DISP_VER_RES][MY_DISP_HOR_RES / 2];

/* 一行打包后的显存数据（最多 128 字节），交给 DMA 一次搬完 */
static uint8_t flush_line[MY_DISP_HOR_RES / 2];

/* 刷屏耗时统计：用 DWT 周期计数器测 disp_flush 的时间。
 * 100MHz 下 lcd_flush_cyc / 100 就是微秒数；Keil 调试时把这两个变量加到 Watch 窗口即可。
 * 判断瓶颈：整屏刷新时它 × 片数 ≈ 每帧发送时间，剩下的就是 LVGL 渲染和主循环的时间。 */
volatile uint32_t lcd_flush_cyc;
volatile uint32_t lcd_flush_cyc_avg;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_port_disp_init(void)
{
    /* 1. 初始化屏幕硬件（这里会打开 SPI1 + DMA） */
    disp_init();

    /* 2. 创建 LVGL 绘图缓冲区 (给 10 行的像素，控制 SRAM 占用) */
    static lv_disp_draw_buf_t draw_buf_dsc;
    static lv_color_t buf_1[MY_DISP_HOR_RES * 10];
    lv_disp_draw_buf_init(&draw_buf_dsc, buf_1, NULL, MY_DISP_HOR_RES * 10);

    /* 3. 注册 LVGL 显示驱动 */
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf_dsc;

    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void disp_init(void)
{
    /* 打开 DWT 周期计数器，给 lcd_flush_cyc 计时用 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    OLED_Init(); // SSD1322 硬件初始化
}

/* Convert an LVGL RGB565 color to a 4-bit SSD1322 grayscale value (0..15).
 * The display is a dark-background OLED (0x00 = off, 0x0F = brightest), while
 * LVGL's default light theme draws light background + dark text, so the result
 * is inverted: a light LVGL color becomes an off pixel, a dark LVGL color
 * becomes a bright pixel.
 *
 * A plain linear inversion makes the anti-aliased edge pixels of thin font
 * strokes (e.g. 'l', 'e') land on dim gray levels that are barely visible on
 * the 4-bit panel, so those characters appear "half".  A squared curve on the
 * inverted luminance brightens the mid-tones (anti-aliasing) while forcing the
 * near-white background fully off. */
static uint8_t lv_color_to_oled_gray(lv_color_t c)
{
    uint16_t r = (c.full >> 11) & 0x1F;
    uint16_t g = (c.full >> 5)  & 0x3F;
    uint16_t b = (c.full)       & 0x1F;

    /* Expand 5/6/5-bit channels to 8-bit */
    r = (uint8_t)((r << 3) | (r >> 2));
    g = (uint8_t)((g << 2) | (g >> 4));
    b = (uint8_t)((b << 3) | (b >> 2));

    /* ITU-R BT.601 luminance, 0..255 (0 = black text, 255 = white background) */
    uint16_t lum = (uint16_t)((r * 77u + g * 150u + b * 29u) >> 8);

    /* Near-white background -> fully off (avoids a faint background glow) */
    if (lum >= 240) return 0x00;

    /* gray = 15 - 15*(lum/255)^2 : brightens anti-aliased mid-tones so thin
     * strokes stay visible.  (lum^2 * 15) >> 16 approximates /65025. */
    uint8_t shade = (uint8_t)(((uint32_t)lum * lum * 15u) >> 16);
    return (uint8_t)(0x0F - shade);
}

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    uint32_t cyc_start = DWT->CYCCNT;

    /* SSD1322 强制 4 像素边界对齐扩展 */
    int32_t x1 = area->x1 & ~3;
    int32_t x2 = area->x2 | 3;
    int32_t y1 = area->y1;
    int32_t y2 = area->y2;

    OLED_Set_Window(x1, y1, x2, y2);

    OLED_CS(0);
    OLED_DC(1);

    for (int32_t y = y1; y <= y2; y++) {
        uint32_t n = 0;

        for (int32_t x = x1; x <= x2; x += 2) {
            uint8_t gray1 = fb_gray[y][x >> 1] >> 4;
            uint8_t gray2 = fb_gray[y][x >> 1] & 0x0F;

            /* 这里 1 行内并不是整行都被 LVGL 重绘，只对落在区域内的像素取色，防止读越界 */
            if (x >= area->x1 && x <= area->x2) {
                gray1 = lv_color_to_oled_gray(*color_p);
                color_p++;
            }

            if ((x + 1) >= area->x1 && (x + 1) <= area->x2) {
                gray2 = lv_color_to_oled_gray(*color_p);
                color_p++;
            }

            uint8_t packed_byte = (uint8_t)((gray1 << 4) | gray2);
            fb_gray[y][x >> 1] = packed_byte;
            flush_line[n++] = packed_byte;   /* 先攒进行缓冲，再整行 DMA 出去 */
        }

        OLED_Write_Buffer(flush_line, n);    /* 硬件 SPI + DMA，阻塞到整行发完 */
    }

    OLED_CS(1);

    /* 统计本次刷屏耗时（周期数），8 次滑动平均 */
    lcd_flush_cyc = DWT->CYCCNT - cyc_start;
    lcd_flush_cyc_avg = (lcd_flush_cyc_avg == 0) ? lcd_flush_cyc
                                                 : (lcd_flush_cyc_avg * 7u + lcd_flush_cyc) / 8u;

    /* 通知 LVGL 刷屏结束 */
    lv_disp_flush_ready(disp_drv);
}

#else

typedef int keep_pedantic_happy;

#endif
