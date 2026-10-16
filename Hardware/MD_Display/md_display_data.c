/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display
 * File    : md_display_data.c
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 数据快照池实现 - 全局状态原子快照捕获与设备状态路由表
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>

#include "MD_Display/md_display_data.h"

#if (boardDISPLAY_EN)
#include "uni_disp_port.h"
#include "Sys/sys_task.h"
#include "MD_Display/md_display_task.h"
#include "BSP/bsp_gxhtc3_temp_humi.h"

#if (boardWIFI_EN)
#include "WiFi/wifi_task.h"
#endif  /* boardWIFI_EN */

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//
static int16_t   s_s_env_temp_x10 = 0x7FFF;
static uint8_t   s_uc_env_humi    = 0xFF;
static uint8_t   s_uc_net_h       = 0xFF;
static uint8_t   s_uc_net_m       = 0xFF;
static uint8_t   s_uc_net_s       = 0xFF;
static Weather_E s_e_weather      = WEATHER_SUNNY;
static bool      s_b_weather_set  = false;

//****************************************************Function Declaration******************************************************//
#if (boardWIFI_EN)
static Weather_E prv_eMapWeatherCode(uint8_t uc_code, bool b_is_day);
static uint8_t   prv_ucCalcSignalLevel(int8_t c_rssi, WiFiConnectState_E e_state);
#endif  /* boardWIFI_EN */

/***********************************************************************************************************************
 * 函数功能    : 设置环境温湿度数据
 * 说明(备注)  : 供传感器任务或驱动层在换算后调用
 * 传入参数    : s_temp_x10: 室内温度×0.1℃, uc_humi: 室内湿度 %RH
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_DataSetEnv(int16_t s_temp_x10, uint8_t uc_humi)
{
	mainENTER_CRITICAL();
	s_s_env_temp_x10 = s_temp_x10;
	s_uc_env_humi    = uc_humi;
	mainEXIT_CRITICAL();
}

/***********************************************************************************************************************
 * 函数功能    : 设置网络同步时间
 * 说明(备注)  : 供网络/SNTP任务同步后显式注入
 * 传入参数    : h: 时, m: 分, s: 秒
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_DataSetNetTime(uint8_t h, uint8_t m, uint8_t s)
{
	mainENTER_CRITICAL();
	s_uc_net_h = h;
	s_uc_net_m = m;
	s_uc_net_s = s;
	mainEXIT_CRITICAL();
}

/***********************************************************************************************************************
 * 函数功能    : 设置网络天气类型
 * 说明(备注)  : 供天气更新任务拉取后调用
 * 传入参数    : e_weather: 天气枚举
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_DataSetWeather(Weather_E e_weather)
{
	mainENTER_CRITICAL();
	s_e_weather     = e_weather;
	s_b_weather_set = true;
	mainEXIT_CRITICAL();
}

/***********************************************************************************************************************
 * 函数功能    : 获取当前全局数据快照
 * 说明(备注)  : 临界区保护读取全局状态、功率与标志位，实现单向原子输入
 * 传入参数    : 无
 * 输出参数    : p_snapshot: 快照存放结构体指针
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_DataCaptureSnapshot(DispDataSnapshot_T *p_snapshot)
{
	if (p_snapshot == NULL)
		return;

	mainENTER_CRITICAL();

	/*----------------系统状态与功率----------------*/
	p_snapshot->ucDevState   = (uint8_t)tSysInfo.eDevState;
	p_snapshot->usSysErrCode = tSysInfo.uErrCode.usCode;
	p_snapshot->usInPwr      = tSysInfo.usInPwr;
	p_snapshot->usOutPwr     = tSysInfo.usOutPwr;

	/*----------------端口与电池预留状态 (已裁剪外设置零)----------------*/
	p_snapshot->ulPortActiveFlags    = 0;
	p_snapshot->ulPortBlinkFlags     = 0;
	p_snapshot->bBatErr              = false;
	p_snapshot->bBatLock             = false;
	p_snapshot->ucSoc                = 0;
	p_snapshot->usChgFullTimeMin     = 0;
	p_snapshot->usDisChgEmptyTimeMin = 0;

	/*----------------环境温湿度数据读取----------------*/
	if (s_s_env_temp_x10 != 0x7FFF)
	{
		p_snapshot->sTempX10   = s_s_env_temp_x10;
		p_snapshot->ucHumidity = s_uc_env_humi;
	}
	else if (bGxhtc3_IsIfaceInited())
	{
		p_snapshot->sTempX10   = sGxhtc3_GetTempX10();
		p_snapshot->ucHumidity = ucGxhtc3_GetHumi();
	}
	else
	{
		p_snapshot->sTempX10   = 0x7FFF;
		p_snapshot->ucHumidity = 0xFF;
	}

	mainEXIT_CRITICAL();

	/*----------------低频量 (依赖访问函数, 显示任务单读者直读安全)----------------*/
	p_snapshot->ucChgState       = (uint8_t)cSys_IsChgState();
	p_snapshot->usPrimaryErrCode = usDisp_ErrCodeDisplay();

	/*----------------WiFi 状态、时间和天气读取----------------*/
	#if (boardWIFI_EN)
	p_snapshot->ucSignalLevel = prv_ucCalcSignalLevel(tWiFiInfo.cRssi, tWiFiInfo.eState);

	if (bWiFi_SntpIsSynced())
	{
		time_t now = 0;
		struct tm timeinfo;
		time(&now);
		localtime_r(&now, &timeinfo);

		p_snapshot->ucLocalHour = (uint8_t)timeinfo.tm_hour;
		p_snapshot->ucLocalMin  = (uint8_t)timeinfo.tm_min;
		p_snapshot->ucLocalSec  = (uint8_t)timeinfo.tm_sec;
		p_snapshot->ucLocalMon  = (uint8_t)(timeinfo.tm_mon + 1);
		p_snapshot->ucLocalDay  = (uint8_t)timeinfo.tm_mday;
		p_snapshot->ucLocalWday = (uint8_t)timeinfo.tm_wday;
		p_snapshot->bNetSynced  = true;
	}
	else
	{
		p_snapshot->ucLocalHour = 0xFF;
		p_snapshot->ucLocalMin  = 0xFF;
		p_snapshot->ucLocalSec  = 0xFF;
		p_snapshot->ucLocalMon  = 0;
		p_snapshot->ucLocalDay  = 0;
		p_snapshot->ucLocalWday = 0;
		p_snapshot->bNetSynced  = false;
	}

	if (bWiFi_WeatherIsValid())
	{
		WiFiWeatherInfo_T t_w;
		if (bWiFi_WeatherGetInfo(&t_w))
		{
			p_snapshot->bWeatherValid   = true;
			p_snapshot->eWeather        = (uint8_t)prv_eMapWeatherCode(t_w.ucWeatherCode, t_w.bIsDay);
			p_snapshot->sWeatherTempX10 = (int16_t)roundf(t_w.fTemperature * 10.0f);
			strncpy(p_snapshot->saWeatherDesc, t_w.saWeatherDesc, sizeof(p_snapshot->saWeatherDesc) - 1);
			p_snapshot->saWeatherDesc[sizeof(p_snapshot->saWeatherDesc) - 1] = '\0';
		}
		else
		{
			p_snapshot->bWeatherValid = false;
			p_snapshot->eWeather      = (uint8_t)WEATHER_SUNNY;
			p_snapshot->saWeatherDesc[0] = '\0';
		}
	}
	else
	{
		p_snapshot->bWeatherValid = false;
		p_snapshot->eWeather      = (uint8_t)WEATHER_SUNNY;
		p_snapshot->saWeatherDesc[0] = '\0';
	}
	#else
	p_snapshot->ucSignalLevel    = 0;
	p_snapshot->bNetSynced       = false;
	p_snapshot->ucLocalHour      = 0xFF;
	p_snapshot->bWeatherValid    = false;
	p_snapshot->eWeather         = (uint8_t)WEATHER_SUNNY;
	p_snapshot->saWeatherDesc[0] = '\0';
	#endif  /* boardWIFI_EN */
}

#if (boardWIFI_EN)
/***********************************************************************************************************************
 * 函数功能    : WMO 天气代码映射为 Weather_E
 * 传入参数    : uc_code: WMO 天气编码
 * 返回值      : Weather_E
 ************************************************************************************************************************/
static Weather_E prv_eMapWeatherCode(uint8_t uc_code, bool b_is_day)
{
	if (b_is_day)
	{
		switch (uc_code)
		{
			case 0:
			case 1:
				return WEATHER_SUNNY;
			case 2:
				return WEATHER_CLOUDY;
			case 3:
			case 45:
			case 48:
				return WEATHER_OVERCAST;
			default:
				return WEATHER_RAIN;
		}
	}
	else
	{
		switch (uc_code)
		{
			case 0:
			case 1:
				return WEATHER_NIGHT_SUNNY;
			case 2:
				return WEATHER_NIGHT_CLOUDY;
			case 3:
			case 45:
			case 48:
				return WEATHER_NIGHT_OVERCAST;
			default:
				return WEATHER_NIGHT_RAIN;
		}
	}
}

/***********************************************************************************************************************
 * 函数功能    : 根据 RSSI 计算信号格数 0~5
 * 传入参数    : c_rssi: 信号强度 (dBm), e_state: 连接状态
 * 返回值      : 信号格数 0~5
 ************************************************************************************************************************/
static uint8_t prv_ucCalcSignalLevel(int8_t c_rssi, WiFiConnectState_E e_state)
{
	if (e_state != WIFI_STATE_CONNECTED)
		return 0;

	if (c_rssi >= -55)
		return 5;
	else if (c_rssi >= -66)
		return 4;
	else if (c_rssi >= -77)
		return 3;
	else if (c_rssi >= -88)
		return 2;
	else if (c_rssi >= -99)
		return 1;
	else
		return 0;
}
#endif  /* boardWIFI_EN */

/***********************************************************************************************************************
 * 函数功能    : 息屏倒计时生效判定
 * 说明(备注)  : 判断当前设备状态是否允许倒计时息屏 (仅工作状态参与倒计时递减)
 * 传入参数    : uc_dev_state: 当前设备状态枚举值
 * 输出参数    : 无
 * 返回值      : true: 允许倒计时递减; false: 暂停倒计时
 ************************************************************************************************************************/
bool bDisp_DataAutoOffArmed(uint8_t uc_dev_state)
{
	return (uc_dev_state == DS_WORK);
}

/***********************************************************************************************************************
 * 函数功能    : 设备状态到页面映射表
 * 说明(备注)  : 根据设备全局状态返回 UniDisplay 目标页面 ID
 * 传入参数    : uc_dev_state: 当前设备状态枚举值
 * 输出参数    : 无
 * 返回值      : DispPageId_E: 目标页面 ID, PAGE_ID_MAX 表示维持当前页
 ************************************************************************************************************************/
DispPageId_E eDisp_MapDevStateToPage(uint8_t uc_dev_state)
{
	switch ((DevState_E)uc_dev_state)
	{
		case DS_INIT:        return PAGE_ID_INIT;        /* 初始化: 保持息屏 */
		case DS_BOOTING:     return PAGE_ID_BOOT;        /* 启动中: 开机步进动画页 */
		case DS_ERR:         return PAGE_ID_FAULT;       /* 故障态: 故障专用告警页 */
		case DS_CLOSING:     return PAGE_ID_CLOSING;     /* 关机中: 关机画面展示 */
		case DS_SHUT_DOWN:   return PAGE_ID_SHUT_DOWN;   /* 已关机: 关机状态页(息屏) */
		#if (boardUPDATE)
		case DS_UPDATE_MODE: return PAGE_ID_UPDATE;      /* 升级中: 固件升级进度页 */
		#endif  /* boardUPDATE */
		#if (boardENG_MODE_EN)
		case DS_ENG_MODE:    return PAGE_ID_ENG_MODE;    /* 工程模式: 工程校准设置页 */
		#endif  /* boardENG_MODE_EN */
		default:             return PAGE_ID_MAX;         /* 无映射: 维持当前页 (如工作态在开机进度完成后跳转) */
	}
}

#endif  /* boardDISPLAY_EN */
