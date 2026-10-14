#include <string.h>
#include "lvgl.h"
#include "vars.h"
#include "screens.h"

static char s_cNavTime[16]     = "--:--";
static char s_cTempVal[16]     = "--.-";
static char s_cTempDesc[16]    = "--";
static char s_cHumiVal[16]     = "--";
static char s_cHumiDesc[16]    = "--";
static char s_cWeatherText[24] = "--";
static char s_cNetTime[24]     = "--:--:--";
static char s_cNetSync[16]     = "未同步";

static void prv_vUpdateLabelIfChanged(char *p_buf, size_t x_max, const char *p_val, lv_obj_t *p_lbl)
{
    if ((p_val == NULL) || (strncmp(p_buf, p_val, x_max - 1U) == 0)) {
        return;
    }
    strncpy(p_buf, p_val, x_max - 1U);
    p_buf[x_max - 1U] = '\0';
    if (p_lbl != NULL) {
        lv_label_set_text(p_lbl, p_buf);
    }
}

const char *get_var_home_nav_time(void)     { return s_cNavTime; }
const char *get_var_home_temp_val(void)     { return s_cTempVal; }
const char *get_var_home_temp_desc(void)    { return s_cTempDesc; }
const char *get_var_home_humi_val(void)     { return s_cHumiVal; }
const char *get_var_home_humi_desc(void)    { return s_cHumiDesc; }
const char *get_var_home_weather_text(void) { return s_cWeatherText; }
const char *get_var_home_net_time(void)     { return s_cNetTime; }
const char *get_var_home_net_sync(void)     { return s_cNetSync; }

void set_var_home_nav_time(const char *value)     { prv_vUpdateLabelIfChanged(s_cNavTime, sizeof(s_cNavTime), value, objects.nav_time); }
void set_var_home_temp_val(const char *value)     { prv_vUpdateLabelIfChanged(s_cTempVal, sizeof(s_cTempVal), value, objects.temp_val); }
void set_var_home_temp_desc(const char *value)    { prv_vUpdateLabelIfChanged(s_cTempDesc, sizeof(s_cTempDesc), value, objects.temp_badge); }
void set_var_home_humi_val(const char *value)     { prv_vUpdateLabelIfChanged(s_cHumiVal, sizeof(s_cHumiVal), value, objects.humi_val); }
void set_var_home_humi_desc(const char *value)    { prv_vUpdateLabelIfChanged(s_cHumiDesc, sizeof(s_cHumiDesc), value, objects.humi_badge); }
void set_var_home_weather_text(const char *value) { prv_vUpdateLabelIfChanged(s_cWeatherText, sizeof(s_cWeatherText), value, objects.weather_text); }
void set_var_home_net_time(const char *value)     { prv_vUpdateLabelIfChanged(s_cNetTime, sizeof(s_cNetTime), value, objects.env_net_time); }
void set_var_home_net_sync(const char *value)     { prv_vUpdateLabelIfChanged(s_cNetSync, sizeof(s_cNetSync), value, objects.net_sync_text); }