/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display
 * File    : md_display_page_boot.c
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 开机页面 - 开机步进动画与进入工作态过渡
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_data.h"

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "uni_disp_core.h"
#include "lvgl.h"
#include "eez_ui/screens.h"

//****************************************************Parameter Initialization**************************************************//
static uint8_t s_uc_loading_step = 0;

//****************************************************Function Declaration******************************************************//
static void v_page_boot_enter(void);
static void v_page_boot_exit(void);
static void v_page_boot_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_boot_event(DispEvent_E e_event, uint32_t ul_param);


const DispPage_T G_tPageBoot =
{
    .ePageId     = PAGE_ID_BOOT,
    .vOnEnter    = v_page_boot_enter,
	.vOnExit     = v_page_boot_exit,
    .vOnUpdate   = v_page_boot_update,
    .bOnEvent    = b_page_boot_event,
    .usRefreshMs = 33,      /* 30FPS, 与框架下限 dispFRAME_PERIOD_MIN_MS 对齐 */
};

/***********************************************************************************************************************
 * 函数功能    : 进入开机页面
 * 说明(备注)  : 初始化开机状态、重置开机计数并开启背光
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_boot_enter(void)
{
    /* 加载开机画面 */
    vDisp_LoadScreen(SCREEN_ID_MAIN_BOOTING);

	/* 重置开机进度 */
    s_uc_loading_step = 0;

    /* 刷新设备参数 */
    vDisp_UpdateDevParam(NULL);

    /* 开启背光 */
    bDisp_Switch(ST_ON, false);
}

/***********************************************************************************************************************
 * 函数功能    : 退出开机页面
 * 说明(备注)  : 完成开机流程
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_boot_exit(void)
{
}

/***********************************************************************************************************************
 * 函数功能    : 开机页面帧更新
 * 说明(备注)  : 步进进度计数；当达到 100% 且系统已进入 DS_WORK 时，显式请求切换至工作页
 * 传入参数    : p_data: 数据快照, b_force: 强制刷新标志
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_boot_update(const DispDataSnapshot_T *p_data, bool b_force)
{
    (void)p_data;
    (void)b_force;

	/* 步进进度计数 */
    if (s_uc_loading_step < 100)
    {
        s_uc_loading_step += 3;
        if (s_uc_loading_step > 100)
            s_uc_loading_step = 100;
	}

    /* 当进度条走满且系统已经处于工作态时，请求切入工作页面 */
    if ((p_data->ucDevState == DS_WORK) && (s_uc_loading_step >= 100))
        vDisp_RequestPage(PAGE_ID_WORK);
}

/***********************************************************************************************************************
 * 函数功能    : 开机页面事件处理
 * 说明(备注)  : none
 * 传入参数    : e_event: 页面事件类型; ul_param: 事件参数
 * 输出参数    : 无
 * 返回值      : false: 透传框架默认处理（本页不消费任何事件）
 ************************************************************************************************************************/
static bool b_page_boot_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)e_event;
    (void)ul_param;
    return false;   /* 透传框架默认处理 */
}

#endif  /* boardDISPLAY_EN */

