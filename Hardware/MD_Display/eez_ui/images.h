#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_ic_temp;
extern const lv_img_dsc_t img_ic_humi;
extern const lv_img_dsc_t img_ic_app_settings;
extern const lv_img_dsc_t img_ic_app_clock;
extern const lv_img_dsc_t img_ic_app_weather;
extern const lv_img_dsc_t img_ic_app_network;
extern const lv_img_dsc_t img_ic_app_about;
extern const lv_img_dsc_t img_ic_app_theme;
extern const lv_img_dsc_t img_anim_sunny_00;
extern const lv_img_dsc_t img_anim_sunny_01;
extern const lv_img_dsc_t img_anim_sunny_02;
extern const lv_img_dsc_t img_anim_sunny_03;
extern const lv_img_dsc_t img_anim_sunny_04;
extern const lv_img_dsc_t img_anim_sunny_05;
extern const lv_img_dsc_t img_anim_sunny_06;
extern const lv_img_dsc_t img_anim_sunny_07;
extern const lv_img_dsc_t img_anim_cloudy_00;
extern const lv_img_dsc_t img_anim_cloudy_01;
extern const lv_img_dsc_t img_anim_cloudy_02;
extern const lv_img_dsc_t img_anim_cloudy_03;
extern const lv_img_dsc_t img_anim_cloudy_04;
extern const lv_img_dsc_t img_anim_cloudy_05;
extern const lv_img_dsc_t img_anim_cloudy_06;
extern const lv_img_dsc_t img_anim_cloudy_07;
extern const lv_img_dsc_t img_anim_overcast_00;
extern const lv_img_dsc_t img_anim_overcast_01;
extern const lv_img_dsc_t img_anim_overcast_02;
extern const lv_img_dsc_t img_anim_overcast_03;
extern const lv_img_dsc_t img_anim_overcast_04;
extern const lv_img_dsc_t img_anim_overcast_05;
extern const lv_img_dsc_t img_anim_overcast_06;
extern const lv_img_dsc_t img_anim_overcast_07;
extern const lv_img_dsc_t img_anim_rain_00;
extern const lv_img_dsc_t img_anim_rain_01;
extern const lv_img_dsc_t img_anim_rain_02;
extern const lv_img_dsc_t img_anim_rain_03;
extern const lv_img_dsc_t img_anim_rain_04;
extern const lv_img_dsc_t img_anim_rain_05;
extern const lv_img_dsc_t img_anim_rain_06;
extern const lv_img_dsc_t img_anim_rain_07;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[40];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/