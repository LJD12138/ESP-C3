/***********************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\BSP
 * File    : bsp_qmc5883l.h
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : QMC5883L 3轴磁力计驱动
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef BSP_QMC5883L_H_
#define BSP_QMC5883L_H_

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"
#include <qmc5883l.h>

#if (1)

//****************************************************Macros********************************************************************//
// QMC5883L I2C地址
#define			QMC5883L_I2C_ADDR						QMC5883L_I2C_ADDR_DEF

//****************************************************Types*********************************************************************//
// 输出数据率
typedef qmc5883l_odr_t      Qmc5883l_Odr_E;
#define			QMC5883L_ODR_10HZ						QMC5883L_DR_10
#define			QMC5883L_ODR_50HZ						QMC5883L_DR_50
#define			QMC5883L_ODR_100HZ						QMC5883L_DR_100
#define			QMC5883L_ODR_200HZ						QMC5883L_DR_200

// 过采样率
typedef qmc5883l_osr_t      Qmc5883l_Osr_E;
#define			QMC5883L_OSR_64							QMC5883L_OSR_64
#define			QMC5883L_OSR_128						QMC5883L_OSR_128
#define			QMC5883L_OSR_256						QMC5883L_OSR_256
#define			QMC5883L_OSR_512						QMC5883L_OSR_512

// 量程
typedef qmc5883l_range_t    Qmc5883l_Range_E;
#define			QMC5883L_RNG_2G							QMC5883L_RNG_2
#define			QMC5883L_RNG_8G							QMC5883L_RNG_8

// 工作模式
typedef qmc5883l_mode_t     Qmc5883l_Mode_E;
#define			QMC5883L_MODE_STANDBY					QMC5883L_MODE_STANDBY
#define			QMC5883L_MODE_CONTINUOUS				QMC5883L_MODE_CONTINUOUS

// 原始数据结构
typedef qmc5883l_raw_data_t Qmc5883l_RawData_T;

// 物理数据结构(毫高斯)
typedef qmc5883l_data_t     Qmc5883l_Data_T;

// 配置结构
typedef struct
{
	Qmc5883l_Odr_E		eOdr;				// 输出数据率
	Qmc5883l_Osr_E		eOsr;				// 过采样率
	Qmc5883l_Range_E	eRange;				// 量程
	Qmc5883l_Mode_E		eMode;				// 工作模式
}Qmc5883l_Config_T;

//****************************************************Extern********************************************************************//
// 控制接口
void                        vQmc5883l_IfaceInit(void);
s8                          cQmc5883l_Init(void);
s8                          cQmc5883l_Config(Qmc5883l_Config_T *p_config);
s8                          cQmc5883l_SetMode(Qmc5883l_Mode_E e_mode);
s8                          cQmc5883l_ReadData(Qmc5883l_Data_T *p_data);
s8                          cQmc5883l_ReadRawData(Qmc5883l_RawData_T *p_data);

// 数据接口
bool                        bQmc5883l_IsIfaceInited(void);
bool                        bQmc5883l_DataReady(void);
float                       fQmc5883l_GetHeading(void);
DevState_E                  eQmc5883l_GetDevState(void);
const Qmc5883l_Data_T      *ptQmc5883l_GetData(void);
const Qmc5883l_Config_T    *ptQmc5883l_GetConfig(void);

#endif  /* 1 */

#ifdef __cplusplus
}
#endif

#endif  /* BSP_QMC5883L_H_ */