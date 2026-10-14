/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\WiFi
 * File    : wifi_queue_task_sntp.c
 * Date    : 2026-10-14
 * Author  : LJD(291483914@qq.com)
 * Desc    : WiFi SNTP 网络时间同步子任务及时间查询接口实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"
#include "esp_netif_sntp.h"

#include "WiFi/wifi_queue_task.h"

#if (boardWIFI_EN)
#include "WiFi/wifi_task.h"
#include "Print/print_task.h"

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//
static bool s_b_sntp_inited = false;

//****************************************************Function Declaration******************************************************//
static void v_sntp_sync_notification_cb(struct timeval *tv);


/***********************************************************************************************************************
 * 函数功能    : SNTP 时间同步成功底层回调函数
 * 说明(备注)  : 驱动层同步到时间后自动调用
 * 传入参数    : tv: 时间结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_sntp_sync_notification_cb(struct timeval *tv)
{
	(void)tv;
	tWiFiInfo.bSntpSynced	= true;

	time_t				now = 0;
	struct tm			timeinfo;
	time(&now);
	localtime_r(&now,	&timeinfo);

	if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
	{
		char strftime_buf[64];
		strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
		log_i("bWiFiTask:SNTP网络对时成功! 北京时间: %s", strftime_buf);
	}
}

/***********************************************************************************************************************
 * 函数功能    : WiFi SNTP 网络对时队列子任务
 * 说明(备注)  : Step 0: 初始化SNTP客户端与配置服务器; Step 1: 等待同步完成或超时
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_wifi_queue_task_sntp(Task_T *p_task)
{
	if (tWiFiInfo.eState != WIFI_STATE_CONNECTED)
	{
		cQueue_GotoStep(p_task, STEP_END);
		return;
	}

	switch (p_task->ucStep)
	{
		//************************************步骤0:配置时区与初始化SNTP客户端**********************************
		case 0:
		{
			/* 设置中国标准时间 (UTC+8) */
			setenv("TZ", "CST-8", 1);
			tzset();

			if (!s_b_sntp_inited)
			{
				esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(wifiSNTP_SERVER_MAIN);
				config.sync_cb           = v_sntp_sync_notification_cb;

				esp_err_t err = esp_netif_sntp_init(&config);
				if (err == ESP_OK)
					s_b_sntp_inited = true;
			}
			else
			{
				esp_netif_sntp_start();
			}

			if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
				log_i("bWiFiTask:发起SNTP网络对时, 服务器: [%s]...", wifiSNTP_SERVER_MAIN);

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		//************************************步骤1:等待时间同步完成或超时**************************************
		case 1:
		{
			/* 检查是否已完成同步 (系统时间年超过 2024 年) */
			time_t now = 0;
			struct tm timeinfo;
			time(&now);
			localtime_r(&now, &timeinfo);

			if (timeinfo.tm_year >= (2024 - 1900))
			{
				tWiFiInfo.bSntpSynced = true;
				cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			if (bQueue_IsStepTimeoutMs(p_task, wifiSNTP_TIMEOUT_MS))
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_w("bWiFiTask:SNTP网络对时超时, 稍后在后台重试");

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

/***********************************************************************************************************************
 * 函数功能    : 查询 SNTP 是否已成功完成对时
 * 说明(备注)  : true 表示本地时钟已校准为有效网络北京时间
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 已对时, false: 未对时
 ************************************************************************************************************************/
bool bWiFi_SntpIsSynced(void)
{
	return tWiFiInfo.bSntpSynced;
}

/***********************************************************************************************************************
 * 函数功能    : 获取当前本地北京时间 (tm 结构体)
 * 说明(备注)  : 若未完成对时则返回 false
 * 传入参数    : p_tm: 存放结果的 tm 结构体指针
 * 输出参数    : p_tm: 填充本地时间
 * 返回值      : true: 成功获得有效时间, false: 尚未对时
 ************************************************************************************************************************/
bool bWiFi_SntpGetLocalTime(struct tm *p_tm)
{
	if (p_tm == NULL || !tWiFiInfo.bSntpSynced)
		return			false;

	time_t				now = 0;
	time(&now);
	localtime_r(&now,	p_tm);
	return				true;
}

/***********************************************************************************************************************
 * 函数功能    : 格式化输出当前本地时间字符串
 * 说明(备注)  : 格式为 "YYYY-MM-DD HH:MM:SS"，若未对时则输出空字符串
 * 传入参数    : p_out_buf: 输出缓冲区指针, buf_len: 缓冲区最大容量
 * 输出参数    : p_out_buf: 填充时间字符串
 * 返回值      : 无
 ************************************************************************************************************************/
void vWiFi_SntpGetTimeStr(char *p_out_buf, size_t buf_len)
{
	if (p_out_buf == NULL || buf_len == 0)
		return;

	if (!tWiFiInfo.bSntpSynced)
	{
		p_out_buf[0] = '\0';
		return;
	}

	time_t now = 0;
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);
	strftime(p_out_buf, buf_len, "%Y-%m-%d %H:%M:%S", &timeinfo);
}

/***********************************************************************************************************************
 * 函数功能    : 主动触发一次 SNTP 网络对时
 * 说明(备注)  : 向队列投递 WTI_SNTP 任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功投递, false: 网络未就绪
 ************************************************************************************************************************/
bool bWiFi_SntpSync(void)
{
	if (tWiFiInfo.eState != WIFI_STATE_CONNECTED)
		return false;

	#if (boardUSE_OS)
	if (tpWiFiTask != NULL)
		cQueue_AddQueueTask(tpWiFiTask, WTI_SNTP, 0, false);
	#endif  /* boardUSE_OS */

	return true;
}

#endif  /* boardWIFI_EN */
