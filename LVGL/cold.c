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
    #include "lvgl.h"
#endif


#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_COLD 
#define LV_ATTRIBUTE_IMG_COLD 
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_COLD  uint8_t cold_map[] = {
 0x00, 0x00, 0x00, 0x00, 	/*Color of index 0*/
  0xff, 0xff, 0xff, 0xc0, 	/*Color of index 1*/

  0x00, 0x00, 
  0x11, 0x88, 
  0x23, 0xc4, 
  0x51, 0x8a, 
  0x09, 0x90, 
  0x05, 0xa0, 
  0x03, 0xc0, 
  0x3f, 0xfc,   /* 新增横线，从第2列到第13列 */
  0x01, 0x80,   /* 中心竖线保留 */
  0x03, 0xc0, 
  0x05, 0xa0, 
  0x09, 0x90, 
  0x51, 0x8a, 
  0x23, 0xc4, 
  0x11, 0x88, 
  0x00, 0x00, 
};

const lv_img_dsc_t cold  = {
  .header.cf = LV_IMG_CF_INDEXED_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 16,
  .header.h = 16,
  .data_size = 40,
  .data = cold_map,
};
