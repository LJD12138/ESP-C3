/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display
 * File    : md_display_page_eng.c
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 工程模式页面 (TFT+LVGL) - 工程参数与校准设置
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_data.h"

#if (boardDISPLAY_EN && boardENG_MODE_EN)
#include "uni_disp_core.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "lvgl.h"
#include "eez_ui/screens.h"

//****************************************************Macros********************************************************************//
#define			dispPAGE_ENG_CYCLE_TIME_MS				20U

//****************************************************Function Declaration******************************************************//
static void v_page_eng_enter(void);
static void v_page_eng_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_eng_event(DispEvent_E e_event, uint32_t ul_param);
static void v_page_eng_exit(void);

const DispPage_T G_tPageEng =
{
    .ePageId     = PAGE_ID_ENG_MODE,
    .vOnEnter    = v_page_eng_enter,
    .vOnUpdate   = v_page_eng_update,
    .bOnEvent    = b_page_eng_event,
    .vOnExit     = v_page_eng_exit,
    .usRefreshMs = dispPAGE_ENG_CYCLE_TIME_MS,
};

/***********************************************************************************************************************
 * 函数功能    : 进入工程模式页面
 * 说明(备注)  : 加载工程界面并点亮背光
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_enter(void)
{
	vDisp_LoadScreen(SCREEN_ID_MAIN_ENG);
	bDisp_Switch(ST_ON, false);
}

/***********************************************************************************************************************
 * 函数功能    : 退出工程模式页面
 * 说明(备注)  : 离开工程模式
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_exit(void)
{
}

/***********************************************************************************************************************
 * 函数功能    : 工程模式页面帧更新
 * 说明(备注)  : 驱动工程模式数据刷新
 * 传入参数    : p_data: 快照数据指针, b_force: 强制刷新标志
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_eng_update(const DispDataSnapshot_T *p_data, bool b_force)
{
    (void)b_force;

	vDisp_UpdateDevParam(p_data);
}

/***********************************************************************************************************************
 * 函数功能    : 工程模式页面事件处理
 * 说明(备注)  : 
 * 传入参数    : e_event: 事件枚举, ul_param: 事件参数
 * 输出参数    : 无
 * 返回值      : false: 透传给框架默认处理
 ************************************************************************************************************************/
static bool b_page_eng_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)e_event;
    (void)ul_param;

    return false;   /* 透传重置息屏等默认行为 */
}

#endif  /* boardDISPLAY_EN && boardENG_MODE_EN */

