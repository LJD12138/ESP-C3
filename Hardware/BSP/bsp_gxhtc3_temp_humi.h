/***********************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\BSP
 * File    : bsp_gxhtc3_temp_humi.h
 * Date    : 2026-01-23
 * Author  : LJD(291483914@qq.com)
 * Desc    : 温湿度传感器Gxhtc3驱动
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef BSP_GXHTC3_TEMP_HUMI_H
#define BSP_GXHTC3_TEMP_HUMI_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"
#include "board_config.h"

#if (1)
//****************************************************Macros********************************************************************//
#define			GXHTC3_I2C_ADDR							0x70	// 7位从机地址
#define			I2C_MASTER_SCL_IO						GPIO_NUM_1	/*!< GPIO number used for I2C master clock */
#define			I2C_MASTER_SDA_IO						GPIO_NUM_0	/*!< GPIO number used for I2C master data  */
#define			I2C_MASTER_NUM							0		/*!< I2C master i2c port number */
#define			I2C_MASTER_FREQ_HZ						400000	/*!< I2C master clock frequency */
#define			I2C_MASTER_TX_BUF_DISABLE				0		/*!< I2C master doesn't need buffer */
#define			I2C_MASTER_RX_BUF_DISABLE				0		/*!< I2C master doesn't need buffer */
#define			I2C_MASTER_TIMEOUT_MS					1000

//****************************************************Extern********************************************************************//
bool bGxhtc3_IfaceInit(void);
s8 cGxhtc3_GetTempHumi(void);
s8 cGxhtc3_ReadId(uint16_t *p_id);
s8 cGxhtc3_SoftReset(void);

/* 数据接口 */
int16_t sGxhtc3_GetTempX10(void);
uint8_t ucGxhtc3_GetHumi(void);
bool bGxhtc3_IsIfaceInited(void);

#endif  /* 1 */

#ifdef __cplusplus
}
#endif

#endif  /* BSP_GXHTC3_TEMP_HUMI_H */