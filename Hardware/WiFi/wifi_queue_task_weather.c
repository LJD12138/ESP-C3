/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\WiFi
 * File    : wifi_queue_task_weather.c
 * Date    : 2026-10-14
 * Author  : LJD(291483914@qq.com)
 * Desc    : WiFi HTTP 天气数据拉取与 JSON 解析子任务及对外接口实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include <string.h>
#include <stdlib.h>
#include "esp_http_client.h"
#include "cJSON.h"

#include "WiFi/wifi_queue_task.h"

#if (boardWIFI_EN)
#include "WiFi/wifi_task.h"
#include "Print/print_task.h"

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//
static char s_sa_http_rx_buf[wifiWEATHER_RX_BUF_SIZE];

//****************************************************Function Declaration******************************************************//
static bool b_weather_parse_json(const char *p_json_str);


/***********************************************************************************************************************
 * 函数功能    : 解析 Open-Meteo 天气 JSON 字符串并更新全局状态
 * 说明(备注)  : 提取当前气温、风速、风向、天气编码与昼夜标识
 * 传入参数    : p_json_str: HTTP 响应体 JSON 文本
 * 输出参数    : 无
 * 返回值      : true: 解析成功并更新有效数据, false: 解析失败
 ************************************************************************************************************************/
static bool b_weather_parse_json(const char *p_json_str)
{
	if (p_json_str == NULL)
		return false;

	cJSON *root = cJSON_Parse(p_json_str);
	if (root == NULL)
	{
		if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
			log_w("bWiFiTask:天气JSON解析失败, 格式非法");
		return false;
	}

	cJSON *cur_weather = cJSON_GetObjectItem(root, "current_weather");
	if (cur_weather == NULL)
	{
		cJSON_Delete(root);
		return false;
	}

	cJSON *item_temp = cJSON_GetObjectItem(cur_weather, "temperature");
	cJSON *item_wind = cJSON_GetObjectItem(cur_weather, "windspeed");
	cJSON *item_dir  = cJSON_GetObjectItem(cur_weather, "winddirection");
	cJSON *item_code = cJSON_GetObjectItem(cur_weather, "weathercode");
	cJSON *item_day  = cJSON_GetObjectItem(cur_weather, "is_day");

	if (item_temp == NULL || item_code == NULL)
	{
		cJSON_Delete(root);
		return false;
	}

	tWiFiInfo.tWeather.fTemperature   = (float)item_temp->valuedouble;
	tWiFiInfo.tWeather.fWindSpeed     = (item_wind != NULL) ? (float)item_wind->valuedouble : 0.0f;
	tWiFiInfo.tWeather.sWindDirection = (item_dir != NULL) ? (int16_t)item_dir->valueint : 0;
	tWiFiInfo.tWeather.ucWeatherCode  = (uint8_t)item_code->valueint;
	tWiFiInfo.tWeather.bIsDay         = (item_day != NULL) ? (item_day->valueint != 0) : true;
	tWiFiInfo.tWeather.bValid         = true;

	const char *p_desc = pcWiFi_WeatherCodeToDesc(tWiFiInfo.tWeather.ucWeatherCode);
	strncpy(tWiFiInfo.tWeather.saWeatherDesc, p_desc, sizeof(tWiFiInfo.tWeather.saWeatherDesc) - 1);
	tWiFiInfo.tWeather.saWeatherDesc[sizeof(tWiFiInfo.tWeather.saWeatherDesc) - 1] = '\0';

	/* 记录最近一次同步时间 */
	vWiFi_SntpGetTimeStr(tWiFiInfo.tWeather.saUpdateTime, sizeof(tWiFiInfo.tWeather.saUpdateTime));

	if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
	{
		log_i("bWiFiTask:天气数据更新成功! 天气:%s, 气温:%.1f℃, 风速:%.1fkm/h, 风向:%d°, 昼夜:%s, 更新时间:[%s]",
		      tWiFiInfo.tWeather.saWeatherDesc,
		      tWiFiInfo.tWeather.fTemperature,
		      tWiFiInfo.tWeather.fWindSpeed,
		      tWiFiInfo.tWeather.sWindDirection,
		      tWiFiInfo.tWeather.bIsDay ? "白天" : "夜晚",
		      tWiFiInfo.tWeather.saUpdateTime);
	}

	cJSON_Delete(root);
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : WiFi HTTP 天气数据拉取队列子任务
 * 说明(备注)  : Step 0: 创建HTTP客户端并发起GET请求，接收并解析天气响应
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_wifi_queue_task_weather(Task_T *p_task)
{
	if (tWiFiInfo.eState != WIFI_STATE_CONNECTED)
	{
		cQueue_GotoStep(p_task, STEP_END);
		return;
	}

	switch (p_task->ucStep)
	{
		//************************************步骤0:发起HTTP GET请求并解析响应**********************************
		case 0:
		{
			if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
				log_i("bWiFiTask:发起HTTP天气查询请求 (Open-Meteo)...");

			esp_http_client_config_t config = {
				.url        = wifiWEATHER_URL_DEFAULT,
				.timeout_ms = wifiWEATHER_TIMEOUT_MS,
				.method     = HTTP_METHOD_GET,
			};

			esp_http_client_handle_t client = esp_http_client_init(&config);
			if (client == NULL)
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_e("bWiFiTask:HTTP客户端初始化失败");

				cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			esp_err_t err = esp_http_client_open(client, 0);
			if (err != ESP_OK)
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_w("bWiFiTask:HTTP连接打开失败: %s", esp_err_to_name(err));

				esp_http_client_cleanup(client);
				cQueue_GotoStep(p_task, STEP_END);
				break;
			}

			esp_http_client_fetch_headers(client);
			int status_code = esp_http_client_get_status_code(client);
			if (status_code == 200)
			{
				int read_len = esp_http_client_read_response(client, s_sa_http_rx_buf, sizeof(s_sa_http_rx_buf) - 1);
				if (read_len > 0)
				{
					s_sa_http_rx_buf[read_len] = '\0';
					b_weather_parse_json(s_sa_http_rx_buf);
				}
				else
				{
					if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
						log_w("bWiFiTask:HTTP响应内容为空");
				}
			}
			else
			{
				if (uPrint.tFlag.bWiFiTask || uPrint.tFlag.bImportant)
					log_w("bWiFiTask:HTTP请求异常, 状态码: %d", status_code);
			}

			esp_http_client_cleanup(client);
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

/***********************************************************************************************************************
 * 函数功能    : 将 WMO 天气编码转换为中文天气描述
 * 说明(备注)  : 覆盖常见 WMO 0-99 天气类型
 * 传入参数    : uc_code: WMO 天气编码
 * 输出参数    : 无
 * 返回值      : 中文天气描述字符串指针
 ************************************************************************************************************************/
const char *pcWiFi_WeatherCodeToDesc(uint8_t uc_code)
{
	switch (uc_code)
	{
		case 0:			return "晴";
		case 1:			return "大部晴朗";
		case 2:			return "多云";
		case 3:			return "阴";
		case 45:
		case 48:		return "雾";
		case 51:
		case 53:
		case 55:		return "毛毛雨";
		case 56:
		case 57:		return "冻毛毛雨";
		case 61:		return "小雨";
		case 63:		return "中雨";
		case 65:		return "大雨";
		case 66:
		case 67:		return "冻雨";
		case 71:		return "小雪";
		case 73:		return "中雪";
		case 75:		return "大雪";
		case 77:		return "雪粒";
		case 80:		return "阵雨(轻)";
		case 81:		return "阵雨(中)";
		case 82:		return "强阵雨";
		case 85:		return "阵雪(轻)";
		case 86:		return "强阵雪";
		case 95:		return "雷暴";
		case 96:
		case 99:		return "雷雨伴冰雹";
		default:		return "未知天气";
	}
}

/***********************************************************************************************************************
 * 函数功能    : 查询当前天气数据是否有效
 * 说明(备注)  : true 表示已成功拉取并解析过天气
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 有效, false: 无效或未拉取
 ************************************************************************************************************************/
bool bWiFi_WeatherIsValid(void)
{
	return tWiFiInfo.tWeather.bValid;
}

/***********************************************************************************************************************
 * 函数功能    : 获取当前天气完整信息
 * 说明(备注)  : 若未获取过天气数据则返回 false
 * 传入参数    : p_info: 存放天气信息的结构体指针
 * 输出参数    : p_info: 填充当前天气数据
 * 返回值      : true: 成功获取有效天气, false: 尚未获取或指针为空
 ************************************************************************************************************************/
bool bWiFi_WeatherGetInfo(WiFiWeatherInfo_T *p_info)
{
	if (p_info == NULL || !tWiFiInfo.tWeather.bValid)
		return false;

	memcpy(p_info, &tWiFiInfo.tWeather, sizeof(WiFiWeatherInfo_T));
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 主动触发一次天气数据拉取
 * 说明(备注)  : 向队列投递 WTI_WEATHER 任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功投递, false: 网络未就绪
 ************************************************************************************************************************/
bool bWiFi_WeatherUpdate(void)
{
	if (tWiFiInfo.eState != WIFI_STATE_CONNECTED)
		return false;

	#if (boardUSE_OS)
	if (tpWiFiTask != NULL)
		cQueue_AddQueueTask(tpWiFiTask, WTI_WEATHER, 0, false);
	#endif  /* boardUSE_OS */

	return true;
}

#endif  /* boardWIFI_EN */
