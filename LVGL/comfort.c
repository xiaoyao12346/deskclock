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

#ifndef LV_ATTRIBUTE_IMG_COMFORT
#define LV_ATTRIBUTE_IMG_COMFORT
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_COMFORT uint8_t comfort_map[] = {
  0x00, 0x00, 0x00, 0x00, 	/*Color of index 0*/
  0xff, 0xff, 0xff, 0xdd, 	/*Color of index 1*/

  0x0f, 0xe0, 
  0x1f, 0xf8, 
  0x3f, 0xfc, 
  0x7f, 0xfe, 
  0x7f, 0xfe, 
  0xf3, 0xcf, 
  0xf3, 0xcf, 
  0xff, 0xff, 
  0xff, 0xff, 
  0xff, 0xff, 
  0xe7, 0xe7, 
  0x79, 0x9e, 
  0x7e, 0x7e, 
  0x3f, 0xfc, 
  0x1f, 0xf8, 
  0x0f, 0xe0, 
};

const lv_img_dsc_t comfort = {
  .header.cf = LV_IMG_CF_INDEXED_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 16,
  .header.h = 16,
  .data_size = 40,
  .data = comfort_map,
};
