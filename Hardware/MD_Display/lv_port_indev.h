/***********************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display
 * File    : lv_port_indev.h
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : LVGL输入设备（触摸屏）移植头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "board_config.h"
#include "MD_Display/md_display_iface.h"

#if (boardDISPLAY_EN && ESP_LCD_TOUCH_ENABLED)
#include "lvgl.h"


//****************************************************Macros********************************************************************//
/* 跳过lv_conf.h，使用Kconfig配置 */
#ifndef LV_CONF_SKIP
#define			LV_CONF_SKIP							1
#endif

//****************************************************Extern********************************************************************//
/* 初始化LVGL输入设备驱动（触摸屏） */
void     lv_port_indev_init(void);

#endif  /* boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif

#endif  /* LV_PORT_INDEV_H */
