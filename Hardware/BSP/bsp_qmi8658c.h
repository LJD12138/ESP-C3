/***********************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\BSP
 * File    : bsp_qmi8658c.h
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : QMI8658C 6轴IMU传感器驱动
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef BSP_QMI8658C_H_
#define BSP_QMI8658C_H_

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"
#include <qmi8658c.h>

#if (1)

//****************************************************Macros********************************************************************//
// QMI8658C I2C地址
#define			QMI8658C_I2C_ADDR						DEFAUL_QMI8658C_ADDR

//****************************************************Types*********************************************************************//
// 加速度计量程
typedef qmi8658c_acc_scale_t   Qmi8658c_AccRange_E;
#define			QMI8658C_ACC_RANGE_2G					QMI8658C_ACC_SCALE_2G
#define			QMI8658C_ACC_RANGE_4G					QMI8658C_ACC_SCALE_4G
#define			QMI8658C_ACC_RANGE_8G					QMI8658C_ACC_SCALE_8G
#define			QMI8658C_ACC_RANGE_16G					QMI8658C_ACC_SCALE_16G

// 陀螺仪量程
typedef qmi8658c_gyro_scale_t  Qmi8658c_GyrRange_E;
#define			QMI8658C_GYR_RANGE_16DPS				QMI8658C_GYRO_SCALE_16DPS
#define			QMI8658C_GYR_RANGE_32DPS				QMI8658C_GYRO_SCALE_32DPS
#define			QMI8658C_GYR_RANGE_64DPS				QMI8658C_GYRO_SCALE_64DPS
#define			QMI8658C_GYR_RANGE_128DPS				QMI8658C_GYRO_SCALE_128DPS
#define			QMI8658C_GYR_RANGE_256DPS				QMI8658C_GYRO_SCALE_256DPS
#define			QMI8658C_GYR_RANGE_512DPS				QMI8658C_GYRO_SCALE_512DPS
#define			QMI8658C_GYR_RANGE_1024DPS				QMI8658C_GYRO_SCALE_1024DPS
#define			QMI8658C_GYR_RANGE_2048DPS				QMI8658C_GYRO_SCALE_2048DPS

// 加速度计ODR
typedef qmi8658c_acc_odr_t     Qmi8658c_AccOdr_E;

// 陀螺仪ODR
typedef qmi8658c_gyro_odr_t    Qmi8658c_GyrOdr_E;

// 工作模式
typedef qmi8658c_mode_t        Qmi8658c_Mode_E;
#define			QMI8658C_MODE_ACC_ONLY					QMI8658C_MODE_ACC_ONLY
#define			QMI8658C_MODE_GYRO_ONLY					QMI8658C_MODE_GYRO_ONLY
#define			QMI8658C_MODE_DUAL						QMI8658C_MODE_DUAL

// 数据结构
typedef qmi8658c_axes_t        Qmi8658c_Axes_T;
typedef qmi8658c_data_t        Qmi8658c_Data_T;

// 配置结构
typedef qmi8658c_config_t      Qmi8658c_Config_T;

//****************************************************Extern********************************************************************//
// 控制接口
void                        vQmi8658c_IfaceInit(void);
s8                          cQmi8658c_Init(void);
s8                          cQmi8658c_Config(Qmi8658c_Config_T *p_config);
s8                          cQmi8658c_ReadData(Qmi8658c_Data_T *p_data);
s8                          cQmi8658c_SetAccRange(Qmi8658c_AccRange_E e_range);
s8                          cQmi8658c_SetGyrRange(Qmi8658c_GyrRange_E e_range);

// 数据接口
bool                        bQmi8658c_IsIfaceInited(void);
float                       fQmi8658c_GetTemp(void);
DevState_E                  eQmi8658c_GetDevState(void);
const Qmi8658c_Data_T      *ptQmi8658c_GetData(void);
const Qmi8658c_Config_T    *ptQmi8658c_GetConfig(void);

#endif  /* 1 */

#ifdef __cplusplus
}
#endif

#endif  /* BSP_QMI8658C_H_ */