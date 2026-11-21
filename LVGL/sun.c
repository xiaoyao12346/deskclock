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

#ifndef LV_ATTRIBUTE_IMG_SUN
#define LV_ATTRIBUTE_IMG_SUN
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_SUN uint8_t sun_map[] = {
  0x00, 0x00, 0x00, 0x00, 	/*Color of index 0*/
  0xff, 0xff, 0xff, 0xb5, 	/*Color of index 1*/

  0x00, 0x00, 
  0x01, 0x80, 
  0x01, 0x80, 
  0x10, 0x08, 
  0x0b, 0xd0, 
  0x07, 0xe0, 
  0x0f, 0xf0, 
  0x6f, 0xf6, 
  0x6f, 0xf6, 
  0x0f, 0xf0, 
  0x07, 0xe0, 
  0x0b, 0xd0, 
  0x10, 0x08, 
  0x01, 0x80, 
  0x01, 0x80, 
  0x00, 0x00, 
};

const lv_img_dsc_t sun = {
  .header.cf = LV_IMG_CF_INDEXED_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 16,
  .header.h = 16,
  .data_size = 40,
  .data = sun_map,
};
