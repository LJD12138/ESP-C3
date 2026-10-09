/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\main
 * File    : fwdgt.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 看门狗驱动与系统复位原因捕获头文件
 * -------------------------------------------------------
 * 移植说明(ESP32-C3 / ESP-IDF v5.5 适配):
 * 1. GD32 独立看门狗(FWDGT)替换为 ESP-IDF 任务看门狗(Task WDT / TWDT)
 *    —— 语义等价: 监控 IDLE 任务, 一旦系统任务全部饿死则触发 panic 并复位
 * 2. GD32 RCU 复位标志替换为 esp_reset_reason()
 * 3. 超时时间由 boardWDGT_TIMEOUT_S 配置
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef FWDGT_H_
#define FWDGT_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardWDGT_EN)

//****************************************************Types*********************************************************************//
typedef struct
{
	u8					ext_pin;			//外部管脚复位
	u8					por;				//上电/掉电复位
	u8					sw;					//软件复位
	u8					fwdgt;				//看门狗复位(任务看门狗/其它看门狗)
	u8					wwdgt;				//窗口看门狗复位(中断看门狗)
	u8					low_power;			//低功耗复位
	u8					panic;				//异常/断言复位
	u8					brownout;			//掉电/欠压复位
	u8					esp_reason;			//ESP-IDF 原始复位原因码(esp_reset_reason_t)
}ResetReason_T;

extern ResetReason_T g_reset_reason;

//****************************************************Extern********************************************************************//
void vFwdgt_Init(void);
void vFwdgt_Reload(void);
void vFwdgt_EnterLowPower(void);
void vFwdgt_ExitLowPower(void);
void vResetReason_Capture(void);
void vFwdgt_PrintResetReason(void);

#endif  /* boardWDGT_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* FWDGT_H_ */
