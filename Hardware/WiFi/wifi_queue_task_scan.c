/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\WiFi
 * File    : wifi_queue_task_scan.c
 * Date    : 2026-10-15
 * Author  : LJD(291483914@qq.com)
 * Desc    : WiFi 周边热点异步扫描队列子任务及热点查询接口实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "WiFi/wifi_queue_task.h"

#if (boardWIFI_EN)
#include <string.h>
#include <stdlib.h>
#include "WiFi/wifi_task.h"
#include "Print/print_task.h"
#include "esp_wifi.h"

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//
static wifi_ap_record_t s_ta_raw_scan_records[32];	/* 静态扫描接收暂存区，避免任务堆栈溢出 */
static volatile bool s_b_scan_event_done = false;	/* 底层扫描完成事件接收标志 */

//****************************************************Function Declaration******************************************************//
static void v_wifi_sort_scan_records(wifi_ap_record_t *p_records, uint16_t count);


/***********************************************************************************************************************
 * 函数功能    : 底层 WiFi 扫描完成事件通知
 * 说明(备注)  : 供 v_wifi_event_handler 在捕获到 WIFI_EVENT_SCAN_DONE 时调用
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vWiFi_ScanDoneNotify(void)
{
	s_b_scan_event_done = true;
}

/***********************************************************************************************************************
 * 函数功能    : 对扫描到的 AP 记录按信号强度 (RSSI) 降序排列
 * 说明(备注)  : 简单的就地选择排序，热点数量小（<=32）耗时极低
 * 传入参数    : p_records: AP 记录数组指针, count: 记录条数
 * 输出参数    : p_records: 排序后的 AP 记录数组
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_wifi_sort_scan_records(wifi_ap_record_t *p_records, uint16_t count)
{
	if (p_records == NULL || count <= 1)
		return;

	for (uint16_t i = 0; i < count; i++)
	{
		for (uint16_t j = i + 1; j < count; j++)
		{
			if (p_records[j].rssi > p_records[i].rssi)
			{
				wifi_ap_record_t t_tmp = p_records[i];
				p_records[i]           = p_records[j];
				p_records[j]           = t_tmp;
			}
		}
	}
}

/***********************************************************************************************************************
 * 函数功能    : WiFi 周边热点扫描队列子任务
 * 说明(备注)  : Step 0: 启动异步全信道主动扫描; Step 1: 等待扫描完成事件并解析排序
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_wifi_queue_task_scan(Task_T *p_task)
{
	/* 若队列有抢占新任务（如关机/断开），及时中止扫描让出执行权 */
	if (lwrb_get_full(&p_task->tQueueBuff) > 0)
	{
		esp_wifi_scan_stop();
		tWiFiInfo.tScan.bScanning = false;
		cQueue_GotoStep(p_task, STEP_END);
		return;
	}

	switch (p_task->ucStep)
	{
		//************************************步骤0:启动异步全信道主动扫描**************************************
		case 0:
		{
			if (tWiFiInfo.eState == WIFI_STATE_STOPPED)
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_w("bWiFiTask:WiFi射频已停用, 无法发起扫描");

				cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			wifi_scan_config_t scan_cfg = {
				.ssid        = NULL,
				.bssid       = NULL,
				.channel     = 0,
				.show_hidden = false,
				.scan_type   = WIFI_SCAN_TYPE_ACTIVE,
				.scan_time   = {
					.active = {
						.min = 100,
						.max = 300,
					},
				},
			};

			s_b_scan_event_done       = false;
			tWiFiInfo.tScan.bScanning = true;
			tWiFiInfo.tScan.bScanDone = false;

			esp_err_t err = esp_wifi_scan_start(&scan_cfg, false);
			if (err != ESP_OK)
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_w("bWiFiTask:启动WiFi扫描失败: %s", esp_err_to_name(err));

				tWiFiInfo.tScan.bScanning = false;
				cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
				log_i("bWiFiTask:发起全信道热点扫描...");

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		//************************************步骤1:等待底层扫描完成事件并解析**********************************
		case 1:
		{
			if (s_b_scan_event_done)
			{
				uint16_t us_ap_num = 0;
				esp_wifi_scan_get_ap_num(&us_ap_num);
				tWiFiInfo.tScan.usTotalApNum = us_ap_num;

				uint16_t us_read_num = us_ap_num > 32 ? 32 : us_ap_num;
				if (us_read_num > 0)
				{
					esp_wifi_scan_get_ap_records(&us_read_num, s_ta_raw_scan_records);
					v_wifi_sort_scan_records(s_ta_raw_scan_records, us_read_num);

					uint16_t us_cache_num = us_read_num > wifiSCAN_MAX_AP_NUM ? wifiSCAN_MAX_AP_NUM : us_read_num;
					tWiFiInfo.tScan.usCachedApNum = us_cache_num;

					for (uint16_t i = 0; i < us_cache_num; i++)
					{
						strncpy(tWiFiInfo.tScan.taApList[i].saSsid, (char *)s_ta_raw_scan_records[i].ssid, sizeof(tWiFiInfo.tScan.taApList[i].saSsid) - 1);
						tWiFiInfo.tScan.taApList[i].saSsid[sizeof(tWiFiInfo.tScan.taApList[i].saSsid) - 1] = '\0';
						tWiFiInfo.tScan.taApList[i].cRssi      = s_ta_raw_scan_records[i].rssi;
						tWiFiInfo.tScan.taApList[i].ucChannel  = s_ta_raw_scan_records[i].primary;
						tWiFiInfo.tScan.taApList[i].ucAuthMode = (uint8_t)s_ta_raw_scan_records[i].authmode;
					}
				}
				else
					tWiFiInfo.tScan.usCachedApNum = 0;

				tWiFiInfo.tScan.bScanning = false;
				tWiFiInfo.tScan.bScanDone = true;

				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
				{
					log_i("bWiFiTask:热点扫描完成, 共发现 %u 个AP, 已缓存前 %u 个强信号热点:", (unsigned int)tWiFiInfo.tScan.usTotalApNum, (unsigned int)tWiFiInfo.tScan.usCachedApNum);
					for (uint16_t i = 0; i < tWiFiInfo.tScan.usCachedApNum; i++)
					{
						log_i("  [%2u] SSID: %-24s RSSI: %3d dBm  CH: %2u  AUTH: %s",
						      (unsigned int)(i + 1),
						      tWiFiInfo.tScan.taApList[i].saSsid,
						      (int)tWiFiInfo.tScan.taApList[i].cRssi,
						      (unsigned int)tWiFiInfo.tScan.taApList[i].ucChannel,
						      pcWiFi_AuthModeToDesc(tWiFiInfo.tScan.taApList[i].ucAuthMode));
					}
				}

				cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			if (bQueue_IsStepTimeoutMs(p_task, wifiSCAN_TIMEOUT_MS))
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_w("bWiFiTask:等待热点扫描超时");

				esp_wifi_scan_stop();
				tWiFiInfo.tScan.bScanning = false;
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
 * 函数功能    : 主动触发一次周边 WiFi 热点扫描
 * 说明(备注)  : 向 WiFi 队列投递 WTI_SCAN 任务，扫描完成后结果保存在 tWiFiInfo.tScan 中
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功投递扫描任务, false: 驱动未就绪或正在扫描中
 ************************************************************************************************************************/
bool bWiFi_StartScan(void)
{
	if (tWiFiInfo.eState == WIFI_STATE_STOPPED)
		return false;

	if (tWiFiInfo.tScan.bScanning)
		return false;

	#if (boardUSE_OS)
	if (tpWiFiTask != NULL)
	{
		cQueue_AddQueueTask(tpWiFiTask, WTI_SCAN, 0, false);
		return true;
	}
	#endif  /* boardUSE_OS */

	return false;
}

/***********************************************************************************************************************
 * 函数功能    : 查询当前是否正处于热点扫描过程中
 * 说明(备注)  : 供 UI 或业务模块查询扫描忙碌状态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 正在扫描中, false: 空闲/未扫描
 ************************************************************************************************************************/
bool bWiFi_IsScanning(void)
{
	return tWiFiInfo.tScan.bScanning;
}

/***********************************************************************************************************************
 * 函数功能    : 查询最近一次热点扫描是否已成功完成
 * 说明(备注)  : 供 UI 或业务模块判断是否有可用的最新扫描结果
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 扫描已完成且结果有效, false: 未扫描或上次扫描失败/超时
 ************************************************************************************************************************/
bool bWiFi_IsScanDone(void)
{
	return tWiFiInfo.tScan.bScanDone;
}

/***********************************************************************************************************************
 * 函数功能    : 拷贝获取最近一次扫描缓存的热点列表
 * 说明(备注)  : 线程安全拷贝，结果已按信号由强到弱排序
 * 传入参数    : p_out_list: 接收热点数组指针, max_count: 接收数组最大容量
 * 输出参数    : p_out_list: 填充热点信息
 * 返回值      : 实际拷贝的热点条数 (<= max_count 且 <= tWiFiInfo.tScan.usCachedApNum)
 ************************************************************************************************************************/
uint16_t usWiFi_GetScanResult(WiFiScanApItem_T *p_out_list, uint16_t max_count)
{
	if (p_out_list == NULL || max_count == 0 || !tWiFiInfo.tScan.bScanDone)
		return 0;

	uint16_t count = tWiFiInfo.tScan.usCachedApNum;
	if (count > max_count)
		count = max_count;

	memcpy(p_out_list, tWiFiInfo.tScan.taApList, count * sizeof(WiFiScanApItem_T));
	return count;
}

/***********************************************************************************************************************
 * 函数功能    : 将 WiFi 认证加密模式转换为可读字符串描述
 * 说明(备注)  : 供 UI 列表显示或日志打印热点加密类型
 * 传入参数    : uc_auth_mode: wifi_auth_mode_t 枚举值
 * 输出参数    : 无
 * 返回值      : 加密描述字符串常量指针 (如 "OPEN", "WPA2-PSK", "WPA3-PSK")
 ************************************************************************************************************************/
const char *pcWiFi_AuthModeToDesc(uint8_t uc_auth_mode)
{
	switch ((wifi_auth_mode_t)uc_auth_mode)
	{
		case WIFI_AUTH_OPEN:
		{
			return "OPEN";
		}

		case WIFI_AUTH_WEP:
		{
			return "WEP";
		}

		case WIFI_AUTH_WPA_PSK:
		{
			return "WPA-PSK";
		}

		case WIFI_AUTH_WPA2_PSK:
		{
			return "WPA2-PSK";
		}

		case WIFI_AUTH_WPA_WPA2_PSK:
		{
			return "WPA/WPA2-PSK";
		}

		case WIFI_AUTH_ENTERPRISE:
		{
			return "WPA2-Enterprise";
		}

		case WIFI_AUTH_WPA3_PSK:
		{
			return "WPA3-PSK";
		}

		case WIFI_AUTH_WPA2_WPA3_PSK:
		{
			return "WPA2/WPA3-PSK";
		}

		case WIFI_AUTH_WAPI_PSK:
		{
			return "WAPI-PSK";
		}

		case WIFI_AUTH_OWE:
		{
			return "OWE";
		}

		default:
		{
			return "UNKNOWN";
		}
	}
}

#endif  /* boardWIFI_EN */

