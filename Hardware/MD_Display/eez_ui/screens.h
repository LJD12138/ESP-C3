#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN_WORK = 1,
    SCREEN_ID_MAIN_BOOTING = 2,
    SCREEN_ID_MAIN_CLOSING = 3,
    SCREEN_ID_MAIN_ENG = 4,
    SCREEN_ID_MAIN_UPDATE = 5,
    _SCREEN_ID_LAST = 5
};

typedef struct _objects_t {
    lv_obj_t *main_work;
    lv_obj_t *main_booting;
    lv_obj_t *main_closing;
    lv_obj_t *main_eng;
    lv_obj_t *main_update;
    lv_obj_t *nav_bar;
    lv_obj_t *nav_time;
    lv_obj_t *nav_date;
    lv_obj_t *sig_bar_0;
    lv_obj_t *sig_bar_1;
    lv_obj_t *sig_bar_2;
    lv_obj_t *sig_bar_3;
    lv_obj_t *sig_bar_4;
    lv_obj_t *home_tileview;
    lv_obj_t *tile_env;
    lv_obj_t *card_temp;
    lv_obj_t *temp_accent;
    lv_obj_t *temp_icon;
    lv_obj_t *temp_title;
    lv_obj_t *temp_badge;
    lv_obj_t *temp_val;
    lv_obj_t *temp_unit;
    lv_obj_t *temp_range;
    lv_obj_t *card_humi;
    lv_obj_t *humi_accent;
    lv_obj_t *humi_icon;
    lv_obj_t *humi_title;
    lv_obj_t *humi_badge;
    lv_obj_t *humi_val;
    lv_obj_t *humi_unit;
    lv_obj_t *humi_range;
    lv_obj_t *card_weather;
    lv_obj_t *weather_anim_img;
    lv_obj_t *weather_text;
    lv_obj_t *env_net_time;
    lv_obj_t *net_sync_dot;
    lv_obj_t *net_sync_text;
    lv_obj_t *tile_grid;
    lv_obj_t *app_cell_0;
    lv_obj_t *app_box_0;
    lv_obj_t *app_ic_0;
    lv_obj_t *app_name_0;
    lv_obj_t *app_cell_1;
    lv_obj_t *app_box_1;
    lv_obj_t *app_ic_1;
    lv_obj_t *app_name_1;
    lv_obj_t *app_cell_2;
    lv_obj_t *app_box_2;
    lv_obj_t *app_ic_2;
    lv_obj_t *app_name_2;
    lv_obj_t *app_cell_3;
    lv_obj_t *app_box_3;
    lv_obj_t *app_ic_3;
    lv_obj_t *app_name_3;
    lv_obj_t *app_cell_4;
    lv_obj_t *app_box_4;
    lv_obj_t *app_ic_4;
    lv_obj_t *app_name_4;
    lv_obj_t *app_cell_5;
    lv_obj_t *app_box_5;
    lv_obj_t *app_ic_5;
    lv_obj_t *app_name_5;
    lv_obj_t *page_dot_0;
    lv_obj_t *page_dot_1;
    lv_obj_t *uc_booting_bar;
} objects_t;

extern objects_t objects;

void create_screen_main_work();
void tick_screen_main_work();

void create_screen_main_booting();
void tick_screen_main_booting();

void create_screen_main_closing();
void tick_screen_main_closing();

void create_screen_main_eng();
void tick_screen_main_eng();

void create_screen_main_update();
void tick_screen_main_update();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/