/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\WiFi
 * File    : wifi_queue_task.c
 * Date    : 2026-10-14
 * Author  : LJD(291483914@qq.com)
 * Desc    : WiFi 任务队列初始化、分发管理与调度回调实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "WiFi/wifi_queue_task.h"

#if (boardWIFI_EN)
#include "WiFi/wifi_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) Task_T *tpWiFiTask = NULL;									/* WiFi 队列任务控制块指针 */

//****************************************************Function Declaration******************************************************//
static bool b_task_manage_func_cb(Task_T *p_task);
static void v_add_task_return_func_cb(Task_T *p_task, u8 num);


/***********************************************************************************************************************
 * 函数功能    : WiFi 任务队列初始化
 * 说明(备注)  : 创建 TaskQueue 控制块并绑定装载管理函数与事件回调
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功; false: 失败
 ************************************************************************************************************************/
bool bWiFi_QueueInit(void)
{
	if (cQueue_TaskInit(&tpWiFiTask, 8, 32, b_task_manage_func_cb, v_add_task_return_func_cb) <= 0)
	{
		if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
			log_e("bWiFiTask:tpWiFiTask任务对象初始化失败");

		return false;
	}
	else if (tpWiFiTask == NULL)
	{
		if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
			log_e("bWiFiTask:tpWiFiTask任务对象创建失败");

		return false;
	}

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 任务管理装载回调函数
 * 说明(备注)  : 出队执行外部指令，或根据当前网络状态自动挂接对应默认任务
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : p_task: 更新任务状态与执行函数
 * 返回值      : true: 成功; false: 失败
 ************************************************************************************************************************/
static bool b_task_manage_func_cb(Task_T *p_task)
{
	TaskItem_T t_item;

	if (p_task == NULL)
		return false;

	vQueue_ResetTaskState(p_task);

	if (tSysInfo.uInit.tFinish.bIF_WiFiTask == 0)
	{
		p_task->ucID      = WTI_INIT;
		p_task->usInParam = 0;
	}
	else if (bQueue_PopTask(p_task, &t_item))
	{
		p_task->ucID      = t_item.ucId;
		p_task->usInParam = t_item.usParam;
	}
	else
	{
		if (tWiFiInfo.eState == WIFI_STATE_CONNECTED)
		{
			p_task->ucID      = WTI_WORK;
			p_task->usInParam = 0;
		}
		else if (tWiFiInfo.eState == WIFI_STATE_STOPPED)
		{
			p_task->ucID      = WTI_NULL;
			p_task->usInParam = 0;
		}
		else
		{
			if (tWiFiInfo.bAutoConnect)
			{
				p_task->ucID      = WTI_RECONNECT;
				p_task->usInParam = 0;
			}
			else
			{
				p_task->ucID      = WTI_NULL;
				p_task->usInParam = 0;
			}
		}
	}

	switch (p_task->ucID)
	{
		case WTI_INIT:
		{
			p_task->vp_func = v_wifi_queue_task_init;
		}
		break;

		case WTI_WORK:
		{
			p_task->vp_func = v_wifi_queue_task_work;
		}
		break;

		case WTI_CONNECT:
		{
			p_task->vp_func = v_wifi_queue_task_connect;
		}
		break;

		case WTI_RECONNECT:
		{
			p_task->vp_func = v_wifi_queue_task_reconnect;
		}
		break;

		case WTI_DISCONNECT:
		{
			p_task->vp_func = v_wifi_queue_task_disconnect;
		}
		break;

		case WTI_STOP:
		{
			p_task->vp_func = v_wifi_queue_task_stop;
		}
		break;

		case WTI_SCAN:
		{
			p_task->vp_func = v_wifi_queue_task_scan;
		}
		break;

		case WTI_SNTP:
		{
			p_task->vp_func = v_wifi_queue_task_sntp;
		}
		break;

		case WTI_WEATHER:
		{
			p_task->vp_func = v_wifi_queue_task_weather;
		}
		break;

		case WTI_NULL:
		default:
		{
			p_task->vp_func   = NULL;
			p_task->usInParam = 0;
		}
		break;
	}

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 任务队列事件回调
 * 说明(备注)  : 响应入队与调度状态变化
 * 传入参数    : p_task: 任务结构体指针, num: 事件类型编号
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_add_task_return_func_cb(Task_T *p_task, u8 num)
{
	(void)p_task;
	(void)num;
}

#endif  /* boardWIFI_EN */
