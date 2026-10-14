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

#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "WiFi/wifi_queue_task.h"

//****************************************************Macros********************************************************************//
#if (boardUSE_OS)
#define			wifiTASK_PRIO							3		/* 任务优先级(网络管理) */
#define			wifiTASK_STK_SIZE						(1024U * 4U)	/* 任务堆栈大小 (字节) */
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
#if (boardUSE_OS)
static TaskHandle_t s_t_wifi_task_handler = NULL;						/* 任务句柄 */
#endif  /* boardUSE_OS */

WiFiInfo_T tWiFiInfo;

static esp_netif_t *s_p_sta_netif = NULL;
static esp_event_handler_instance_t s_wifi_event_instance = NULL;
static esp_event_handler_instance_t s_ip_event_instance = NULL;
static bool s_b_wifi_driver_inited = false;

//****************************************************Function Declaration******************************************************//
static void v_wifi_event_handler(void *p_arg, esp_event_base_t event_base, int32_t l_event_id, void *p_event_data);


/***********************************************************************************************************************
 * 函数功能    : WiFi 任务与底层网络协议栈初始化
 * 说明(备注)  : 仅负责 NVS、Netif、默认事件循环、WiFi 硬件驱动配置及 OS 任务队列创建绑定
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功; 负数: 对应初始化步骤故障码
 ************************************************************************************************************************/
s8 cWiFi_TaskInit(void)
{
	esp_err_t err = ESP_OK;

	memset(&tWiFiInfo, 0, sizeof(WiFiInfo_T));
	tWiFiInfo.eState = WIFI_STATE_DISCONNECTED;
	tWiFiInfo.cRssi  = -100;
	strncpy(tWiFiInfo.saSsid, wifiDEFAULT_SSID, sizeof(tWiFiInfo.saSsid) - 1);
	strncpy(tWiFiInfo.saPassword, wifiDEFAULT_PWD, sizeof(tWiFiInfo.saPassword) - 1);

	/* 1. NVS 初始化 (WiFi 依赖 NVS 保存 RF 校准参数) */
	err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		nvs_flash_erase();
		err = nvs_flash_init();
	}
	if (err != ESP_OK)
		return -1;

	/* 2. TCP/IP 协议栈基础层与事件循环初始化 */
	err = esp_netif_init();
	if (err != ESP_OK)
		return -2;

	err = esp_event_loop_create_default();
	if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
		return -3;

	/* 3. 创建默认 STA 网络接口句柄 */
	if (s_p_sta_netif == NULL)
	{
		s_p_sta_netif = esp_netif_create_default_wifi_sta();
		if (s_p_sta_netif == NULL)
			return -4;
	}

	/* 4. WiFi 硬件驱动初始化 (轻量化缓冲配置，适配片内SRAM) */
	if (!s_b_wifi_driver_inited)
	{
		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		cfg.static_rx_buf_num  = 4;
		cfg.dynamic_rx_buf_num = 16;
		cfg.dynamic_tx_buf_num = 16;
		cfg.mgmt_sbuf_num      = 16;
		err = esp_wifi_init(&cfg);
		if (err != ESP_OK)
			return -5;

		s_b_wifi_driver_inited = true;
	}

	#if (boardUSE_OS)
	/* 5. 初始化 WiFi TaskQueue 任务队列对象 */
	if (!bWiFi_QueueInit())
		return -6;

	/* 6. 创建任务队列主调度任务并绑定任务句柄 */
	if (xTaskCreate((TaskFunction_t )vWiFi_Task,
	                (const char*    )"bWiFiTask",
	                (uint16_t       )wifiTASK_STK_SIZE,
	                (void*          )NULL,
	                (UBaseType_t    )wifiTASK_PRIO,
	                (TaskHandle_t*  )&s_t_wifi_task_handler) != pdPASS)
		return -7;

	vQueue_BindTaskHandler(tpWiFiTask, s_t_wifi_task_handler);
	#endif  /* boardUSE_OS */

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 注册 WiFi 与 IP 事件监听
 * 说明(备注)  : 供 v_wifi_queue_task_init 在任务启动步骤中安全调用
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功; false: 失败
 ************************************************************************************************************************/
bool bWiFi_RegisterEvents(void)
{
	esp_err_t err = ESP_OK;

	if (s_wifi_event_instance == NULL)
	{
		err = esp_event_handler_instance_register(WIFI_EVENT,
		                                          ESP_EVENT_ANY_ID,
		                                          &v_wifi_event_handler,
		                                          NULL,
		                                          &s_wifi_event_instance);
		if (err != ESP_OK)
			return false;
	}

	if (s_ip_event_instance == NULL)
	{
		err = esp_event_handler_instance_register(IP_EVENT,
		                                          IP_EVENT_STA_GOT_IP,
		                                          &v_wifi_event_handler,
		                                          NULL,
		                                          &s_ip_event_instance);
		if (err != ESP_OK)
			return false;
	}

	return true;
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
	wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
	wifi_cfg.sta.pmf_cfg.capable    = true;
	wifi_cfg.sta.pmf_cfg.required   = false;

	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
}

/***********************************************************************************************************************
 * 函数功能    : 启动 WiFi 底层射频驱动
 * 说明(备注)  : 供 v_wifi_queue_task_init 在配置完成后启动射频
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功; false: 失败
 ************************************************************************************************************************/
bool bWiFi_StartDriver(void)
{
	return (esp_wifi_start() == ESP_OK);
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
 * 说明(备注)  : 更新配置并向队列投递 WTI_CONNECT 抢占任务
 * 传入参数    : p_ssid: 目标 SSID, p_pwd: 目标密码
 * 输出参数    : 无
 * 返回值      : true: 成功触发连接流程, false: 参数无效
 ************************************************************************************************************************/
bool bWiFi_Connect(const char *p_ssid, const char *p_pwd)
{
	if (p_ssid == NULL || strlen(p_ssid) == 0)
		return false;

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
	if (!s_b_wifi_driver_inited)
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
 * 函数功能    : ESP-IDF WiFi 与 IP 事件回调响应函数
 * 说明(备注)  : 运行于 esp_sys_loop 任务上下文中，将底层网络事件转换为 TaskQueue 队列任务
 * 传入参数    : p_arg: 用户上下文指针, event_base: 事件基类别, l_event_id: 事件ID, p_event_data: 事件携带数据指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_wifi_event_handler(void *p_arg, esp_event_base_t event_base, int32_t l_event_id, void *p_event_data)
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
				cQueue_AddQueueTask(tpWiFiTask, WTI_CONNECT, 0, true);
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
					tWiFiInfo.eState = WIFI_STATE_DISCONNECTED;
					tWiFiInfo.cRssi  = -100;
					tWiFiInfo.ucRetryCnt++;

					#if (boardUSE_OS)
					cQueue_AddQueueTask(tpWiFiTask, WTI_RECONNECT, 0, true);
					#endif  /* boardUSE_OS */
				}
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

			if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
				log_i("bWiFiTask:网络就绪! IP: %s, RSSI: %d dBm", tWiFiInfo.saIpAddr, (int)tWiFiInfo.cRssi);

			#if (boardUSE_OS)
			cQueue_AddQueueTask(tpWiFiTask, WTI_WORK, 0, true);
			#endif  /* boardUSE_OS */
		}
	}
}
#endif  /* boardWIFI_EN */
