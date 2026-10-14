/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\WiFi
 * File    : wifi_queue_task_reconnect.c
 * Date    : 2026-10-14
 * Author  : LJD(291483914@qq.com)
 * Desc    : WiFi 掉线指数退避重连状态机队列子任务实现
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
#include "Print/print_task.h"
#include "esp_wifi.h"

//****************************************************Macros********************************************************************//
#define			wifiCONNECT_TIMEOUT_MS					15000	/* 连接单次超时判定门限 15s */

//****************************************************Parameter Initialization**************************************************//

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 计算断线重连指数退避时间
 * 说明(备注)  : 避免重连风暴对 CPU、电量及路由器的过度挤占，最小 1s，最大 30s
 * 传入参数    : uc_retry_count: 当前连续失败次数
 * 输出参数    : 无
 * 返回值      : 延迟毫秒数 (ms)
 ************************************************************************************************************************/
uint32_t ulWiFi_CalcBackoffDelayMs(uint8_t uc_retry_count)
{
	uint32_t ul_factor = 1U << (uc_retry_count > 5U ? 5U : uc_retry_count);
	uint32_t ul_delay  = (uint32_t)wifiRECONNECT_BASE_INTERVAL_MS * ul_factor;

	if (ul_delay > (uint32_t)wifiRECONNECT_MAX_INTERVAL_MS)
		ul_delay = (uint32_t)wifiRECONNECT_MAX_INTERVAL_MS;

	return ul_delay;
}

/***********************************************************************************************************************
 * 函数功能    : WiFi 掉线指数退避重连状态机
 * 说明(备注)  : Step 0: 计算退避延时; Step 1: 物理时间等待; Step 2: 发起连接; Step 3: 超时监控
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_wifi_queue_task_reconnect(Task_T *p_task)
{
	if (lwrb_get_full(&p_task->tQueueBuff) > 0)
	{
		cQueue_GotoStep(p_task, STEP_END);
		return;
	}

	switch (p_task->ucStep)
	{
		case 0:
		{
			uint32_t ul_backoff_ms = ulWiFi_CalcBackoffDelayMs(tWiFiInfo.ucRetryCnt);

			if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
				log_w("bWiFiTask:WiFi断开连接, 第%u次重试将在%u ms后发起", (unsigned int)tWiFiInfo.ucRetryCnt, (unsigned int)ul_backoff_ms);

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		case 1:
		{
			uint32_t ul_backoff_ms = ulWiFi_CalcBackoffDelayMs(tWiFiInfo.ucRetryCnt);

			if (bQueue_IsStepTimeoutMs(p_task, ul_backoff_ms))
				cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		case 2:
		{
			tWiFiInfo.eState = WIFI_STATE_CONNECTING;
			esp_wifi_connect();

			if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
				log_i("bWiFiTask:发起第%u次重连AP: [%s]...", (unsigned int)tWiFiInfo.ucRetryCnt, tWiFiInfo.saSsid);

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		case 3:
		{
			if (tWiFiInfo.eState == WIFI_STATE_CONNECTED)
			{
				cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			if (tWiFiInfo.eState == WIFI_STATE_DISCONNECTED)
			{
				cQueue_GotoStep(p_task, 0);
				break;
			}

			if (bQueue_IsStepTimeoutMs(p_task, wifiCONNECT_TIMEOUT_MS))
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_w("bWiFiTask:等待连接AP超时，准备下一轮重试");

				esp_wifi_disconnect();
				tWiFiInfo.ucRetryCnt++;
				cQueue_GotoStep(p_task, 0);
			}
		}
		break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);
		}
		break;
	}

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(wifiTASK_CYCLE_TIME));
	#endif  /* boardUSE_OS */
}

#endif  /* boardWIFI_EN */
