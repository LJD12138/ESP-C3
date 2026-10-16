/***********************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\main
 * File    : timer_task.h
 * Date    : 2026-10-11
 * Author  : LJD(291483914@qq.com)
 * Desc    : 软件定时器任务及各外设超时回调声明头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef TIMER_TASK_H_
#define TIMER_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#endif  /* boardUSE_OS */

#if (1)
//****************************************************Macros********************************************************************//

//****************************************************Types*********************************************************************//

//****************************************************Globals*******************************************************************//
#if (boardBMS_485_IFACE_EN)
extern TimerHandle_t                    tBmsRxEnTimer;                          /* 单次定时器: BMS的485发送延时切换 */
#endif  /* boardBMS_485_IFACE_EN */

extern TimerHandle_t                    tWakeUpBmsTimer;                        /* 单次定时器: 辅助开机脉冲延时关闭(兼恢复bIF_Gpio解锁按键, 不随BMS裁剪) */

//****************************************************Extern********************************************************************//
s8   cTimer_TaskInit(void);

#if (boardLOW_POWER)
void vCount_EnterLowPower(void);
void vCount_ExitLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* 1 */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* TIMER_TASK_H_ */
