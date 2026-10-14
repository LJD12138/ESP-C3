/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\WiFi
 * File    : wifi_queue_task_init.c
 * Date    : 2026-10-14
 * Author  : LJD(291483914@qq.com)
 * Desc    : WiFi 驱动初始化与首次网络连接队列子任务实现
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
#include "esp_wifi.h"

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : WiFi 初始化队列子任务
 * 说明(备注)  : 依次注册事件监听、配置STA参数、启动射频驱动、置就绪标志并触发首轮连接
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_wifi_queue_task_init(Task_T *p_task)
{
	switch (p_task->ucStep)
	{
		//************************************步骤0:注册底层WiFi与IP事件监听************************************
		case 0:
		{
			if (!bWiFi_RegisterEvents())
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_e("bWiFiTask:注册事件监听失败");

				if (bQueue_IsStepRetryOver(p_task, 3))
					cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		//************************************步骤1:配置STA工作模式与网络凭据**********************************
		case 1:
		{
			vWiFi_ApplyStaConfig(tWiFiInfo.saSsid, tWiFiInfo.saPassword);
			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		//************************************步骤2:启动WiFi底层射频驱动****************************************
		case 2:
		{
			if (!bWiFi_StartDriver())
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_w("bWiFiTask:启动WiFi射频驱动重试");

				if (bQueue_IsStepRetryOver(p_task, 3))
					cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		//************************************步骤3:就绪标记与完成初始化****************************************
		case 3:
		{
			tSysInfo.uInit.tFinish.bIF_WiFiTask = 1;

			if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
				log_i("bWiFiTask:网络硬件与驱动就绪，进入事件驱动连接流程");

			cQueue_GotoStep(p_task, STEP_END);
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
