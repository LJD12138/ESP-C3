/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\main
 * File    : fwdgt.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 看门狗驱动及复位源标志捕获实现 (ESP32-C3 / ESP-IDF 任务看门狗版本)
 * -------------------------------------------------------
 * 移植说明(ESP32-C3 / ESP-IDF v5.5 适配):
 * 1. GD32 独立看门狗(FWDGT) -> ESP-IDF 任务看门狗(Task WDT)
 *    ESP-IDF 默认已初始化 TWDT 并订阅 CPU0 的 IDLE 任务, 此处按 boardWDGT_TIMEOUT_S
 *    重配超时并打开 trigger_panic, 使 IDLE 任务被饿死时能够真正复位系统
 * 2. GD32 RCU 复位标志 -> esp_reset_reason()
 * 3. vFwdgt_Reload 仅在当前任务已订阅 TWDT 时才喂狗, 避免 ESP_ERR_NOT_FOUND
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "fwdgt.h"

#if (boardWDGT_EN)

#include "esp_task_wdt.h"
#include "esp_system.h"

//****************************************************Parameter Initialization**************************************************//
ResetReason_T g_reset_reason;

/***********************************************************************************************************************
 * 函数功能    : 看门狗初始化
 * 说明(备注)  : 配置 ESP-IDF 任务看门狗超时时间与触发动作, 并监控 CPU0 空闲任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vFwdgt_Init(void)
{
	esp_task_wdt_config_t t_cfg = {
		.timeout_ms      = (uint32_t)boardWDGT_TIMEOUT_S * 1000U,	/* 超时时间(ms) */
		.idle_core_mask  = (1U << 0),								/* 监控 CPU0 的空闲任务 */
		.trigger_panic   = true,									/* 超时后触发 panic 并复位 */
	};

	esp_err_t e_ret = esp_task_wdt_reconfigure(&t_cfg);

	/* 若 TWDT 尚未初始化(reconfigure 返回无效状态), 则直接初始化 */
	if (e_ret == ESP_ERR_INVALID_STATE)
		e_ret = esp_task_wdt_init(&t_cfg);

	if (e_ret != ESP_OK)
		printf("[WDGT] Task WDT init failed, err = %d\r\n", (int)e_ret);
}

/***********************************************************************************************************************
 * 函数功能    : 看门狗喂狗
 * 说明(备注)  : 当前任务已订阅 TWDT 时重置计数; 未订阅时静默跳过
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vFwdgt_Reload(void)
{
	if (esp_task_wdt_status(NULL) == ESP_OK)
		(void)esp_task_wdt_reset();
}

/***********************************************************************************************************************
 * 函数功能    : 看门狗进入低功耗配置
 * 说明(备注)  : ESP32 任务看门狗由硬件/系统托管, 低功耗下无法改周期;
 *               这里仅先行喂狗, 保持与 GD32 版本相同的调用契约
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vFwdgt_EnterLowPower(void)
{
	vFwdgt_Reload();
}

/***********************************************************************************************************************
 * 函数功能    : 看门狗退出低功耗
 * 说明(备注)  : 喂狗并按配置重新初始化任务看门狗
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vFwdgt_ExitLowPower(void)
{
	vFwdgt_Reload();
	vFwdgt_Init();
}

/***********************************************************************************************************************
 * 函数功能    : 获取并暂存复位源信息
 * 说明(备注)  : 读取 ESP-IDF 复位原因并映射到通用复位标志
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vResetReason_Capture(void)
{
	esp_reset_reason_t e_reason = esp_reset_reason();

	memset(&g_reset_reason, 0, sizeof(g_reset_reason));
	g_reset_reason.esp_reason = (u8)e_reason;

	g_reset_reason.ext_pin   = (e_reason == ESP_RST_EXT);
	g_reset_reason.por       = (e_reason == ESP_RST_POWERON);
	g_reset_reason.sw        = (e_reason == ESP_RST_SW);
	g_reset_reason.fwdgt     = (e_reason == ESP_RST_TASK_WDT) || (e_reason == ESP_RST_WDT);
	g_reset_reason.wwdgt     = (e_reason == ESP_RST_INT_WDT);
	g_reset_reason.low_power = (e_reason == ESP_RST_DEEPSLEEP);
	g_reset_reason.panic     = (e_reason == ESP_RST_PANIC);
	g_reset_reason.brownout  = (e_reason == ESP_RST_BROWNOUT);
}

/***********************************************************************************************************************
 * 函数功能    : 打印看门狗及系统复位原因
 * 说明(备注)  : 解析捕获的复位原因并输出调试日志
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void vFwdgt_PrintResetReason(void)
{
	vResetReason_Capture();

	if (g_reset_reason.fwdgt)
		printf("[Reset] reason: FWDGT reset\r\n");
	else if (g_reset_reason.wwdgt)
		printf("[Reset] reason: WWDGT reset\r\n");
	else if (g_reset_reason.sw)
		printf("[Reset] reason: software reset\r\n");
	else if (g_reset_reason.ext_pin)
		printf("[Reset] reason: external pin reset\r\n");
	else if (g_reset_reason.por)
		printf("[Reset] reason: power on reset\r\n");
	else if (g_reset_reason.low_power)
		printf("[Reset] reason: low power reset\r\n");
	else if (g_reset_reason.panic)
		printf("[Reset] reason: panic reset\r\n");
	else if (g_reset_reason.brownout)
		printf("[Reset] reason: brownout reset\r\n");
	else
		printf("[Reset] reason: unknown(%d)\r\n", (int)g_reset_reason.esp_reason);
}

#endif  //boardWDGT_EN
