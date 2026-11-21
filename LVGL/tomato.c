#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
    #include "lvgl.h"
#else
    #include  "lvgl.h"
#endif


#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_TOMATO
#define LV_ATTRIBUTE_IMG_TOMATO
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_TOMATO uint8_t tomato_map[] = {
  0x00, 0x00, 0x00, 0x00, 	/*Color of index 0*/
  0xff, 0xff, 0xff, 0xe4, 	/*Color of index 1*/

  0x00, 0x00, 
  0x01, 0x80, 
  0x1f, 0xe0, 
  0x1f, 0x78,   /* 右边补到第12列，叶子下沿更完整 */
  0x27, 0xc4,   /* 右侧第12列改成第13列，与左侧第2列对称 */
  0x40, 0x02,   /* 右侧收进第14列，与左侧第1列对称 */
  0x40, 0x02,   /* 去掉多余的第13列，只留第14列 */
  0x80, 0x01,   /* 右侧改为第15列，与左侧第0列对称 */
  0x80, 0x01,   /* 去掉第12列，右侧只留第15列 */
  0xc0, 0x03,   /* 右侧补上第15列，左右各两个像素 */
  0x40, 0x02,   /* 去掉第12列，右侧只留第14列 */
  0x40, 0x02,   /* 右侧第13列改为第14列，与左侧第1列对称 */
  0x20, 0x04,   /* 保持左右对称：第2列对第13列 */
  0x30, 0x0c,   /* 右侧第11列改为第13列，形成第12、13列 */
  0x0f, 0xf0,   /* 底部补到第11列，左右对称 */
  0x00, 0x00,
};

const lv_img_dsc_t tomato = {
  .header.cf = LV_IMG_CF_INDEXED_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 16,
  .header.h = 16,
  .data_size = 40,
  .data = tomato_map,
};
