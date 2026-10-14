/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display
 * File    : lv_port_indev.c
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : LVGL输入设备（触摸屏）移植层实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "lv_port_indev.h"

#if (boardDISPLAY_EN && ESP_LCD_TOUCH_ENABLED)
#include <stdbool.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_lcd_touch_ft6x36.h"


#if (boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

//****************************************************Macros********************************************************************//
#define			TAG										"LVGL_INDEV"

//****************************************************Parameter Initialization**************************************************//
static esp_lcd_touch_handle_t t_lcd_touch_handle = NULL;
static lv_indev_t *touch_indev = NULL;

//****************************************************Function Declaration******************************************************//
static esp_err_t e_disp_touch_init(void);
static void      touchpad_read_cb(lv_indev_t *indev, lv_indev_data_t *data);


/***********************************************************************************************************************
 * 函数功能    : FT6336 触摸屏驱动初始化
 * 说明(备注)  : 创建触摸屏 I2C IO 句柄并初始化 FT6336 触摸芯片控制器
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : esp_err_t: ESP_OK 成功, 其他失败
 ************************************************************************************************************************/
static esp_err_t e_disp_touch_init(void)
{
	esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_FT6x36_CONFIG();
	esp_lcd_panel_io_handle_t tp_io_handle = NULL;

	esp_err_t ret = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)dispTOUCH_NUM,
	                                         &io_config, &tp_io_handle);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to create touch IO handle: %s", esp_err_to_name(ret));
		return ret;
	}

	esp_lcd_touch_config_t tp_cfg = {
		.x_max               = dispTFT_HEIGHT,  /* FT6336 原生 X 轴量程为 240 (swap_xy 交换后对应屏幕 Y) */
		.y_max               = dispTFT_WIDTH,   /* FT6336 原生 Y 轴量程为 320 (swap_xy 交换后对应屏幕 X) */
		.rst_gpio_num        = dispTOUCH_RST_PIN,
		.int_gpio_num        = dispTOUCH_INT_PIN,
		.levels = {
			.reset           = 0,
			.interrupt       = 0,
		},
		.flags = {
			.swap_xy         = 1,
			.mirror_x        = 1,
			.mirror_y        = 0,
		},
		.process_coordinates = NULL,
		.interrupt_callback  = NULL,
		.user_data           = NULL,
		.driver_data         = NULL,
	};

	ret = esp_lcd_touch_new_i2c_ft6x36(tp_io_handle, &tp_cfg, &t_lcd_touch_handle);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "FT6336 touch initialization failed: %s", esp_err_to_name(ret));
		return ret;
	}

	return ESP_OK;
}

/***********************************************************************************************************************
 * 函数功能    : 初始化LVGL输入设备驱动（触摸屏）
 * 说明(备注)  : 初始化FT6336芯片并注册LVGL指针输入设备
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void lv_port_indev_init(void)
{
	/* 初始化触摸屏 */
	if (e_disp_touch_init() != ESP_OK)
	{
		ESP_LOGE(TAG, "Touch initialization failed");
		return;
	}

	/* 创建触摸屏输入设备 */
	touch_indev = lv_indev_create();
	if (touch_indev == NULL)
	{
		ESP_LOGE(TAG, "Failed to create input device");
		return;
	}

	lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(touch_indev, touchpad_read_cb);

	/* 触摸采样周期 33ms(LV_DEF_REFR_PERIOD 默认) -> 16ms, 与显示任务快泵节拍对齐*/
	lv_timer_set_period(lv_indev_get_read_timer(touch_indev), dispINDEV_POLL_PERIOD_MS);
}

/***********************************************************************************************************************
 * 函数功能    : 触摸屏读取回调函数
 * 说明(备注)  : 获取FT6336当前按下状态与坐标并传递给LVGL
 * 传入参数    : indev: 输入设备句柄, data: 输入事件数据结构体指针
 * 输出参数    : data: 包含坐标与按压状态
 * 返回值      : none
 ************************************************************************************************************************/
static void touchpad_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
	(void)indev;

	/* 句柄有效性防御检查，防止硬件未就绪时空指针解引用崩溃 */
	if (t_lcd_touch_handle == NULL)
	{
		data->state = LV_INDEV_STATE_RELEASED;
		return;
	}

	/* 必须先从触摸控制器读取数据 */
	esp_lcd_touch_read_data(t_lcd_touch_handle);

	/* 获取触摸坐标 */
	esp_lcd_touch_point_data_t touch_data;
	uint8_t                    point_cnt = 0;

	esp_err_t                  ret       = esp_lcd_touch_get_data(t_lcd_touch_handle, &touch_data, &point_cnt, 1);

	if (ret == ESP_OK && point_cnt > 0)
	{
		data->point.x = touch_data.x;
		data->point.y = touch_data.y;
		data->state   = LV_INDEV_STATE_PRESSED;
	}
	else
		data->state = LV_INDEV_STATE_RELEASED;
}

#endif  /* boardDISPLAY_EN && ESP_LCD_TOUCH_ENABLED */
