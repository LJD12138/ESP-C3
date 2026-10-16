/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display
 * File    : md_display_page_work.c
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 主工作页面 (TFT+LVGL) - 调度 EEZ 工作界面与实时渲染
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_data.h"

#if (boardDISPLAY_EN)
#include "uni_disp_core.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "user_ui/home_ui_ext.h"

#include "lvgl.h"
#include "esp_log.h"
#include "eez_ui/screens.h"

//****************************************************Static Variables**********************************************************//

//****************************************************Function Declaration******************************************************//
static void v_page_work_enter(void);
static void v_page_work_exit(void);
static void v_page_work_update(const DispDataSnapshot_T *p_data, bool b_force);
static bool b_page_work_event(DispEvent_E e_event, uint32_t ul_param);

const DispPage_T G_tPageWork =
{
    .ePageId     = PAGE_ID_WORK,
    .vOnEnter    = v_page_work_enter,
    .vOnExit     = v_page_work_exit,
    .vOnUpdate   = v_page_work_update,
    .bOnEvent    = b_page_work_event,
    .usRefreshMs = 33,      /* 30FPS, 与框架下限 dispFRAME_PERIOD_MIN_MS 对齐 */
};

/***********************************************************************************************************************
 * 函数功能    : 进入工作页面
 * 说明(备注)  : 加载主工作界面，初始化主页扩展UI与动画，首刷数据并确保背光开启
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_work_enter(void)
{
    DispDataSnapshot_T t_snapshot;

    /* 确保当前活动屏幕为主工作屏幕 */
    if (lv_screen_active() != objects.main_work)
        vDisp_LoadScreen(SCREEN_ID_MAIN_WORK);

    /* 抓取一次快照用于首帧静态渲染 */
    vDisp_DataCaptureSnapshot(&t_snapshot);

    /* 刷新主页温湿度、时间和天气数据 */
    vHomeUi_Update(&t_snapshot);

    /* 启动主页动态动效 (天气动画等) */
    vHomeUi_AnimaEnter();

    lv_refr_now(NULL);

    /* 进入工作页若背光未开启, 显式点亮背光 */
    if (bDisp_IsBacklightOn() == false)
        bDisp_Switch(ST_ON, false);
}

/***********************************************************************************************************************
 * 函数功能    : 退出工作页面
 * 说明(备注)  : 清理主工作界面动画与动效
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_work_exit(void)
{
    /* 停止主页动画，防止退页后产生无效脏矩形重绘 */
    vHomeUi_AnimaStop();
}

/***********************************************************************************************************************
 * 函数功能    : 工作页面帧更新
 * 说明(备注)  : 轮询跨任务切页请求，从快照渲染温湿度、网络时间、天气及信号
 * 传入参数    : p_data: 快照数据指针, b_force: 强制刷新标志
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_page_work_update(const DispDataSnapshot_T *p_data, bool b_force)
{
    (void)b_force;

    /* 处理跨任务切页请求 */
    vHomeUi_PollRequest();

    /* 周期更新主页环境温湿度、天气与时间 */
    vHomeUi_Update(p_data);
}

/***********************************************************************************************************************
 * 函数功能    : 工作页事件回调
 * 说明(备注)  : 短按按键切换 Tileview 双 Tile，其余事件透传框架默认处理
 * 传入参数    : e_event: 事件类型
 * 输出参数    : ul_param: 事件参数
 * 返回值      : true: 已消费事件, false: 透传给框架
 ************************************************************************************************************************/
static bool b_page_work_event(DispEvent_E e_event, uint32_t ul_param)
{
    (void)ul_param;

    if (e_event == DISP_EVT_KEY_SHORT)
    {
        /* 短按切换主页1 (环境卡片) 与 主页2 (应用网格) */
        uint8_t cur_tile = ucHomeUi_GetTile();
        bHomeUi_SetTile(cur_tile ^ 1U, true);
        return true;
    }

    return false;   /* 透传框架默认处理: 按键重置息屏倒计时, 故障自动弹窗等 */
}

#endif  /* boardDISPLAY_EN */
