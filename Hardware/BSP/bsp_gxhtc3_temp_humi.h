/***********************************************************************************************************************
 * Project : ESP32-C3
 * Module  : G:\TempFile\ESP32-C3\Hardware\BSP
 * File    : esp_humi_gxhtc3.h
 * Date    : 2026-01-23 23:21:34
 * Author  : LJD(291483914@qq.com)
 * Desc    : description
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
************************************************************************************************************************/
#ifndef BSP_GXHTC3_TEMP_HUMI_H
#define BSP_GXHTC3_TEMP_HUMI_H


#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================includes====================================*/
#include "board_config.h"

#if(1)
/* ==========================================macros======================================*/
#define I2C_MASTER_SCL_IO           GPIO_NUM_1      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           GPIO_NUM_0      /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

/* ==========================================globals=====================================*/
//*********************************任务对象**********************************
#pragma pack(1)//强制一个字节对齐
typedef struct
{
    vu16             	usHumi;       //自动关闭计时
	vu16             	usTemp;      //自动关闭时间  0为关闭此功能
}Gxhtc3_T;         
#pragma pack() //取消一个字节对齐
extern  Gxhtc3_T    	tGxhtc3; 

/* ==========================================types=======================================*/


/* ==========================================extern======================================*/

bool bGxhtc3_IfaceInit(void);
s8 cGxhtc3_GetTempHumi(void);
#endif  //

#ifdef __cplusplus
}
#endif

#endif  //BSP_HUMI_GXHTC3_H