/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\WiFi
 * File    : wifi_queue_task_work.c
 * Date    : 2026-10-14
 * Author  : LJD(291483914@qq.com)
 * Desc    : WiFi 正常就绪工作态管理队列子任务实现，包含 RSSI 采样与 SNTP 周期对时触发
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

//****************************************************Parameter Initialization**************************************************//
#if (boardUSE_OS)
static uint32_t s_ul_last_sntp_sync_tick = 0;
static uint32_t s_ul_last_weather_tick   = 0;
#endif  /* boardUSE_OS */

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : WiFi 正常就绪工作态管理任务
 * 说明(备注)  : 周期维护 RSSI 信号强度，触发首轮与周期性 SNTP 网络对时与天气更新
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_wifi_queue_task_work(Task_T *p_task)
{
	if (tWiFiInfo.eState != WIFI_STATE_CONNECTED)
	{
		cQueue_GotoStep(p_task, STEP_END);
		return;
	}

	if (lwrb_get_full(&p_task->tQueueBuff) > 0)
	{
		cQueue_GotoStep(p_task, STEP_END);
		return;
	}

	switch (p_task->ucStep)
	{
		case 0:
		{
			/* 1. 周期刷新 RSSI 信号强度 */
			if (bQueue_IsStepTimeoutMs(p_task, wifiRSSI_POLL_INTERVAL_MS))
			{
				wifi_ap_record_t ap_info;
				if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
					tWiFiInfo.cRssi = ap_info.rssi;
				else
					tWiFiInfo.cRssi = -100;

				vQueue_RefreshStepTick(p_task);
			}

			#if (boardUSE_OS)
			/* 2. 网络就绪后自动触发首轮或周期性 SNTP 对时 */
			bool b_need_sntp = (!tWiFiInfo.bSntpSynced) ||
			                   ((xTaskGetTickCount() - s_ul_last_sntp_sync_tick) >= pdMS_TO_TICKS(wifiSNTP_SYNC_INTERVAL_MS));

			if (b_need_sntp)
			{
				s_ul_last_sntp_sync_tick = xTaskGetTickCount();
				cQueue_AddQueueTask(p_task, WTI_SNTP, 0, false);
				cQueue_GotoStep(p_task, STEP_END);
				return;
			}

			/* 3. SNTP 成功对时后，自动触发首轮或周期性天气数据拉取 (成功间隔30分钟，失败退避重试10秒) */
			if (tWiFiInfo.bSntpSynced)
			{
				uint32_t ul_interval_ms = tWiFiInfo.tWeather.bValid ? wifiWEATHER_UPDATE_INTERVAL_MS : wifiWEATHER_RETRY_INTERVAL_MS;
				bool b_need_weather     = (s_ul_last_weather_tick == 0) ||
				                          ((xTaskGetTickCount() - s_ul_last_weather_tick) >= pdMS_TO_TICKS(ul_interval_ms));

				if (b_need_weather)
				{
					s_ul_last_weather_tick = xTaskGetTickCount();
					cQueue_AddQueueTask(p_task, WTI_WEATHER, 0, false);
					cQueue_GotoStep(p_task, STEP_END);
					return;
				}
			}
			#endif  /* boardUSE_OS */
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
