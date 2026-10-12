/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display
 * File    : md_display_page_update.c
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 固件升级页面 (TFT+LVGL) - 升级进度与状态显示
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_data.h"

#if (boardDISPLAY_EN && boardUPDATE)
#include "uni_disp_core.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "lvgl.h"
#include "eez_ui/screens.h"

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//
static void v_page_update_enter(void);
static void v_page_update_exit(void);
static void v_page_update_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_update_event(DispEvent_E e_event, uint32_t ul_param);

const DispPage_T G_tPageUpdate =
{
	.ePageId     = PAGE_ID_UPDATE,
	.vOnEnter    = v_page_update_enter,
	.vOnExit     = v_page_update_exit,
	.vOnUpdate   = v_page_update_update,
	.bOnEvent    = b_page_update_event,
	.usRefreshMs = 33,
};

/***********************************************************************************************************************
 * 函数功能    : 进入升级页
 * 说明(备注)  : 
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void v_page_update_enter(void)
{
	vDisp_LoadScreen(SCREEN_ID_MAIN_UPDATE);
	bDisp_Switch(ST_ON, false);
}

/***********************************************************************************************************************
 * 函数功能    : 退出升级页面
 * 说明(备注)  : 离开升级流程
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_update_exit(void)
{
}

/***********************************************************************************************************************
 * 函数功能    : 升级页面帧刷新
 * 说明(备注)  : 刷新升级进度与状态
 * 传入参数    : p_data: 数据快照, b_force: 强制刷新标志
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_update_update(const DispDataSnapshot_T *p_data, bool b_force)
{
	(void)b_force;

	vDisp_UpdateDevParam(p_data);
}

/***********************************************************************************************************************
 * 函数功能    : 升级页面事件处理
 * 说明(备注)  : 透传给框架
 * 传入参数    : e_event: 事件类型, ul_param: 事件参数
 * 输出参数    : 无
 * 返回值      : bool: false (透传)
 ************************************************************************************************************************/
static bool b_page_update_event(DispEvent_E e_event, uint32_t ul_param)
{
	(void)e_event;
	(void)ul_param;
	return false;
}

#endif  /* boardDISPLAY_EN && boardUPDATE */
