#ifndef EEZ_LVGL_UI_STYLES_H
#define EEZ_LVGL_UI_STYLES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Style: st_home_card
void add_style_st_home_card(lv_obj_t *obj);
void remove_style_st_home_card(lv_obj_t *obj);

// Style: st_home_cell
void add_style_st_home_cell(lv_obj_t *obj);
void remove_style_st_home_cell(lv_obj_t *obj);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_STYLES_H*/