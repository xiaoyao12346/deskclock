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

#ifndef LV_ATTRIBUTE_IMG_THUNDERSHOWER
#define LV_ATTRIBUTE_IMG_THUNDERSHOWER
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_THUNDERSHOWER uint8_t thundershower_map[] = {
  0x00, 0x00, 0x00, 0x00, 	/*Color of index 0*/
  0xff, 0xff, 0xff, 0x9d, 	/*Color of index 1*/

  0x01, 0xf0, 
  0x0f, 0x98, 
  0x1e, 0x0c, 
  0x30, 0x04, 
  0x20, 0x04, 
  0x60, 0x07, 
  0xc0, 0x03, 
  0x80, 0x01, 
  0xc0, 0x01, 
  0xc0, 0x07, 
  0x7f, 0xfe, 
  0x03, 0xc0, 
  0x07, 0xe0, 
  0x07, 0xc0, 
  0x07, 0x00, 
  0x04, 0x00, 
};

const lv_img_dsc_t thundershower = {
  .header.cf = LV_IMG_CF_INDEXED_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 16,
  .header.h = 16,
  .data_size = 40,
  .data = thundershower_map,
};
