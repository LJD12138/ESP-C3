/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display
 * File    : md_display_err.c
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : LCD 错误代码轮播显示 (X-macro 单一数据源表驱动)
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"

#if (boardDISPLAY_EN)
#include "Sys/sys_task.h"

#if (boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
/*--------------------------------------------------
 * 故障清单 (单一数据源): 每个故障仅此一行, 下方自动展开生成
 * "检测函数" 与 "轮播表", 新增故障只需在此追加一行即可
 * 行格式: X(显示码, 检测函数名, 位域表达式)
 *-------------------------------------------------*/
#define			ERR_LIST_SYS(X)							\
	X(1,   sys_ot,                tSysInfo.uErrCode.tCode.bOT) \
	X(2,   sys_ut,                tSysInfo.uErrCode.tCode.bUT) \
	X(3,   sys_ov,                tSysInfo.uErrCode.tCode.bOV) \
	X(4,   sys_uv,                tSysInfo.uErrCode.tCode.bUV) \
	X(5,   sys_ol,                tSysInfo.uErrCode.tCode.bOL) \
	X(6,   sys_dischg_ol,         tSysInfo.uErrCode.tCode.bDisChgOL) \
	X(7,   sys_0_soc,             tSysInfo.uErrCode.tCode.b0SOC) \
	X(8,   sys_close_fault,       tSysInfo.uErrCode.tCode.bCloseFault) \
	X(9,   sys_boot_fault,        tSysInfo.uErrCode.tCode.bBootFault) \
	X(10,  sys_task_fault,        tSysInfo.uErrCode.tCode.bTaskFault)

#define			ERR_LIST_ALL(X)							\
	ERR_LIST_SYS(X)

//****************************************************Parameter Initialization**************************************************//
typedef bool (*fnErrActive_T)(void);

typedef struct
{
	uint16_t			usCode;				/* 故障显示码 */
	fnErrActive_T		pfActive;			/* 故障激活检测函数指针 */
}ErrItem_T;

/* 展开生成各个具体故障项的激活判定函数 */
#define			X_FUNC(code, name, expr)				static bool b_err_##name##_active(void) { return (expr) != 0; }
ERR_LIST_ALL(X_FUNC)
#undef			X_FUNC

/* 展开生成故障轮播静态配置表 */
#define			X_ITEM(code, name, expr)				{ code, b_err_##name##_active },
static const ErrItem_T S_tErrTable[] =
{
	ERR_LIST_ALL(X_ITEM)
};
#undef			X_ITEM

#define			ERR_TABLE_NUM							mainARRAY_SIZE(S_tErrTable)

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 轮播计算主告警故障显示码
 * 说明(备注)  : 表驱动环形扫描；无故障返回 0；单故障常显(不插入空白帧)；多故障按页帧周期累加至 2000ms 轮显
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : uint16_t: 当前应显示的故障码 (0 表示无故障或空白切换帧)
 ************************************************************************************************************************/
uint16_t usDisp_ErrCodeDisplay(void)
{
	static uint16_t s_us_idx       = 0;
	static uint16_t s_us_disp_ms   = 0;
	static uint16_t s_us_blank     = 0;
	static uint16_t s_us_curr_code = 0;

	uint16_t us_scan;
	uint8_t  uc_active_cnt = 0;

	/* 快速路径: 系统故障字全 0 时直接退出 */
	if (tSysInfo.uErrCode.usCode == 0)
	{
		s_us_idx       = 0;
		s_us_disp_ms   = 0;
		s_us_blank     = 0;
		s_us_curr_code = 0;
		return 0;
	}

	/* 统计当前激活故障总数 */
	for (us_scan = 0; us_scan < ERR_TABLE_NUM; us_scan++)
	{
		if (S_tErrTable[us_scan].pfActive != NULL && S_tErrTable[us_scan].pfActive())
		{
			uc_active_cnt++;
			if (uc_active_cnt > 1)
				break;
		}
	}

	/* 没有任何激活故障 */
	if (uc_active_cnt == 0)
	{
		s_us_idx       = 0;
		s_us_disp_ms   = 0;
		s_us_blank     = 0;
		s_us_curr_code = 0;
		return 0;
	}

	/* 单故障场景: 持续常显, 绝不插入灭屏空白帧, 避免单故障呼吸闪烁 */
	if (uc_active_cnt <= 1)
		s_us_blank = 0;
	else if (s_us_blank)
	{
		/* 多故障轮播切换时的单帧空白间隔 */
		s_us_blank = 0;
		return 0;
	}

	/* 自 s_us_idx 起环形扫描下一个激活的故障 */
	for (us_scan = 0; us_scan < ERR_TABLE_NUM; us_scan++)
	{
		if (S_tErrTable[s_us_idx].pfActive != NULL && S_tErrTable[s_us_idx].pfActive())
			break;
		s_us_idx = (s_us_idx + 1) % ERR_TABLE_NUM;
	}

	if (us_scan >= ERR_TABLE_NUM)
		return 0;

	uint16_t us_active_code = S_tErrTable[s_us_idx].usCode;

	/* 故障码发生变更 (如某故障中途解除或轮播至新故障), 重置计时 */
	if (s_us_curr_code != us_active_code)
	{
		s_us_curr_code = us_active_code;
		s_us_disp_ms   = 0;
	}

	/* 显示当前激活故障: 持续 2000ms, 按当前页面帧周期累计 */
	s_us_disp_ms += usDisp_GetFramePeriod();
	if (s_us_disp_ms >= 2000)
	{
		s_us_disp_ms = 0;
		if (uc_active_cnt > 1)
		{
			s_us_idx   = (s_us_idx + 1) % ERR_TABLE_NUM;   /* 指向下一个 */
			s_us_blank = 1;                                /* 多故障时插入一帧空白间隔 */
			return 0;
		}
	}

	return us_active_code;
}

#endif  /* boardDISPLAY_EN */
