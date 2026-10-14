/***********************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\WiFi
 * File    : wifi_queue_task.h
 * Date    : 2026-10-14
 * Author  : LJD(291483914@qq.com)
 * Desc    : WiFi 任务队列管理及各子任务状态机函数声明
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef WIFI_QUEUE_TASK_H
#define WIFI_QUEUE_TASK_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardWIFI_EN)
#include "queue_task.h"

#if (boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			wifiTASK_CYCLE_TIME						100		/* WiFi 任务空闲轮询等待周期 (ms) */

//****************************************************Types*********************************************************************//
/* WiFi 队列任务 ID 枚举 (1Tab -> 5Tab 对齐，闭合无空格) */
typedef enum
{
	WTI_NULL = 0,		/* 空任务 / 休眠态 */
	WTI_INIT,			/* 驱动与网络接口初始化任务 */
	WTI_WORK,			/* 已就绪工作态管理 (RSSI轮询) */
	WTI_CONNECT,		/* 主动发起连接任务 */
	WTI_RECONNECT,		/* 掉线指数退避重连任务 */
	WTI_DISCONNECT,		/* 主动断开连接任务 */
	WTI_STOP,			/* 停用射频休眠任务 */
	WTI_SNTP,			/* SNTP 网络时钟同步任务 */
	WTI_WEATHER,		/* HTTP 天气数据拉取与解析任务 */
}WiFiQueueTaskID_E;

//****************************************************Globals*******************************************************************//
extern Task_T *tpWiFiTask;								/* WiFi 任务队列控制块指针 */

//****************************************************Extern********************************************************************//
bool bWiFi_QueueInit(void);
uint32_t ulWiFi_CalcBackoffDelayMs(uint8_t uc_retry_count);

void v_wifi_queue_task_init(Task_T *p_task);
void v_wifi_queue_task_work(Task_T *p_task);
void v_wifi_queue_task_connect(Task_T *p_task);
void v_wifi_queue_task_reconnect(Task_T *p_task);
void v_wifi_queue_task_disconnect(Task_T *p_task);
void v_wifi_queue_task_stop(Task_T *p_task);
void v_wifi_queue_task_sntp(Task_T *p_task);
void v_wifi_queue_task_weather(Task_T *p_task);

#endif  /* boardWIFI_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* WIFI_QUEUE_TASK_H */
