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