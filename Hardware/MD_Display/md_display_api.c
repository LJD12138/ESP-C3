/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display
 * File    : md_display_api.c
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示对外 API 实现 - LVGL9核心/EEZ UI调度/触摸接入/背光协同
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_api.h"

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_iface.h"
#include "uni_disp_port.h"

#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

#include "eez_ui/ui.h"
#include "eez_ui/screens.h"

#if (boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif  /* boardUSE_OS */
#include "esp_timer.h"

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//
/* UI 屏幕对象已创建标志 (防重复 ui_init 导致 EEZ 屏幕对象重复创建) */
static bool G_bUiInitialized = false;

//****************************************************Function Declaration******************************************************//
static uint32_t prv_lv_tick_get_cb(void);

/***********************************************************************************************************************
 * 函数功能    : LVGL 系统毫秒时基回调
 * 说明(备注)  : 提供稳定准确的高精度毫秒时基，驱动动画、定时器与输入设备消抖
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : uint32_t: 当前系统毫秒数
 ************************************************************************************************************************/
static uint32_t prv_lv_tick_get_cb(void)
{
	return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

/***********************************************************************************************************************
 * 函数功能    : 显示模块初始化函数
 * 说明(备注)  : 完成 LVGL 初始化、驱动层挂载、触摸屏使能与 UI 创建
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_Init(void)
{
	lv_obj_t *active_screen;

	/* 1. 初始化 LVGL 核心库并注册系统毫秒时基 */
	if (!lv_is_initialized())
	{
		lv_init();
		lv_tick_set_cb(prv_lv_tick_get_cb);
	}

	/* 2. 初始化显示驱动 */
	if (lv_display_get_default() == NULL)
		lv_port_disp_init();

	/* 3. 初始化触摸输入设备 (如果已使能) */
	#if (ESP_LCD_TOUCH_ENABLED)
	lv_port_indev_init();
	#endif  /* ESP_LCD_TOUCH_ENABLED */

	/* 4. 初始化 EEZ Studio UI */
	if (G_bUiInitialized == false)
	{
		G_bUiInitialized = true;
		ui_init();
	}

	active_screen = lv_screen_active();
	if (active_screen != NULL)
		lv_obj_invalidate(active_screen);

	/* 5. 初始关闭背光，等待系统根据状态机按需点亮 */
	vDisp_SetBacklight(0);
}

/***********************************************************************************************************************
 * 函数功能    : 安全加载指定屏幕
 * 说明(备注)  : 仅在显示任务上下文调用，切换活动 LVGL 屏幕
 * 传入参数    : screen_id: 目标屏幕枚举/ID (ScreensEnum)
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_LoadScreen(int screen_id)
{
	lv_obj_t *p_target = NULL;

	if (!lv_is_initialized())
		return;

	switch (screen_id)
	{
		case SCREEN_ID_MAIN_WORK:
			p_target = objects.main_work;
			break;
		case SCREEN_ID_MAIN_BOOTING:
			p_target = objects.main_booting;
			break;
		case SCREEN_ID_MAIN_CLOSING:
			p_target = objects.main_closing;
			break;
		case SCREEN_ID_MAIN_ENG:
			p_target = objects.main_eng;
			break;
		case SCREEN_ID_MAIN_UPDATE:
			p_target = objects.main_update;
			break;
		default:
			p_target = objects.main_work;
			break;
	}

	if (p_target != NULL && lv_screen_active() != p_target)
		lv_screen_load(p_target);
}

/***********************************************************************************************************************
 * 函数功能    : 请求执行一次 UI 业务刷新
 * 说明(备注)  : 唤醒处于休眠/等待的显示任务执行界面刷新
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_ReqUiRefresh(void)
{
	vDisp_PortWakeTask();
}

/***********************************************************************************************************************
 * 函数功能    : 执行挂起的 UI 业务刷新
 * 说明(备注)  : 在显示任务 EnginePoll 每帧末尾调用，推进 EEZ tick 与 LVGL 定时器
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_UiRefresh(void)
{
	if (!lv_is_initialized())
		return;

	/* 1. EEZ Studio 变量绑定与视图更新 */
	ui_tick();

	/* 2. LVGL 核心定时器与重绘处理 */
	lv_timer_handler();
}

/***********************************************************************************************************************
 * 函数功能    : 更新 UI 界面设备业务参数
 * 说明(备注)  : 供页面帧更新钩子调用 (随后续具体 UI 扩展控件绑定)
 * 传入参数    : p_snapshot: 数据快照结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_UpdateDevParam(const struct DispDataSnapshot_T *p_snapshot)
{
	(void)p_snapshot;
	/* 保留参数刷新钩子供后续扩展具体控件属性 */
}

/***********************************************************************************************************************
 * 函数功能    : 显示就绪状态检查函数
 * 说明(备注)  : 检查 LVGL 核心是否已初始化完成
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 显示模块已初始化并就绪; false: 未初始化
 ************************************************************************************************************************/
bool bDisp_IsReady(void)
{
	return lv_is_initialized();
}

/***********************************************************************************************************************
 * 函数功能    : 清屏指定颜色
 * 说明(备注)  : 设置活动屏幕背景颜色并标记脏矩形
 * 传入参数    : us_color: RGB565 颜色值
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_ClearScreen(uint16_t us_color)
{
	if (!lv_is_initialized())
		return;

	lv_obj_t *active_screen = lv_screen_active();
	if (active_screen != NULL)
	{
		lv_color_t color = lv_color_hex(((us_color & 0xF800) << 8) | ((us_color & 0x07E0) << 5) | ((us_color & 0x001F) << 3));
		lv_obj_set_style_bg_color(active_screen, color, LV_PART_MAIN);
		lv_obj_invalidate(active_screen);
	}
}

#endif  /* boardDISPLAY_EN */
