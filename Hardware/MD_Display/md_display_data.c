/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display
 * File    : md_display_data.c
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 数据快照池实现 - 全局状态原子快照捕获与设备状态路由表
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_data.h"

#if (boardDISPLAY_EN)
#include "uni_disp_port.h"
#include "Sys/sys_task.h"
#include "MD_Display/md_display_task.h"

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 获取当前全局数据快照
 * 说明(备注)  : 临界区保护读取全局状态、功率与标志位，实现单向原子输入
 * 传入参数    : 无
 * 输出参数    : p_snapshot: 快照存放结构体指针
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_DataCaptureSnapshot(DispDataSnapshot_T *p_snapshot)
{
	if (p_snapshot == NULL)
		return;

	mainENTER_CRITICAL();

	/*----------------系统状态与功率----------------*/
	p_snapshot->ucDevState   = (uint8_t)tSysInfo.eDevState;
	p_snapshot->usSysErrCode = tSysInfo.uErrCode.usCode;
	p_snapshot->usInPwr      = tSysInfo.usInPwr;
	p_snapshot->usOutPwr     = tSysInfo.usOutPwr;

	/*----------------端口与电池预留状态 (已裁剪外设置零)----------------*/
	p_snapshot->ulPortActiveFlags    = 0;
	p_snapshot->ulPortBlinkFlags     = 0;
	p_snapshot->bBatErr              = false;
	p_snapshot->bBatLock             = false;
	p_snapshot->ucSoc                = 0;
	p_snapshot->usChgFullTimeMin     = 0;
	p_snapshot->usDisChgEmptyTimeMin = 0;

	mainEXIT_CRITICAL();

	/*----------------低频量 (依赖访问函数, 显示任务单读者直读安全)----------------*/
	p_snapshot->ucChgState       = (uint8_t)cSys_IsChgState();
	p_snapshot->usPrimaryErrCode = usDisp_ErrCodeDisplay();
}

/***********************************************************************************************************************
 * 函数功能    : 息屏倒计时生效判定
 * 说明(备注)  : 判断当前设备状态是否允许倒计时息屏 (仅工作状态参与倒计时递减)
 * 传入参数    : uc_dev_state: 当前设备状态枚举值
 * 输出参数    : 无
 * 返回值      : true: 允许倒计时递减; false: 暂停倒计时
 ************************************************************************************************************************/
bool bDisp_DataAutoOffArmed(uint8_t uc_dev_state)
{
	return (uc_dev_state == DS_WORK);
}

/***********************************************************************************************************************
 * 函数功能    : 设备状态到页面映射表
 * 说明(备注)  : 根据设备全局状态返回 UniDisplay 目标页面 ID
 * 传入参数    : uc_dev_state: 当前设备状态枚举值
 * 输出参数    : 无
 * 返回值      : DispPageId_E: 目标页面 ID, PAGE_ID_MAX 表示维持当前页
 ************************************************************************************************************************/
DispPageId_E eDisp_MapDevStateToPage(uint8_t uc_dev_state)
{
	switch ((DevState_E)uc_dev_state)
	{
		case DS_INIT:        return PAGE_ID_INIT;        /* 初始化: 保持息屏 */
		case DS_BOOTING:     return PAGE_ID_BOOT;        /* 启动中: 开机步进动画页 */
		case DS_ERR:         return PAGE_ID_FAULT;       /* 故障态: 故障专用告警页 */
		case DS_CLOSING:     return PAGE_ID_CLOSING;     /* 关机中: 关机画面展示 */
		case DS_SHUT_DOWN:   return PAGE_ID_SHUT_DOWN;   /* 已关机: 关机状态页(息屏) */
		#if (boardUPDATE)
		case DS_UPDATE_MODE: return PAGE_ID_UPDATE;      /* 升级中: 固件升级进度页 */
		#endif  /* boardUPDATE */
		#if (boardENG_MODE_EN)
		case DS_ENG_MODE:    return PAGE_ID_ENG_MODE;    /* 工程模式: 工程校准设置页 */
		#endif  /* boardENG_MODE_EN */
		default:             return PAGE_ID_MAX;         /* 无映射: 维持当前页 (如工作态在开机进度完成后跳转) */
	}
}

#endif  /* boardDISPLAY_EN */
