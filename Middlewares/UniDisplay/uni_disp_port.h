/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\UniDisplay
 * File    : uni_disp_port.h
 * Date    : 2026-09-16
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 移植层 - 临界区宏与显示任务唤醒钩子 (工程唯一需要按平台适配的文件)
 * -------------------------------------------------------
 * 移植说明:
 * 1. FreeRTOS 环境: 提供任务级临界区与任务通知唤醒, 等待原语由任务主体(通知+超时)实现;
 * 2. 裸机环境: 临界区为空实现, 唤醒为空操作;
 * 3. 本文件不含任何业务逻辑, 其余 UniDisplay 文件不需要因平台差异而修改。
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef UNI_DISP_PORT_H
#define UNI_DISP_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_data.h"	/* 项目快照定义 (引擎栈分配快照需完整类型, 工程适配点) */

#if (boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//

/* 板级显示参数转接 (框架唯一允许引用 board_config 显示参数的位置, 其余通用文件一律使用别名) */
#define			dispFRAME_PERIOD_MIN_MS					(33U)	/* 页面帧周期下限 ms (适配TFT 33ms/30FPS) */

//****************************************************Types*********************************************************************//

//****************************************************Extern********************************************************************//
#if (boardUSE_OS)
/* 绑定显示任务句柄 (显示任务创建后调用一次, 供唤醒钩子使用) */
void vDisp_PortSetTaskHandle(TaskHandle_t t_handle);

/* 唤醒显示任务: 事件入队/背光点亮后调用, 使 EnginePoll 提前执行 */
void vDisp_PortWakeTask(void);
#else
#define			vDisp_PortSetTaskHandle(h_handle)		((void)0)
#define			vDisp_PortWakeTask()					((void)0)
#endif  //boardUSE_OS

/*----------------项目耦合适配原语 (对项目的全部访问经此汇聚, 通用件零项目依赖)----------------*/
/* 渲染输出一轮 (转调项目件: ui_tick -> 自绘动画 -> lv_timer_handler) */
void vDisp_PortUiRefresh(void);

/* 背光硬件开关 (true=点亮, false=熄灭) */
void vDisp_PortSetBacklight(bool b_on);

/* 持久化息屏时间秒数 (0=常亮) */
uint16_t usDisp_PortGetAutoOffTime(void);

/* 系统就绪判定 (false 时引擎锁定 INIT 页) */
bool bDisp_PortIsSystemReady(void);

/* 常亮亮屏提示钩子 (内部按 boardBUZ_EN 裁剪) */
void vDisp_PortBeepOnLightOn(void);

#endif  /* boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif

#endif  /* UNI_DISP_PORT_H */
