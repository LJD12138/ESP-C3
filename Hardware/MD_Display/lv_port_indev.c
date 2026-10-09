#include "lv_port_indev.h"
#include "../../main/board_config.h"

#if(boardDISPLAY)

/*********************
 *      INCLUDES
 *********************/
#include "MD_Display/md_display_iface.h"
#include <stdbool.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "lvgl.h"

#if(boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

/*********************
 *      DEFINES
 *********************/
#define TAG "LVGL_INDEV"

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void touchpad_read_cb(lv_indev_t * indev, lv_indev_data_t * data);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_indev_t *touch_indev = NULL;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化LVGL输入设备驱动（触摸屏）
 */
void lv_port_indev_init(void)
{
#if ESP_LCD_TOUCH_ENABLED
    /* 初始化触摸屏 */
    if (eDisp_TouchInit() != ESP_OK) {
        ESP_LOGE(TAG, "Touch initialization failed");
        return;
    }

    /* 创建触摸屏输入设备 */
    touch_indev = lv_indev_create();
    if (touch_indev == NULL) {
        ESP_LOGE(TAG, "Failed to create input device");
        return;
    }

    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, touchpad_read_cb);

    ESP_LOGI(TAG, "Touch input device initialized");
#else
    ESP_LOGW(TAG, "Touch support not enabled - esp_lcd_touch component not found");
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief 触摸屏读取回调函数
 */
static void touchpad_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
#if ESP_LCD_TOUCH_ENABLED
    /* 必须先从触摸控制器读取数据 */
    esp_lcd_touch_read_data(P_touch_handle);
    
    /* 获取触摸坐标 */
    esp_lcd_touch_point_data_t touch_data;
    uint8_t point_cnt = 0;
    
    esp_err_t ret = esp_lcd_touch_get_data(P_touch_handle, &touch_data, &point_cnt, 1);
    
    if (ret == ESP_OK && point_cnt > 0) {
        data->point.x = touch_data.x;
        data->point.y = touch_data.y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
#else
    data->state = LV_INDEV_STATE_RELEASED;
#endif
}

#endif /* boardDISPLAY */
