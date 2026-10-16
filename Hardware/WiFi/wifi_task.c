/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\WiFi
 * File    : wifi_task.c
 * Date    : 2026-10-14
 * Author  : LJD(291483914@qq.com)
 * Desc    : WiFi 连接管理任务实现，包含 NVS/Netif/WiFi 驱动初始化、底层事件监听与任务队列驱动引擎
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "WiFi/wifi_task.h"

#if (boardWIFI_EN)
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#if (boardEASY_FLASH)
#include "easyflash.h"
#endif  /* boardEASY_FLASH */

#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "WiFi/wifi_queue_task.h"

//****************************************************Task Declaration**********************************************************//
#if (boardUSE_OS)
#define			wifiTASK_PRIO							3		/* 任务优先级(网络管理) */
#define			wifiTASK_STK_SIZE						(1024U * 4U)	/* 任务堆栈大小 (字节) 4KB足够容纳队列驱动与底层回调 */
static TaskHandle_t s_t_wifi_task_handler = NULL;						/* 任务句柄 */
static void vWiFi_Task(void *p_v_parameters);
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
WiFiInfo_T tWiFiInfo;

//****************************************************Function Declaration******************************************************//
static bool b_task_param_init(void);
void vWiFi_EventHandler(void *p_arg, esp_event_base_t event_base, int32_t l_event_id, void *p_event_data);


/***********************************************************************************************************************
 * 函数功能    : 任务参数初始化
 * 说明(备注)  : 初始化 WiFiInfo 结构体，设置默认值
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true 成功, false 失败
 ************************************************************************************************************************/
static bool b_task_param_init(void)
{
	memset(&tWiFiInfo, 0, sizeof(WiFiInfo_T));
	tWiFiInfo.eState       = WIFI_STATE_DISCONNECTED;
	tWiFiInfo.cRssi        = -100;
	tWiFiInfo.bAutoConnect = true;
	tWiFiInfo.bPowerSaveEn = false;
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : WiFi 任务与任务队列调度初始化
 * 说明(备注)  : 仅负责全局参数清零、TaskQueue 任务队列对象创建与 FreeRTOS 任务绑定 (微秒级无感返回)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功; 负数: 对应初始化步骤故障码
 ************************************************************************************************************************/
s8 cWiFi_TaskInit(void)
{
	if(!b_task_param_init())
		return -1;

	#if (boardUSE_OS)
	/* 1. 初始化 WiFi TaskQueue 任务队列对象 */
	if (!bWiFi_QueueInit())
		return -2;

	/* 2. 创建任务队列主调度任务并绑定任务句柄 */
	if (xTaskCreate((TaskFunction_t )vWiFi_Task,
	                (const char*    )"bWiFiTask",
	                (uint16_t       )wifiTASK_STK_SIZE,
	                (void*          )NULL,
	                (UBaseType_t    )wifiTASK_PRIO,
	                (TaskHandle_t*  )&s_t_wifi_task_handler) != pdPASS)
		return -3;

	vQueue_BindTaskHandler(tpWiFiTask, s_t_wifi_task_handler);
	#endif  /* boardUSE_OS */

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 更新并下发 STA 目标连接凭据
 * 说明(备注)  : 设置 SSID、密码及工作模式至底层驱动
 * 传入参数    : p_ssid: WiFi名称, p_pwd: WiFi密码
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vWiFi_ApplyStaConfig(const char *p_ssid, const char *p_pwd)
{
	wifi_config_t wifi_cfg;
	memset(&wifi_cfg, 0, sizeof(wifi_config_t));

	if (p_ssid != NULL)
	{
		strncpy(tWiFiInfo.saSsid, p_ssid, sizeof(tWiFiInfo.saSsid) - 1);
		tWiFiInfo.saSsid[sizeof(tWiFiInfo.saSsid) - 1] = '\0';
	}
	if (p_pwd != NULL)
	{
		strncpy(tWiFiInfo.saPassword, p_pwd, sizeof(tWiFiInfo.saPassword) - 1);
		tWiFiInfo.saPassword[sizeof(tWiFiInfo.saPassword) - 1] = '\0';
	}

	strncpy((char *)wifi_cfg.sta.ssid, tWiFiInfo.saSsid, sizeof(wifi_cfg.sta.ssid));
	strncpy((char *)wifi_cfg.sta.password, tWiFiInfo.saPassword, sizeof(wifi_cfg.sta.password));
	wifi_cfg.sta.threshold.authmode = (strlen(tWiFiInfo.saPassword) > 0) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;
	wifi_cfg.sta.pmf_cfg.capable    = true;
	wifi_cfg.sta.pmf_cfg.required   = false;

	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
}

/***********************************************************************************************************************
 * 函数功能    : WiFi 任务队列主调度循环
 * 说明(备注)  : 标准 TaskQueue 引擎驱动，空闲时自动进入轻量睡眠并在事件到达时即刻唤醒
 * 传入参数    : p_v_parameters: 任务创建参数指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vWiFi_Task(void *p_v_parameters)
{
	(void)p_v_parameters;

	#if (boardUSE_OS)
	for (;;)
	{
		vQueue_TaskPoll(tpWiFiTask, wifiTASK_CYCLE_TIME);
	}
	#endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 动态修改并重新连接指定 WiFi 热点
 * 说明(备注)  : 自动持久化凭据至 EasyFlash，更新配置并向队列投递 WTI_CONNECT 抢占任务
 * 传入参数    : p_ssid: 目标 SSID, p_pwd: 目标密码
 * 输出参数    : 无
 * 返回值      : true: 成功触发连接流程, false: 参数无效
 ************************************************************************************************************************/
bool bWiFi_Connect(const char *p_ssid, const char *p_pwd)
{
	if (p_ssid == NULL || strlen(p_ssid) == 0)
		return false;

	/* 持久化保存新凭据至 EasyFlash */
	bWiFi_SaveConfig(p_ssid, p_pwd);

	vWiFi_ApplyStaConfig(p_ssid, p_pwd);
	tWiFiInfo.ucRetryCnt = 0;

	#if (boardUSE_OS)
	if (tpWiFiTask != NULL)
		cQueue_AddQueueTask(tpWiFiTask, WTI_CONNECT, 0, true);
	#endif  /* boardUSE_OS */

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 断开当前 WiFi 链路
 * 说明(备注)  : 向队列投递 WTI_DISCONNECT 任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 驱动未就绪
 ************************************************************************************************************************/
bool bWiFi_Disconnect(void)
{
	if (tSysInfo.uInit.tFinish.bIF_WiFiTask == 0)
		return false;

	#if (boardUSE_OS)
	if (tpWiFiTask != NULL)
		cQueue_AddQueueTask(tpWiFiTask, WTI_DISCONNECT, 0, true);
	#endif  /* boardUSE_OS */

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 查询当前 WiFi 是否处于网络就绪连接态
 * 说明(备注)  : 供 UI 或业务模块快速读取网络状态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 已连接且有IP, false: 未连接
 ************************************************************************************************************************/
bool bWiFi_IsConnected(void)
{
	return (tWiFiInfo.eState == WIFI_STATE_CONNECTED);
}

/***********************************************************************************************************************
 * 函数功能    : 获取当前 WiFi 信号强度 (RSSI)
 * 说明(备注)  : 单位 dBm (-100 ~ 0)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 信号强度值
 ************************************************************************************************************************/
int8_t cWiFi_GetRssi(void)
{
	return tWiFiInfo.cRssi;
}

/***********************************************************************************************************************
 * 函数功能    : 复制当前分配到的 IP 地址字符串
 * 说明(备注)  : 若未就绪则输出空字符串
 * 传入参数    : p_out_buf: 输出缓冲, buf_len: 缓冲容量
 * 输出参数    : p_out_buf: 填充 IP 字符串
 * 返回值      : 无
 ************************************************************************************************************************/
void vWiFi_GetIpStr(char *p_out_buf, size_t buf_len)
{
	if (p_out_buf == NULL || buf_len == 0)
		return;

	strncpy(p_out_buf, tWiFiInfo.saIpAddr, buf_len - 1);
	p_out_buf[buf_len - 1] = '\0';
}

/***********************************************************************************************************************
 * 函数功能    : 停用 WiFi 射频以降低功耗 (关机休眠协同)
 * 说明(备注)  : 向队列投递 WTI_STOP 任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vWiFi_Stop(void)
{
	#if (boardUSE_OS)
	if (tpWiFiTask != NULL)
		cQueue_AddQueueTask(tpWiFiTask, WTI_STOP, 0, true);
	#endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 从 EasyFlash 环境变量载入 WiFi 配置凭据
 * 说明(备注)  : 若 EasyFlash 中无配置，则使用出厂宏默认值并写入 Flash 做初始持久化
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功载入已有配置; false: 未找到已有配置，已初次写入默认配置
 ************************************************************************************************************************/
bool bWiFi_LoadConfig(void)
{
	#if (boardEASY_FLASH)
	char *p_ssid = ef_get_env(wifiENV_KEY_SSID);
	char *p_pwd  = ef_get_env(wifiENV_KEY_PWD);
	char *p_auto = ef_get_env(wifiENV_KEY_AUTO);

	/* 校验是否具有有效热点配置 (过滤历史残留的测试值 "1") */
	if (p_ssid != NULL && strlen(p_ssid) > 0 && strcmp(p_ssid, "1") != 0)
	{
		strncpy(tWiFiInfo.saSsid, p_ssid, sizeof(tWiFiInfo.saSsid) - 1);
		tWiFiInfo.saSsid[sizeof(tWiFiInfo.saSsid) - 1] = '\0';

		if (p_pwd != NULL)
		{
			strncpy(tWiFiInfo.saPassword, p_pwd, sizeof(tWiFiInfo.saPassword) - 1);
			tWiFiInfo.saPassword[sizeof(tWiFiInfo.saPassword) - 1] = '\0';
		}
		else
		{
			tWiFiInfo.saPassword[0] = '\0';
		}

		if (p_auto != NULL)
			tWiFiInfo.bAutoConnect = (p_auto[0] != '0');
		else
			tWiFiInfo.bAutoConnect = true;

		if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
			log_i("bWiFiTask:从EasyFlash载入WiFi配置: SSID=[%s], AutoConnect=%d", tWiFiInfo.saSsid, (int)tWiFiInfo.bAutoConnect);

		return true;
	}
	else
	{
		/* 若无有效配置或为历史异常残留值("1")，重置并持久化为出厂宏默认配置 */
		strncpy(tWiFiInfo.saSsid, wifiDEFAULT_SSID, sizeof(tWiFiInfo.saSsid) - 1);
		tWiFiInfo.saSsid[sizeof(tWiFiInfo.saSsid) - 1] = '\0';
		strncpy(tWiFiInfo.saPassword, wifiDEFAULT_PWD, sizeof(tWiFiInfo.saPassword) - 1);
		tWiFiInfo.saPassword[sizeof(tWiFiInfo.saPassword) - 1] = '\0';
		tWiFiInfo.bAutoConnect = true;

		ef_set_env(wifiENV_KEY_SSID, wifiDEFAULT_SSID);
		ef_set_env(wifiENV_KEY_PWD, wifiDEFAULT_PWD);
		ef_set_env(wifiENV_KEY_AUTO, "1");

		if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
			log_i("bWiFiTask:未检测到有效WiFi配置(原为[%s]), 重新写入默认配置: SSID=[%s]", (p_ssid != NULL) ? p_ssid : "NULL", wifiDEFAULT_SSID);

		return false;
	}
	#else
	strncpy(tWiFiInfo.saSsid, wifiDEFAULT_SSID, sizeof(tWiFiInfo.saSsid) - 1);
	tWiFiInfo.saSsid[sizeof(tWiFiInfo.saSsid) - 1] = '\0';
	strncpy(tWiFiInfo.saPassword, wifiDEFAULT_PWD, sizeof(tWiFiInfo.saPassword) - 1);
	tWiFiInfo.saPassword[sizeof(tWiFiInfo.saPassword) - 1] = '\0';
	tWiFiInfo.bAutoConnect = true;

	if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
		log_i("bWiFiTask:载入宏默认WiFi配置: SSID=[%s]", wifiDEFAULT_SSID);

	return true;
	#endif  /* boardEASY_FLASH */
}

/***********************************************************************************************************************
 * 函数功能    : 保存 WiFi 目标连接凭据至 EasyFlash 环境变量
 * 说明(备注)  : 同时更新全局状态结构体 tWiFiInfo 与 Flash 存储
 * 传入参数    : p_ssid: 目标 SSID, p_pwd: 目标密码 (可为 NULL/空字符串 表示开放热点)
 * 输出参数    : 无
 * 返回值      : true: 保存成功, false: 参数无效
 ************************************************************************************************************************/
bool bWiFi_SaveConfig(const char *p_ssid, const char *p_pwd)
{
	if (p_ssid == NULL || strlen(p_ssid) == 0)
		return false;

	strncpy(tWiFiInfo.saSsid, p_ssid, sizeof(tWiFiInfo.saSsid) - 1);
	tWiFiInfo.saSsid[sizeof(tWiFiInfo.saSsid) - 1] = '\0';

	if (p_pwd != NULL)
	{
		strncpy(tWiFiInfo.saPassword, p_pwd, sizeof(tWiFiInfo.saPassword) - 1);
		tWiFiInfo.saPassword[sizeof(tWiFiInfo.saPassword) - 1] = '\0';
	}
	else
	{
		tWiFiInfo.saPassword[0] = '\0';
	}

	#if (boardEASY_FLASH)
	ef_set_env(wifiENV_KEY_SSID, tWiFiInfo.saSsid);
	ef_set_env(wifiENV_KEY_PWD, tWiFiInfo.saPassword);
	ef_set_env(wifiENV_KEY_AUTO, tWiFiInfo.bAutoConnect ? "1" : "0");

	if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
		log_i("bWiFiTask:WiFi凭据已持久化至EasyFlash: SSID=[%s]", tWiFiInfo.saSsid);
	#endif  /* boardEASY_FLASH */

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 恢复 WiFi 目标热点与配置为出厂默认值
 * 说明(备注)  : 重置 EasyFlash 键值并重新应用底层驱动 STA 参数
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功恢复
 ************************************************************************************************************************/
bool bWiFi_RestoreDefaultConfig(void)
{
	strncpy(tWiFiInfo.saSsid, wifiDEFAULT_SSID, sizeof(tWiFiInfo.saSsid) - 1);
	tWiFiInfo.saSsid[sizeof(tWiFiInfo.saSsid) - 1] = '\0';
	strncpy(tWiFiInfo.saPassword, wifiDEFAULT_PWD, sizeof(tWiFiInfo.saPassword) - 1);
	tWiFiInfo.saPassword[sizeof(tWiFiInfo.saPassword) - 1] = '\0';
	tWiFiInfo.bAutoConnect = true;

	#if (boardEASY_FLASH)
	ef_set_env(wifiENV_KEY_SSID, wifiDEFAULT_SSID);
	ef_set_env(wifiENV_KEY_PWD, wifiDEFAULT_PWD);
	ef_set_env(wifiENV_KEY_AUTO, "1");

	if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
		log_i("bWiFiTask:WiFi配置已恢复为出厂默认值: SSID=[%s]", wifiDEFAULT_SSID);
	#endif  /* boardEASY_FLASH */

	vWiFi_ApplyStaConfig(tWiFiInfo.saSsid, tWiFiInfo.saPassword);
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置是否启用开机自动连接
 * 说明(备注)  : 同步更新 EasyFlash 存储
 * 传入参数    : b_enable: true: 自动连接, false: 禁用自动连接
 * 输出参数    : 无
 * 返回值      : true: 设置成功
 ************************************************************************************************************************/
bool bWiFi_SetAutoConnect(bool b_enable)
{
	tWiFiInfo.bAutoConnect = b_enable;

	#if (boardEASY_FLASH)
	ef_set_env(wifiENV_KEY_AUTO, b_enable ? "1" : "0");
	#endif  /* boardEASY_FLASH */

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 配置 WiFi 调制解调器休眠节能模式 (Modem Sleep)
 * 说明(备注)  : 启用后空闲时射频关闭，到 DTIM 时隙自动唤醒，大幅降低功耗与发热
 * 传入参数    : b_enable: true: 开启 WIFI_PS_MIN_MODEM; false: 关闭 (WIFI_PS_NONE)
 * 输出参数    : 无
 * 返回值      : true: 设置成功; false: 底层接口返回错误
 ************************************************************************************************************************/
bool bWiFi_SetPowerSave(bool b_enable)
{
	wifi_ps_type_t ps_type = b_enable ? WIFI_PS_MIN_MODEM : WIFI_PS_NONE;
	esp_err_t err = esp_wifi_set_ps(ps_type);
	if (err == ESP_OK)
	{
		tWiFiInfo.bPowerSaveEn = b_enable;
		if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
			log_i("bWiFiTask:WiFi节能模式设置成功: %s", b_enable ? "Modem Sleep (MIN_MODEM)" : "关闭 (NONE)");
		return true;
	}
	else
	{
		if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
			log_w("bWiFiTask:WiFi节能模式设置失败: %s", esp_err_to_name(err));
		return false;
	}
}

/***********************************************************************************************************************
 * 函数功能    : 查询当前是否已启用 Modem Sleep 节能模式
 * 说明(备注)  : none
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 已启用; false: 未启用
 ************************************************************************************************************************/
bool bWiFi_IsPowerSaveEnabled(void)
{
	return tWiFiInfo.bPowerSaveEn;
}

/***********************************************************************************************************************
 * 函数功能    : 获取底层 WiFi 断开原因描述文本
 * 说明(备注)  : 将 ESP-IDF wifi_err_reason_t 原因码转换为易读字符串
 * 传入参数    : reason: 断开原因码 (uint8_t)
 * 输出参数    : 无
 * 返回值      : 错误描述字符串指针
 ************************************************************************************************************************/
static const char *prv_pcGetDisconnectReasonDesc(uint8_t reason)
{
	switch (reason)
	{
		case 1:   return "UNSPECIFIED";
		case 2:   return "AUTH_EXPIRE";
		case 3:   return "AUTH_LEAVE";
		case 4:   return "ASSOC_EXPIRE";
		case 5:   return "ASSOC_TOOMANY";
		case 6:   return "NOT_AUTHED";
		case 7:   return "NOT_ASSOCED";
		case 8:   return "ASSOC_LEAVE";
		case 13:  return "IE_IN_4WAY_DIFFERS";
		case 15:  return "4WAY_HANDSHAKE_TIMEOUT";
		case 200: return "BEACON_TIMEOUT";
		case 201: return "NO_AP_FOUND (未找到该WiFi热点或信号弱/非2.4G频段)";
		case 202: return "AUTH_FAIL (密码错误或加密模式不匹配)";
		case 203: return "ASSOC_FAIL";
		case 204: return "HANDSHAKE_TIMEOUT (握手超时)";
		case 205: return "CONNECTION_FAIL";
		default:  return "UNKNOWN";
	}
}

/***********************************************************************************************************************
 * 函数功能    : ESP-IDF WiFi 与 IP 事件回调响应函数
 * 说明(备注)  : 运行于 esp_sys_loop 任务上下文中，将底层网络事件转换为 TaskQueue 队列任务
 * 传入参数    : p_arg: 用户上下文指针, event_base: 事件基类别, l_event_id: 事件ID, p_event_data: 事件携带数据指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vWiFi_EventHandler(void *p_arg, esp_event_base_t event_base, int32_t l_event_id, void *p_event_data)
{
	(void)p_arg;

	#if (boardUSE_OS)
	if (tpWiFiTask == NULL)
		return;
	#endif  /* boardUSE_OS */

	if (event_base == WIFI_EVENT)
	{
		switch (l_event_id)
		{
			case WIFI_EVENT_STA_START:
			{
				#if (boardUSE_OS)
				if (tWiFiInfo.bAutoConnect)
				{
					cQueue_AddQueueTask(tpWiFiTask, WTI_CONNECT, 0, true);
				}
				else
				{
					if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
						log_i("bWiFiTask:开机自动连接已禁用, 保持待机模式");
				}
				#endif  /* boardUSE_OS */
			}
			break;

			case WIFI_EVENT_STA_CONNECTED:
			{
				if (uPrint.tFlag.bWiFiTask)
					log_i("bWiFiTask:链路已接通，正在获取IP...");
			}
			break;

			case WIFI_EVENT_STA_DISCONNECTED:
			{
				if (tWiFiInfo.eState != WIFI_STATE_STOPPED)
				{
					wifi_event_sta_disconnected_t *p_dis = (wifi_event_sta_disconnected_t *)p_event_data;
					uint8_t uc_reason = (p_dis != NULL) ? p_dis->reason : 0;

					tWiFiInfo.eState = WIFI_STATE_DISCONNECTED;
					tWiFiInfo.cRssi  = -100;
					tWiFiInfo.ucRetryCnt++;

					if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
						log_w("bWiFiTask:WiFi断开连接, 原因码: %u (%s)", (unsigned int)uc_reason, prv_pcGetDisconnectReasonDesc(uc_reason));

					#if (boardUSE_OS)
					if (tWiFiInfo.bAutoConnect)
						cQueue_AddQueueTask(tpWiFiTask, WTI_RECONNECT, 0, true);
					#endif  /* boardUSE_OS */
				}
			}
			break;

			case WIFI_EVENT_SCAN_DONE:
			{
				vWiFi_ScanDoneNotify();
				#if (boardUSE_OS)
				if (s_t_wifi_task_handler != NULL)
					xTaskNotifyGive(s_t_wifi_task_handler);
				#endif  /* boardUSE_OS */
			}
			break;

			default:
			{
			}
			break;
		}
	}
	else if (event_base == IP_EVENT)
	{
		if (l_event_id == IP_EVENT_STA_GOT_IP)
		{
			ip_event_got_ip_t *p_event = (ip_event_got_ip_t *)p_event_data;
			snprintf(tWiFiInfo.saIpAddr, sizeof(tWiFiInfo.saIpAddr), IPSTR, IP2STR(&p_event->ip_info.ip));

			tWiFiInfo.eState     = WIFI_STATE_CONNECTED;
			tWiFiInfo.ucRetryCnt = 0;

			wifi_ap_record_t ap_info;
			if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
				tWiFiInfo.cRssi = ap_info.rssi;

			/* 网络连接成功后自动激活 Modem Sleep 节能模式 */
			bWiFi_SetPowerSave(true);

			if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
				log_i("bWiFiTask:网络就绪! IP: %s, RSSI: %d dBm, Modem Sleep 节能模式已激活", tWiFiInfo.saIpAddr, (int)tWiFiInfo.cRssi);

			#if (boardUSE_OS)
			cQueue_AddQueueTask(tpWiFiTask, WTI_WORK, 0, true);
			#endif  /* boardUSE_OS */
		}
	}
}
#endif  /* boardWIFI_EN */
