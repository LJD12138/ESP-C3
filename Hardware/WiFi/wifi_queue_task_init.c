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
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//
static esp_netif_t *s_p_sta_netif = NULL;
static esp_event_handler_instance_t s_wifi_event_instance = NULL;
static esp_event_handler_instance_t s_ip_event_instance = NULL;
static bool s_b_wifi_driver_inited = false;

//****************************************************Function Declaration******************************************************//
extern void vWiFi_EventHandler(void *p_arg, esp_event_base_t event_base, int32_t l_event_id, void *p_event_data);

static bool prv_bInitNvs(void);
static bool prv_bInitNetif(void);
static bool prv_bInitDriver(void);
static bool prv_bRegisterEvents(void);
static bool prv_bStartDriver(void);


/***********************************************************************************************************************
 * 函数功能    : 初始化 NVS Flash 存储
 * 说明(备注)  : WiFi 硬件必须依赖 NVS 保存 RF 校准参数与底层状态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功; false: 失败
 ************************************************************************************************************************/
static bool prv_bInitNvs(void)
{
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		nvs_flash_erase();
		err = nvs_flash_init();
	}
	return (err == ESP_OK);
}

/***********************************************************************************************************************
 * 函数功能    : 初始化 TCP/IP 网络协议栈与事件循环
 * 说明(备注)  : 初始化 esp_netif、默认事件循环并创建默认 STA 网卡接口
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功; false: 失败
 ************************************************************************************************************************/
static bool prv_bInitNetif(void)
{
	esp_err_t err = esp_netif_init();
	if (err != ESP_OK)
		return false;

	err = esp_event_loop_create_default();
	if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
		return false;

	if (s_p_sta_netif == NULL)
	{
		s_p_sta_netif = esp_netif_create_default_wifi_sta();
		if (s_p_sta_netif == NULL)
			return false;
	}

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 初始化 WiFi 硬件底层射频驱动
 * 说明(备注)  : 轻量化缓冲配置，适配片内 SRAM 资源
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功; false: 失败
 ************************************************************************************************************************/
static bool prv_bInitDriver(void)
{
	if (s_b_wifi_driver_inited)
		return true;

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	cfg.static_rx_buf_num  = 4;
	cfg.dynamic_rx_buf_num = 16;
	cfg.dynamic_tx_buf_num = 16;
	cfg.mgmt_sbuf_num      = 16;
	esp_err_t err = esp_wifi_init(&cfg);
	if (err != ESP_OK)
		return false;

	s_b_wifi_driver_inited = true;
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 注册 WiFi 与 IP 事件监听
 * 说明(备注)  : 供 v_wifi_queue_task_init 在任务启动步骤中安全调用
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功; false: 失败
 ************************************************************************************************************************/
static bool prv_bRegisterEvents(void)
{
	esp_err_t err = ESP_OK;

	if (s_wifi_event_instance == NULL)
	{
		err = esp_event_handler_instance_register(WIFI_EVENT,
		                                          ESP_EVENT_ANY_ID,
		                                          &vWiFi_EventHandler,
		                                          NULL,
		                                          &s_wifi_event_instance);
		if (err != ESP_OK)
			return false;
	}

	if (s_ip_event_instance == NULL)
	{
		err = esp_event_handler_instance_register(IP_EVENT,
		                                          IP_EVENT_STA_GOT_IP,
		                                          &vWiFi_EventHandler,
		                                          NULL,
		                                          &s_ip_event_instance);
		if (err != ESP_OK)
			return false;
	}

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 启动 WiFi 底层射频驱动
 * 说明(备注)  : 供 v_wifi_queue_task_init 在配置完成后启动射频
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功; false: 失败
 ************************************************************************************************************************/
static bool prv_bStartDriver(void)
{
	return (esp_wifi_start() == ESP_OK);
}

/***********************************************************************************************************************
 * 函数功能    : WiFi 初始化队列子任务
 * 说明(备注)  : 异步状态机逐步完成：Flash凭据读取、NVS挂载、Netif协议栈初始化、WiFi硬件驱动初始化、
 *              注册事件监听、下发STA参数、启动射频驱动、置就绪标志
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_wifi_queue_task_init(Task_T *p_task)
{
	switch (p_task->ucStep)
	{
		//************************************步骤0:从EasyFlash载入WiFi配置凭据**********************************
		case 0:
		{
			bWiFi_LoadConfig();
			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		//************************************步骤1:初始化NVS存储************************************************
		case 1:
		{
			if (!prv_bInitNvs())
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_e("bWiFiTask:NVS存储初始化失败重试");

				if (bQueue_IsStepRetryOver(p_task, 3))
					cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		//************************************步骤2:初始化TCP/IP协议栈与网络接口*********************************
		case 2:
		{
			if (!prv_bInitNetif())
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_e("bWiFiTask:TCP/IP协议栈或STA接口初始化失败重试");

				if (bQueue_IsStepRetryOver(p_task, 3))
					cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		//************************************步骤3:初始化WiFi底层射频硬件驱动***********************************
		case 3:
		{
			if (!prv_bInitDriver())
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_e("bWiFiTask:WiFi硬件驱动初始化失败重试");

				if (bQueue_IsStepRetryOver(p_task, 3))
				{
					#if (boardUSE_OS)
					vTaskDelay(pdMS_TO_TICKS(5000));
					#endif  /* boardUSE_OS */
					cQueue_GotoStep(p_task, STEP_END);
				}
				break;
			}

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		//************************************步骤4:注册底层WiFi与IP事件监听************************************
		case 4:
		{
			if (!prv_bRegisterEvents())
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_e("bWiFiTask:注册事件监听失败重试");

				if (bQueue_IsStepRetryOver(p_task, 3))
					cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		//************************************步骤5:配置STA工作模式与网络凭据**********************************
		case 5:
		{
			vWiFi_ApplyStaConfig(tWiFiInfo.saSsid, tWiFiInfo.saPassword);
			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		//************************************步骤6:启动WiFi底层射频驱动并开启节能模式**************************
		case 6:
		{
			if (!prv_bStartDriver())
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_w("bWiFiTask:启动WiFi射频驱动重试");

				if (bQueue_IsStepRetryOver(p_task, 3))
					cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			bWiFi_SetPowerSave(true);
			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		//************************************步骤7:就绪标记与完成初始化****************************************
		case 7:
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
