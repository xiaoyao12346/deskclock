#include "ssd1322.h"

/* ================= SSD1322 显示驱动（硬件 SPI1 + DMA 加速） =================
 * 接线（与原来的软件 SPI 完全一致，不用改硬件）：
 *   PA2 -> RES, PA3 -> DC, PA4 -> CS     普通 GPIO
 *   PA5 -> SCL  = SPI1_SCK  (AF5)
 *   PA7 -> SDA  = SPI1_MOSI (AF5)
 *
 * SPI1 挂在 APB2 = 100MHz，分频 /8 -> 12.5MHz（SSD1322 手册串行时钟约 10MHz，
 * 这里轻度超频；原来的软件 SPI 在 -O3 下每 bit 约 12 周期，等效已经接近 8MHz，
 * 所以分频必须给到 /8 才比软件 SPI 快）。
 * 若出现花屏/错位，把 OLED_SPI_BR 改成 3u（/16 = 6.25MHz）或 4u（/32 = 3.125MHz）。
 * 数据搬运用 DMA2 Stream3 Channel3（SPI1_TX），一次一整行（<=128 字节），
 * CPU 不用再逐位翻转 IO。
 * ========================================================================= */

#define OLED_SPI_BR   (2u)   /* 2 = /8 (12.5MHz)，3 = /16 (6.25MHz) */

static void OLED_SPI_Init(void);

/**
  * @brief  等待 SPI 把最后一个字节移出去
  */
static void SPI_Wait_Idle(void)
{
    while (!(SPI1->SR & SPI_SR_TXE)) { }   /* 发送缓冲已空 */
    while (SPI1->SR & SPI_SR_BSY) { }      /* 移位寄存器已空 */
}

/**
  * @brief  发送单字节（保留原函数名，内部已换成硬件 SPI1）
  */
void Soft_SPI_SwapByte(uint8_t byte)
{
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    *(__IO uint8_t *)&SPI1->DR = byte;
    SPI_Wait_Idle();
}

/**
  * @brief  初始化 SPI1 + DMA2
  * @note   PA5/PA7 配成 AF5；SPI 模式 0（CPOL=0/CPHA=0），SSD1322 上升沿采样
  */
static void OLED_SPI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* PA5=SCK, PA7=MOSI 交给 SPI1 */
    GPIO_InitStruct.Pin       = GPIO_PIN_5 | GPIO_PIN_7;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    __HAL_RCC_SPI1_FORCE_RESET();
    __HAL_RCC_SPI1_RELEASE_RESET();

    /* 主机、软件 NSS、8bit、模式0，SPE 使能 */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                (OLED_SPI_BR << SPI_CR1_BR_Pos);
    SPI1->CR2 = 0;                        /* 8bit，只发不收 */
    SPI1->CR1 |= SPI_CR1_SPE;

    /* DMA2 Stream3 Channel3: 内存 -> SPI1->DR，字节，单次，高优先级 */
    DMA2_Stream3->CR &= ~DMA_SxCR_EN;
    while (DMA2_Stream3->CR & DMA_SxCR_EN) { }
    DMA2->LIFCR = DMA_LIFCR_CFEIF3 | DMA_LIFCR_CDMEIF3 | DMA_LIFCR_CTEIF3 |
                  DMA_LIFCR_CHTIF3 | DMA_LIFCR_CTCIF3;

    DMA2_Stream3->PAR  = (uint32_t)&SPI1->DR;
    DMA2_Stream3->M0AR = 0;
    DMA2_Stream3->NDTR = 0;
    DMA2_Stream3->FCR  = 0;               /* 直接模式，不分 FIFO */
    DMA2_Stream3->CR   = (3u << DMA_SxCR_CHSEL_Pos) |   /* 通道3 = SPI1_TX */
                         DMA_SxCR_MINC |                /* 内存地址递增 */
                         DMA_SxCR_DIR_0 |               /* 内存 -> 外设 */
                         (2u << DMA_SxCR_PL_Pos);       /* 高优先级 */
}

/**
  * @brief  初始化 IO 口
  */
void OLED_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA2=RES, PA3=DC, PA4=CS：普通推挽输出 */
    GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    OLED_CS(1);
    OLED_RES(1);
    OLED_DC(1);

    OLED_SPI_Init();      /* 再把 PA5/PA7 切到 SPI1 复用 */
}

/**
  * @brief  向 OLED 写入数据
  */
void Oled_Write_data(uint8_t data)
{
    OLED_CS(0);
    OLED_DC(1);
    Soft_SPI_SwapByte(data);
    OLED_CS(1);
}

/**
  * @brief  向 OLED 写入命令
  */
void Oled_Write_cmd(uint8_t cmd)
{
    OLED_CS(0);
    OLED_DC(0);
    Soft_SPI_SwapByte(cmd);
    OLED_CS(1);
}

/**
  * @brief  SSD1322 初始化序列
  */
void OLED_Init(void)
{
    OLED_GPIO_Init();   // 初始化 GPIO / SPI / DMA

    // 硬件复位
    OLED_RES(0);        // reset low
    HAL_Delay(100);     // hold low 100ms
    OLED_RES(1);        // release reset
    HAL_Delay(100);     // wait for chip stable

    Oled_Write_cmd(0xFD); Oled_Write_data(0x12);   // unlock command

    Oled_Write_cmd(0xAE);                          // display off

    Oled_Write_cmd(0xB3); Oled_Write_data(0x91);   // clock divider / osc freq
    Oled_Write_cmd(0xCA); Oled_Write_data(0x3F);   // mux ratio = 64

    Oled_Write_cmd(0xA2); Oled_Write_data(0x00);   // display offset
    Oled_Write_cmd(0xA1); Oled_Write_data(0x00);   // start line

    Oled_Write_cmd(0xA0); Oled_Write_data(0x14);   // re-map + dual COM line
                          Oled_Write_data(0x11);

    Oled_Write_cmd(0xB5); Oled_Write_data(0x00);   // GPIO config
    Oled_Write_cmd(0xAB); Oled_Write_data(0x01);   // enable internal VDD (关键点：开启内部电源)

    Oled_Write_cmd(0xB4); Oled_Write_data(0xA0);   // display enhancement A
                          Oled_Write_data(0xFD);

    Oled_Write_cmd(0xC1); Oled_Write_data(0x9F);   // contrast current
    Oled_Write_cmd(0xC7); Oled_Write_data(0x0F);   // master contrast current

    Oled_Write_cmd(0xB1); Oled_Write_data(0xE2);   // phase length
    Oled_Write_cmd(0xD1); Oled_Write_data(0x82);   // display enhancement B
                          Oled_Write_data(0x20);

    Oled_Write_cmd(0xBB); Oled_Write_data(0x1F);   // pre-charge voltage
    Oled_Write_cmd(0xB6); Oled_Write_data(0x08);   // 2nd pre-charge period
    Oled_Write_cmd(0xBE); Oled_Write_data(0x07);   // VCOMH voltage

    Oled_Write_cmd(0xA6);                          // normal display

    Oled_Write_cmd(0xAF);                          // display on
    HAL_Delay(100);
}

void Oled_Init(void)
{
    OLED_Init();
}

/**
  * @brief  设置 SSD1322 绘图窗口
  * @note   SSD1322 列地址以 4 个像素（2 字节）为 1 个单位，因此 x 必须 4 像素对齐
  */
void OLED_Set_Window(uint16_t x1, uint8_t y1, uint16_t x2, uint8_t y2)
{
    // 4 像素对齐：x1 向下取整，x2 向上取整
    x1 = x1 & ~3;
    x2 = x2 | 3;

    uint8_t start_col = 0x1C + (x1 / 4); // 0x1C 为 256x64 屏幕的列偏移基准
    uint8_t end_col   = 0x1C + (x2 / 4);

    Oled_Write_cmd(0x15);            // Set Column Address
    Oled_Write_data(start_col);
    Oled_Write_data(end_col);

    Oled_Write_cmd(0x75);            // Set Row Address
    Oled_Write_data(y1);
    Oled_Write_data(y2);

    Oled_Write_cmd(0x5C);            // Write RAM
}

/**
  * @brief  通过 DMA 连续写入数据
  * @param  buf 待发送缓冲区（调用前 CS 已拉低、DC=1）
  * @param  len 字节数，单次最多 65535
  * @note   阻塞等待本次搬运完成；返回时最后一个字节已移出，可安全拉高 CS
  */
void OLED_Write_Buffer(uint8_t *buf, uint32_t len)
{
    if (len == 0 || len > 0xFFFF) return;

    DMA2_Stream3->CR &= ~DMA_SxCR_EN;
    while (DMA2_Stream3->CR & DMA_SxCR_EN) { }
    DMA2->LIFCR = DMA_LIFCR_CFEIF3 | DMA_LIFCR_CDMEIF3 | DMA_LIFCR_CTEIF3 |
                  DMA_LIFCR_CHTIF3 | DMA_LIFCR_CTCIF3;

    DMA2_Stream3->M0AR = (uint32_t)buf;
    DMA2_Stream3->NDTR = len;
    SPI1->CR2 |= SPI_CR2_TXDMAEN;
    DMA2_Stream3->CR |= DMA_SxCR_EN;

    /* 等 DMA 搬完；加 50ms 超时保护，避免 SPI/DMA 配置异常时整块屏幕卡死 */
    {
        uint32_t t0 = HAL_GetTick();
        while (!(DMA2->LISR & DMA_LISR_TCIF3)) {
            if ((HAL_GetTick() - t0) > 50u) break;
        }
    }

    DMA2_Stream3->CR &= ~DMA_SxCR_EN;
    while (DMA2_Stream3->CR & DMA_SxCR_EN) { }
    SPI1->CR2 &= ~SPI_CR2_TXDMAEN;

    DMA2->LIFCR = DMA_LIFCR_CFEIF3 | DMA_LIFCR_CDMEIF3 | DMA_LIFCR_CTEIF3 |
                  DMA_LIFCR_CHTIF3 | DMA_LIFCR_CTCIF3;

    SPI_Wait_Idle();
}
