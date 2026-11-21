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

#ifndef LV_ATTRIBUTE_IMG_WARNING
#define LV_ATTRIBUTE_IMG_WARNING
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_WARNING uint8_t Warning_map[] = {
   0x00, 0x00, 0x00, 0x00, 	/*Color of index 0*/
  0xff, 0xff, 0xff, 0xd4, 	/*Color of index 1*/

  0x00, 0x00, 
  0x01, 0x80, 
  0x03, 0xc0, 
  0x07, 0xe0, 
  0x06, 0x60, 
  0x0e, 0x70, 
  0x0e, 0x70, 
  0x1e, 0x78, 
  0x1e, 0x78, 
  0x3e, 0x7c, 
  0x3f, 0xfc, 
  0x3e, 0x7c, 
  0x3e, 0x7c, 
  0x3f, 0xfc, 
  0x3f, 0xfc, 
  0x3f, 0xfc, 
};

const lv_img_dsc_t Warning = {
  .header.cf = LV_IMG_CF_INDEXED_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 16,
  .header.h = 16,
  .data_size = 40,
  .data = Warning_map,
};
