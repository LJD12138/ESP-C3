#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_NONE
};

// Native global variables

extern const char *get_var_home_nav_time();
extern void set_var_home_nav_time(const char *value);
extern const char *get_var_home_temp_val();
extern void set_var_home_temp_val(const char *value);
extern const char *get_var_home_temp_desc();
extern void set_var_home_temp_desc(const char *value);
extern const char *get_var_home_humi_val();
extern void set_var_home_humi_val(const char *value);
extern const char *get_var_home_humi_desc();
extern void set_var_home_humi_desc(const char *value);
extern const char *get_var_home_weather_text();
extern void set_var_home_weather_text(const char *value);
extern const char *get_var_home_net_time();
extern void set_var_home_net_time(const char *value);
extern const char *get_var_home_net_sync();
extern void set_var_home_net_sync(const char *value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/