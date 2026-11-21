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

#ifndef LV_ATTRIBUTE_IMG_OVERCAST
#define LV_ATTRIBUTE_IMG_OVERCAST
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_OVERCAST uint8_t overcast_map[] = {
   0x00, 0x00, 0x00, 0x00, 	/*Color of index 0*/
  0xff, 0xff, 0xff, 0xa3, 	/*Color of index 1*/

  0x00, 0x00, 
  0x00, 0x00, 
  0x03, 0x80, 
  0x07, 0xe0, 
  0x0e, 0x30, 
  0x0c, 0x78, 
  0x18, 0x7c, 
  0x38, 0x0c, 
  0x68, 0x06, 
  0x60, 0x06, 
  0x60, 0x06, 
  0x30, 0x1c, 
  0x1f, 0xf8, 
  0x00, 0x00, 
  0x00, 0x00, 
  0x00, 0x00, 
};

const lv_img_dsc_t overcast = {
  .header.cf = LV_IMG_CF_INDEXED_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 16,
  .header.h = 16,
  .data_size = 40,
  .data = overcast_map,
};
