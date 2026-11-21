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

#ifndef LV_ATTRIBUTE_IMG_TIDE
#define LV_ATTRIBUTE_IMG_TIDE
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_TIDE uint8_t tide_map[] = {
  0x00, 0x00, 0x00, 0x00, 	/*Color of index 0*/
  0xff, 0xff, 0xff, 0xe4, 	/*Color of index 1*/

  0x01, 0x80, 
  0x01, 0xc0, 
  0x03, 0xe0, 
  0x07, 0xf0, 
  0x0f, 0xf0, 
  0x0f, 0xf8, 
  0x1f, 0xfc, 
  0x1f, 0xfc, 
  0x3f, 0xfe, 
  0x3f, 0xfe, 
  0x3f, 0xfe, 
  0x3f, 0xfc, 
  0x3f, 0xfc, 
  0x1f, 0xf8, 
  0x0f, 0xf8, 
  0x07, 0xe0, 
};

const lv_img_dsc_t tide = {
  .header.cf = LV_IMG_CF_INDEXED_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 16,
  .header.h = 16,
  .data_size = 40,
  .data = tide_map,
};
