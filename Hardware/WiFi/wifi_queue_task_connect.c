/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\WiFi
 * File    : wifi_queue_task_connect.c
 * Date    : 2026-10-14
 * Author  : LJD(291483914@qq.com)
 * Desc    : WiFi 主动发起连接队列子任务实现
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
 * 函数功能    : WiFi 主动发起连接子任务
 * 说明(备注)  : 响应外部指定配置的主动连接请求
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_wifi_queue_task_connect(Task_T *p_task)
{
	switch (p_task->ucStep)
	{
		case 0:
		{
			tWiFiInfo.eState = WIFI_STATE_CONNECTING;
			esp_err_t err = esp_wifi_connect();
			if (err != ESP_OK)
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_w("bWiFiTask:发起连接失败: %s", esp_err_to_name(err));

				tWiFiInfo.eState = WIFI_STATE_DISCONNECTED;
				cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
				log_i("bWiFiTask:正在连接AP: [%s]...", tWiFiInfo.saSsid);

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		case 1:
		{
			if (tWiFiInfo.eState == WIFI_STATE_CONNECTED ||
			    tWiFiInfo.eState == WIFI_STATE_DISCONNECTED ||
			    bQueue_IsStepTimeoutMs(p_task, wifiCONNECT_TIMEOUT_MS))
			{
				cQueue_GotoStep(p_task, STEP_END);
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
