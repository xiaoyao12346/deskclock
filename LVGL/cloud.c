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
    #include    "lvgl.h"
#endif


#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_CLOUD
#define LV_ATTRIBUTE_IMG_CLOUD
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_CLOUD uint8_t cloud_map[] = {
  0x00, 0x00, 0x00, 0x00, 	/*Color of index 0*/
  0xff, 0xff, 0xff, 0x80, 	/*Color of index 1*/

  0x00, 0x00, 
  0x08, 0x80, 
  0x09, 0x00, 
  0x0f, 0x00, 
  0x7d, 0xb0, 
  0x10, 0xc0, 
  0x10, 0xc0, 
  0x5b, 0xf0, 
  0x6e, 0x18, 
  0x1e, 0x08, 
  0x30, 0x0c, 
  0x60, 0x02, 
  0x40, 0x02, 
  0x60, 0x02, 
  0x3f, 0xfc, 
  0x00, 0x00, 
};

const lv_img_dsc_t cloud = {
  .header.cf = LV_IMG_CF_INDEXED_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 16,
  .header.h = 16,
  .data_size = 40,
  .data = cloud_map,
};
