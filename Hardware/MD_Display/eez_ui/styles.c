#include "styles.h"
#include "images.h"
#include "fonts.h"

#include "ui.h"
#include "screens.h"

//
// Style: st_home_card
//

void add_style_st_home_card(lv_obj_t *obj) {
    (void)obj;
};

void remove_style_st_home_card(lv_obj_t *obj) {
    (void)obj;
};

//
// Style: st_home_cell
//

void add_style_st_home_cell(lv_obj_t *obj) {
    (void)obj;
};

void remove_style_st_home_cell(lv_obj_t *obj) {
    (void)obj;
};

//
//
//

void add_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*AddStyleFunc)(lv_obj_t *obj);
    static const AddStyleFunc add_style_funcs[] = {
        add_style_st_home_card,
        add_style_st_home_cell,
    };
    add_style_funcs[styleIndex](obj);
}

void remove_style(lv_obj_t *obj, int32_t styleIndex) {
    typedef void (*RemoveStyleFunc)(lv_obj_t *obj);
    static const RemoveStyleFunc remove_style_funcs[] = {
        remove_style_st_home_card,
        remove_style_st_home_cell,
    };
    remove_style_funcs[styleIndex](obj);
}