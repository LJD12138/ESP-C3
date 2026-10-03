/*****************************************************************************************************************
*                                            QMI8658C 6轴IMU传感器驱动                                          *
*                                      封装层 - 基于esp-idf-lib组件库                                            *
******************************************************************************************************************/
#ifndef BSP_QMI8658C_H_
#define BSP_QMI8658C_H_

#include "main.h"
#include <qmi8658c.h>

// QMI8658C I2C地址
#define     QMI8658C_I2C_ADDR               DEFAUL_QMI8658C_ADDR

// 加速度计量程
typedef qmi8658c_acc_scale_t   Qmi8658c_AccRange_E;
#define     QMI8658C_ACC_RANGE_2G           QMI8658C_ACC_SCALE_2G
#define     QMI8658C_ACC_RANGE_4G           QMI8658C_ACC_SCALE_4G
#define     QMI8658C_ACC_RANGE_8G           QMI8658C_ACC_SCALE_8G
#define     QMI8658C_ACC_RANGE_16G          QMI8658C_ACC_SCALE_16G

// 陀螺仪量程
typedef qmi8658c_gyro_scale_t  Qmi8658c_GyrRange_E;
#define     QMI8658C_GYR_RANGE_16DPS        QMI8658C_GYRO_SCALE_16DPS
#define     QMI8658C_GYR_RANGE_32DPS        QMI8658C_GYRO_SCALE_32DPS
#define     QMI8658C_GYR_RANGE_64DPS        QMI8658C_GYRO_SCALE_64DPS
#define     QMI8658C_GYR_RANGE_128DPS       QMI8658C_GYRO_SCALE_128DPS
#define     QMI8658C_GYR_RANGE_256DPS       QMI8658C_GYRO_SCALE_256DPS
#define     QMI8658C_GYR_RANGE_512DPS       QMI8658C_GYRO_SCALE_512DPS
#define     QMI8658C_GYR_RANGE_1024DPS      QMI8658C_GYRO_SCALE_1024DPS
#define     QMI8658C_GYR_RANGE_2048DPS      QMI8658C_GYRO_SCALE_2048DPS

// 加速度计ODR
typedef qmi8658c_acc_odr_t     Qmi8658c_AccOdr_E;

// 陀螺仪ODR
typedef qmi8658c_gyro_odr_t    Qmi8658c_GyrOdr_E;

// 工作模式
typedef qmi8658c_mode_t        Qmi8658c_Mode_E;
#define     QMI8658C_MODE_ACC_ONLY           QMI8658C_MODE_ACC_ONLY
#define     QMI8658C_MODE_GYRO_ONLY          QMI8658C_MODE_GYRO_ONLY
#define     QMI8658C_MODE_DUAL               QMI8658C_MODE_DUAL

// 数据结构
typedef qmi8658c_axes_t        Qmi8658c_Axes_T;
typedef qmi8658c_data_t        Qmi8658c_Data_T;

// 配置结构
typedef qmi8658c_config_t      Qmi8658c_Config_T;

// QMI8658C对象结构
typedef struct
{
    i2c_dev_t              tI2cDev;         // I2C设备对象
    Qmi8658c_Config_T      tConfig;         // 配置参数
    Qmi8658c_Data_T        tData;           // 数据
    DevState_E             eDevState;       // 设备状态
}Qmi8658c_T;

extern Qmi8658c_T   tQmi8658c;

// 接口函数
void    vQmi8658c_IfaceInit(void);
s8      cQmi8658c_Init(void);
s8      cQmi8658c_Config(Qmi8658c_Config_T *p_config);
s8      cQmi8658c_ReadData(Qmi8658c_Data_T *p_data);
s8      cQmi8658c_SetAccRange(Qmi8658c_AccRange_E e_range);
s8      cQmi8658c_SetGyrRange(Qmi8658c_GyrRange_E e_range);
float   fQmi8658c_GetTemp(void);

#endif  //BSP_QMI8658C_H_