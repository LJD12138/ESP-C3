/***********************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\WiFi
 * File    : wifi_task.h
 * Date    : 2026-10-14
 * Author  : LJD(291483914@qq.com)
 * Desc    : WiFi 连接管理任务头文件，声明状态机类型、配置及控制接口
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardWIFI_EN)
#include "WiFi/wifi_queue_task.h"

/* 前置声明，避免在全局头文件中引入 <time.h> 与 main.h 宏冲突 */
struct tm;
//****************************************************Macros********************************************************************//
#ifndef wifiDEFAULT_SSID
#define			wifiDEFAULT_SSID						"YF-5G"	/* 默认出厂测试 SSID */
#endif

#ifndef wifiDEFAULT_PWD
#define			wifiDEFAULT_PWD							"asdfghjkl"	/* 默认出厂测试密码 */
#endif

#define			wifiENV_KEY_SSID						"wifi_ssid"	/* EasyFlash 中 WiFi SSID 键名 */
#define			wifiENV_KEY_PWD							"wifi_pwd"	/* EasyFlash 中 WiFi 密码键名 */
#define			wifiENV_KEY_AUTO						"wifi_auto"	/* EasyFlash 中自动连接使能键名 */

#define			wifiRECONNECT_MAX_INTERVAL_MS			30000	/* 最大重连退避间隔 30s */
#define			wifiRECONNECT_BASE_INTERVAL_MS			1000	/* 初始重连间隔 1s */
#define			wifiRSSI_POLL_INTERVAL_MS				5000	/* 已连接状态下 RSSI 采样周期 5s */
#define			wifiSNTP_SERVER_MAIN					"ntp.aliyun.com"	/* 阿里云 NTP 主对时服务器 */
#define			wifiSNTP_SYNC_INTERVAL_MS				(1000U * 60U * 60U)	/* 周期性网络对时间隔 1小时 */
#define			wifiSNTP_TIMEOUT_MS						10000	/* 单次对时超时等待门限 10s */
#define			wifiWEATHER_URL_DEFAULT					"http://api.open-meteo.com/v1/forecast?latitude=39.9042&longitude=116.4074&current_weather=true"	/* Open-Meteo 免费天气 API (默认北京) */
#define			wifiWEATHER_UPDATE_INTERVAL_MS			(1000U * 60U * 30U)	/* 天气更新周期 30分钟 */
#define			wifiWEATHER_RETRY_INTERVAL_MS			10000	/* 天气失败重试等待门限 10s */
#define			wifiWEATHER_TIMEOUT_MS					8000	/* 单次 HTTP 请求超时等待 8s */
#define			wifiWEATHER_RX_BUF_SIZE					1024	/* HTTP 响应接收缓冲区大小 (字节) */
#define			wifiSCAN_MAX_AP_NUM						16		/* 扫描结果最大缓存热点数量 */
#define			wifiSCAN_TIMEOUT_MS						10000	/* 扫描单次最长等待超时 10s */

//****************************************************Types*********************************************************************//
/* WiFi 连接状态枚举 (双列对齐，以 Tab 为准) */
typedef enum
{
	WIFI_STATE_DISCONNECTED = 0,	/* 未连接/掉线 */
	WIFI_STATE_CONNECTING,	/* 正在连接/重连中 */
	WIFI_STATE_CONNECTED,	/* 已连接且成功获取 IP */
	WIFI_STATE_CONNECT_FAILED,	/* 连接失败 */
	WIFI_STATE_STOPPED,	/* 射频已关闭 (低功耗/关机) */
}WiFiConnectState_E;

/* 天气信息结构体 (1Tab -> 5Tab -> 5Tab 对齐) */
typedef struct
{
	bool				bValid;				/* 天气数据是否已成功获取并有效 */
	float				fTemperature;		/* 当前气温 (摄氏度 ℃) */
	float				fWindSpeed;			/* 当前风速 (km/h) */
	int16_t				sWindDirection;		/* 风向角 (0-360°) */
	uint8_t				ucWeatherCode;		/* WMO 天气编码 */
	bool				bIsDay;				/* 是否为白昼 (true: 白天, false: 夜晚) */
	char				saWeatherDesc[32];	/* 中文天气描述 (如 "晴", "多云", "小雨") */
	char				saUpdateTime[32];	/* 最近一次更新时间 (北京时间 "YYYY-MM-DD HH:MM:SS") */
}WiFiWeatherInfo_T;

/* 扫描热点条目结构体 (1Tab -> 5Tab -> 5Tab 对齐) */
typedef struct
{
	char				saSsid[33];			/* 热点 SSID 字符串 */
	int8_t				cRssi;				/* 信号强度 (-100 ~ 0 dBm) */
	uint8_t				ucChannel;			/* 信道编号 (1 ~ 13) */
	uint8_t				ucAuthMode;			/* 认证加密模式 (wifi_auth_mode_t) */
}WiFiScanApItem_T;

/* 扫描结果集合结构体 (1Tab -> 5Tab -> 5Tab 对齐) */
typedef struct
{
	bool				bScanning;			/* 是否正在扫描中 */
	bool				bScanDone;			/* 最近一次扫描是否已成功完成 */
	uint16_t			usTotalApNum;		/* 扫描发现的总 AP 数量 */
	uint16_t			usCachedApNum;		/* 实际缓存的 AP 数量 (<= wifiSCAN_MAX_AP_NUM) */
	WiFiScanApItem_T	taApList[wifiSCAN_MAX_AP_NUM];	/* 缓存的热点列表 (按信号由强到弱排序) */
}WiFiScanResult_T;

/* WiFi 运行状态信息结构体 (1Tab -> 5Tab -> 5Tab 对齐) */
typedef struct
{
	WiFiConnectState_E	eState;				/* 当前连接状态 */
	int8_t				cRssi;				/* 当前信号强度 (dBm, -100 ~ 0) */
	uint8_t				ucRetryCnt;			/* 连续重连失败计数 */
	bool				bSntpSynced;		/* SNTP 是否已成功完成对时 */
	bool				bPowerSaveEn;		/* 是否已启用 Modem Sleep 节能模式 */
	bool				bAutoConnect;		/* 是否启用开机自动连接 */
	char				saIpAddr[16];		/* 本机 IP 地址字符串 (IPv4) */
	char				saSsid[33];			/* 当前目标 SSID */
	char				saPassword[65];		/* 当前连接密码 */
	WiFiWeatherInfo_T	tWeather;			/* 当前天气信息 */
	WiFiScanResult_T	tScan;				/* 周边热点扫描结果集合 */
}WiFiInfo_T;

//****************************************************Globals*******************************************************************//
extern WiFiInfo_T tWiFiInfo;

//****************************************************Extern********************************************************************//
s8 cWiFi_TaskInit(void);
void vWiFi_ApplyStaConfig(const char *p_ssid, const char *p_pwd);
bool bWiFi_Connect(const char *p_ssid, const char *p_pwd);
bool bWiFi_Disconnect(void);
bool bWiFi_IsConnected(void);
int8_t cWiFi_GetRssi(void);
void vWiFi_GetIpStr(char *p_out_buf, size_t buf_len);
void vWiFi_Stop(void);
bool bWiFi_LoadConfig(void);
bool bWiFi_SaveConfig(const char *p_ssid, const char *p_pwd);
bool bWiFi_RestoreDefaultConfig(void);
bool bWiFi_SetAutoConnect(bool b_enable);
bool bWiFi_SetPowerSave(bool b_enable);
bool bWiFi_IsPowerSaveEnabled(void);
bool bWiFi_StartScan(void);
bool bWiFi_IsScanning(void);
bool bWiFi_IsScanDone(void);
uint16_t usWiFi_GetScanResult(WiFiScanApItem_T *p_out_list, uint16_t max_count);
const char *pcWiFi_AuthModeToDesc(uint8_t uc_auth_mode);
bool bWiFi_SntpIsSynced(void);
bool bWiFi_SntpGetLocalTime(struct tm *p_tm);
void vWiFi_SntpGetTimeStr(char *p_out_buf, size_t buf_len);
bool bWiFi_SntpSync(void);
bool bWiFi_WeatherIsValid(void);
bool bWiFi_WeatherGetInfo(WiFiWeatherInfo_T *p_info);
bool bWiFi_WeatherUpdate(void);
const char *pcWiFi_WeatherCodeToDesc(uint8_t uc_code);

#endif  /* boardWIFI_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* WIFI_TASK_H */
