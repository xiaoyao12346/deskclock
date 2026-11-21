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

#ifndef LV_ATTRIBUTE_IMG_CLOCK
#define LV_ATTRIBUTE_IMG_CLOCK
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_CLOCK uint8_t clock_map[] = {
 0x00, 0x00, 0x00, 0x00, 	/*Color of index 0*/
  0xff, 0xff, 0xff, 0xe4, 	/*Color of index 1*/

  0x00, 0x00, 
  0x39, 0x9c,   /* 原 0x39,0x98，右侧补上第13列 */
  0x3f, 0xfe,   /* 原 0x3f,0xf6，右侧补第12列、去掉第14列，左右对称 */
  0x78, 0x1e,   /* 原 0x58,0x1e，左侧补第2列，使左边连续 */
  0x30, 0x0c, 
  0x21, 0x84, 
  0x21, 0x84, 
  0x61, 0x86, 
  0x60, 0xc6, 
  0x20, 0x64, 
  0x20, 0x04, 
  0x30, 0x0c, 
  0x18, 0x18, 
  0x1f, 0xf8, 
  0x31, 0x88, 
  0x00, 0x00, 
};

const lv_img_dsc_t clock = {
  .header.cf = LV_IMG_CF_INDEXED_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 16,
  .header.h = 16,
  .data_size = 40,
  .data = clock_map,
};
