/***********************************************************************************************************************
-----@file sw6303_common.h
-----函数功能   SW6303 芯片通用驱动
-----作者       LJD
-----版本 1.0.0
-----日期       2026-01-22
-----@copyright Copyright (c) 2026  深圳市百酷新能源有限公司
-----@par 修改日志:
************************************************************************************************************************/
#ifndef USB_SW6303_COMMON_H_
#define USB_SW6303_COMMON_H_

#include "board_config.h"
#include "i2c2.h"

// SW6303 寄存器地址定义
#define SW6303_REG_ABNORMAL_EVENT          0x15    // 异常事件指示
#define SW6303_REG_SYS_STATUS              0x18    // 系统状态指示
#define SW6303_REG_SYS_ABNORMAL_STATUS0    0x2A    // 系统异常状态指示0
#define SW6303_REG_SYS_ABNORMAL_STATUS1    0x2B    // 系统异常状态指示1
#define SW6303_REG_SYS_ABNORMAL_STATUS2    0x2C    // 系统异常状态指示2
#define SW6303_REG_LOW_POWER_SET           0x23    // 低功耗设置
#define SW6303_REG_I2C_ENABLE              0x24    // I2C使能
#define SW6303_REG_PORT_STATUS             0x1D    // 端口状态指示
#define SW6303_REG_CTRG_MODE               0x28    // 模式设置
#define SW6303_REG_ADC_CONFIG              0x30    // ADC配置
#define SW6303_REG_ADC_DATA_LOW            0x31    // ADC数据低8位
#define SW6303_REG_ADC_DATA_HIGH           0x32    // ADC数据高4位
#define SW6303_REG_CTRG_FORCECTL           0x40U   // 强制控制使能
#define SW6303_REG_CTRG_CHG_VOLT_LOW       0x46U   // 充电目标电压低8位
#define SW6303_REG_CTRG_CHG_VOLT_HIGH      0x47U   // 充电目标电压高4位
#define SW6303_REG_CTRG_DCHG_IBAT          0x44U   // 输出电池限流
#define SW6303_REG_CTRG_PISET              0x45U   // 输入功率
#define SW6303_REG_CTRG_CHG_IBAT           0x4AU   // 充电电池限流
#define SW6303_REG_CTRG_POSET              0x4FU   // 输出功率
#define SW6303_REG_STRG_POMAX             0x51U   // 放电最大功率
#define SW6303_REG_STRG_PIMAX             0x52U   // 充电最大功率
#define SW6303_REG_STRG_CHG_CFG0         0x107U  // 充电配置0
#define SW6303_REG_STRG_CHG_CFG1         0x108U  // 充电配置1
#define SW6303_REG_STRG_CHG_CFG2         0x109U  // 充电配置2
#define SW6303_REG_STRG_CHG_CFG3         0x10AU  // 充电配置3
#define SW6303_REG_STRG_CHG_CFG4         0x10BU  // 充电配置4
#define SW6303_REG_STRG_CHG_CFG5         0x10CU  // 充电配置5
#define SW6303_REG_STRG_CHG_CFG6         0x10DU  // 充电配置6
#define SW6303_REG_STRG_CHG_CFG7         0x10EU  // 充电配置7
#define SW6303_REG_STRG_CHG_CFG8         0x10FU  // 充电配置8
#define SW6303_REG_STRG_DCHG_CFG0        0x100U  // 放电配置0
#define SW6303_REG_STRG_DCHG_CFG1        0x101U  // 放电配置1
#define SW6303_REG_STRG_DCHG_CFG2        0x102U  // 放电配置2
#define SW6303_REG_STRG_DCHG_CFG3        0x103U  // 放电配置3
#define SW6303_REG_STRG_DCHG_CFG4        0x104U  // 放电配置4
#define SW6303_REG_STRG_DCHG_CFG5        0x106U  // 放电配置5
#define SW6303_REG_TYPEC_SET1             0x132   // TYPEC设置1
#define SW6303_REG_I2C_ENABLE_8BIT_CLR    0x1FF   // 清除8位地址bit（向REG0x1FF写0）

// ADC数据类型选择（REG0x30 bit[4:0]）
typedef enum
{
    ADC_SEL_VBUS = 0,     // 输入输出电压（8mV）
    ADC_SEL_IBUS,         // 输入/输出IBUS电流（4mA）
    ADC_SEL_VBAT,         // 电池电压（7mV）
    ADC_SEL_IBAT,         // 输入/输出IBAT电流（5mA）
    ADC_SEL_TEMP,         // 芯片温度(1/6.82°C) - 值为9
    ADC_SEL_NTC,          // NTC电压（1.1mV） - 值为10
}ADC_Sel_E;

// 强制控制参数结构
#pragma pack(1)
typedef struct
{
    vu16                usVbus;         // 1mV
    vu16                usChgVolt;      // 1mV
    vu16                usMaxInPwr;     // 1W
    vu16                usChgHoldVolt;  // 0.1V
    vu16                temp;
    vu16                usIbat;         // 0.1A
    vu16                usIbus;         // 0.01A
    vu16                usMaxOutPwr;    // 1W
}SW6303_ForceCtrl_T;
#pragma pack()

// TYPEC角色定义
typedef enum
{
    SW6303_TYPEC_ROLE_DRP = 0,    // 双角色
    SW6303_TYPEC_ROLE_SRC = 1,    // 放电
    SW6303_TYPEC_ROLE_SNK = 2,    // 充电
}SW6303_TYPEC_ROLE_E;

// 模式设置
typedef enum : u8
{
    SW6303_MODE_ALL = 0,      // 充放电
    SW6303_MODE_DISCHG,       // 放电
    SW6303_MODE_CHG,          // 充电
}SW6303_Mode_E;

// SW6303控制对象定义
typedef enum : u8
{
    SW6303_CTRL_OBJ_VBUS = 0,      // 输出电压
    SW6303_CTRL_OBJ_CHG_VOLT,      // 充电电压
    SW6303_CTRL_OBJ_MAX_IN_PWR,    // 最大输入功率
    SW6303_CTRL_OBJ_CHG_HOLD_VOLT, // 充电 hold 门限
    SW6303_CTRL_OBJ_TEMP,         // 预留
    SW6303_CTRL_OBJ_IBAT,         // 电池限流
    SW6303_CTRL_OBJ_IBUS,         // 输出电流
    SW6303_CTRL_OBJ_MAX_OUT_PWR,  // 最大输出功率
    SW6303_CTRL_OBJ_ALL,          // 设置所有
    SW6303_CTRL_OBJ_MAX
}SW6303_CtrlObj_E;

// SW6303放电配置对象定义
typedef enum : u8
{
    SW6303_DCHG_CFG_OBJ_ALL = 0,        // 配置所有放电寄存器
    SW6303_DCHG_CFG_OBJ_CFG0,           // 放电配置0 (REG0x100)
    SW6303_DCHG_CFG_OBJ_CFG1,           // 放电配置1 (REG0x101)
    SW6303_DCHG_CFG_OBJ_CFG2,           // 放电配置2 (REG0x102)
    SW6303_DCHG_CFG_OBJ_CFG3,           // 放电配置3 (REG0x103)
    SW6303_DCHG_CFG_OBJ_CFG4,           // 放电配置4 (REG0x104)
    SW6303_DCHG_CFG_OBJ_CFG5,           // 放电配置5 (REG0x106)
    SW6303_DCHG_CFG_OBJ_MAX
}SW6303_DchgCfgObj_E;

// 放电恒温环阈值定义
typedef enum : u8
{
    SW6303_DCHG_TEMP_THR_60C = 0,   // 60°C
    SW6303_DCHG_TEMP_THR_70C,       // 70°C
    SW6303_DCHG_TEMP_THR_80C,       // 80°C
    SW6303_DCHG_TEMP_THR_90C,       // 90°C
    SW6303_DCHG_TEMP_THR_100C,      // 100°C
    SW6303_DCHG_TEMP_THR_110C,      // 110°C
    SW6303_DCHG_TEMP_THR_120C,      // 120°C
    SW6303_DCHG_TEMP_THR_130C,      // 130°C
}SW6303_DchgTempThr_E;

// 输出功率设置定义
typedef enum : u8
{
    SW6303_OUT_PWR_27W = 0,    // 27W
    SW6303_OUT_PWR_30W,        // 30W
    SW6303_OUT_PWR_35W,        // 35W
    SW6303_OUT_PWR_45W,        // 45W
    SW6303_OUT_PWR_60W,        // 60W
    SW6303_OUT_PWR_65W,        // 65W
    SW6303_OUT_PWR_100W,       // 100W
    SW6303_OUT_PWR_RESERVED,    // reserved
}SW6303_OutPwr_E;

// 多口输出最大电流定义
typedef enum : u8
{
    SW6303_MULTI_PORT_MAX_I_4_2A = 0,  // 4.2A
    SW6303_MULTI_PORT_MAX_I_3_0A,      // 3.0A
    SW6303_MULTI_PORT_MAX_I_5_2A,      // 5.2A
    SW6303_MULTI_PORT_MAX_I_6_2A,      // 6.2A
}SW6303_MultiPortMaxI_E;

// VBUS限流固定偏移定义
typedef enum : u8
{
    SW6303_VBUS_I_OFFSET_300MA = 0,    // 300mA
    SW6303_VBUS_I_OFFSET_150MA,        // 150mA
    SW6303_VBUS_I_OFFSET_450MA,        // 450mA
    SW6303_VBUS_I_OFFSET_600MA,        // 600mA
}SW6303_VbusIOffset_E;

// 输出电压固定偏移定义
typedef enum : u8
{
    SW6303_VBUS_V_OFFSET_100MV = 0,    // 100mV
    SW6303_VBUS_V_OFFSET_0MV,          // 0mV
    SW6303_VBUS_V_OFFSET_200MV,        // 200mV
    SW6303_VBUS_V_OFFSET_300MV,        // 300mV
}SW6303_VbusVOffset_E;

// 线补设置定义
typedef enum : u8
{
    SW6303_LINE_COMP_60MV_A = 0,    // 60mV/A
    SW6303_LINE_COMP_0MV_A,        // 0mV/A
    SW6303_LINE_COMP_100MV_A,      // 100mV/A
    SW6303_LINE_COMP_80MV_A,       // 80mV/A
}SW6303_LineComp_E;

// 放电IBAT限流默认值定义
typedef enum : u8
{
    SW6303_DCHG_IBAT_12A = 0,  // 12A
    SW6303_DCHG_IBAT_10A,      // 10A
    SW6303_DCHG_IBAT_8A,       // 8A
    SW6303_DCHG_IBAT_INVALID,  // 不生效
}SW6303_DchgIbatDef_E;

// UVLO迟滞定义 (N为电池节数)
typedef enum : u8
{
    SW6303_UVLO_HYST_0_4V = 0,   // 0.4V*N
    SW6303_UVLO_HYST_0_1V,       // 0.1V*N
    SW6303_UVLO_HYST_0_2V,       // 0.2V*N
    SW6303_UVLO_HYST_0_3V,       // 0.3V*N
    SW6303_UVLO_HYST_0_5V,       // 0.5V*N
    SW6303_UVLO_HYST_0_6V,       // 0.6V*N
    SW6303_UVLO_HYST_0V,         // 0V
    SW6303_UVLO_HYST_0_8V,       // 0.8V*N
}SW6303_UvloHyst_E;

// 三元锂电池欠压门限定义 (N为电池节数)
typedef enum : u8
{
    SW6303_LIPO_UVLO_3_0V = 0,   // 3.0V*N
    SW6303_LIPO_UVLO_2_6V,       // 2.6V*N
    SW6303_LIPO_UVLO_2_7V,       // 2.7V*N
    SW6303_LIPO_UVLO_2_8V,       // 2.8V*N
    SW6303_LIPO_UVLO_2_9V,       // 2.9V*N
    SW6303_LIPO_UVLO_3_1V,       // 3.1V*N
    SW6303_LIPO_UVLO_3_2V,       // 3.2V*N
    SW6303_LIPO_UVLO_3_3V,       // 3.3V*N
}SW6303_LipoUvlo_E;

// 电感设置定义
typedef enum : u8
{
    SW6303_INDUCTOR_4_7UH = 0,   // 4.7uH
    SW6303_INDUCTOR_10UH,        // 10uH
}SW6303_Inductor_E;

// 放电NTC高温门限定义
typedef enum : u8
{
    SW6303_DCHG_NTC_HIGH_60C = 0,   // 60°C
    SW6303_DCHG_NTC_HIGH_50C,       // 50°C
    SW6303_DCHG_NTC_HIGH_55C,       // 55°C
    SW6303_DCHG_NTC_HIGH_65C,       // 65°C
}SW6303_DchgNtcHigh_E;

// 放电NTC低温门限定义
typedef enum : u8
{
    SW6303_DCHG_NTC_LOW_M20C = 0,   // -20°C
    SW6303_DCHG_NTC_LOW_M15C,       // -15°C
    SW6303_DCHG_NTC_LOW_M10C,       // -10°C
    SW6303_DCHG_NTC_LOW_M5C,        // -5°C
    SW6303_DCHG_NTC_LOW_0C,         // 0°C
    SW6303_DCHG_NTC_LOW_INVALID,    // 不生效
    SW6303_DCHG_NTC_LOW_M25C,       // -25°C
    SW6303_DCHG_NTC_LOW_M30C,       // -30°C
}SW6303_DchgNtcLow_E;

// 充电配置0: 输入最高功率设置
typedef enum : u8
{
    SW6303_CHG_IN_PWR_27W = 0,    // 27W
    SW6303_CHG_IN_PWR_30W,        // 30W
    SW6303_CHG_IN_PWR_35W,        // 35W
    SW6303_CHG_IN_PWR_45W,        // 45W
    SW6303_CHG_IN_PWR_60W,        // 60W
    SW6303_CHG_IN_PWR_65W,        // 65W
    SW6303_CHG_IN_PWR_100W,       // 100W
    SW6303_CHG_IN_PWR_RESERVED,    // reserved
}SW6303_ChgInPwr_E;

// 电池类型设置
typedef enum : u8
{
    SW6303_BAT_TYPE_4_2V = 0,    // 4.2V
    SW6303_BAT_TYPE_4_3V,          // 4.3V
    SW6303_BAT_TYPE_4_35V,        // 4.35V
    SW6303_BAT_TYPE_4_4V,         // 4.4V
    SW6303_BAT_TYPE_4_5V,         // 4.5V
    SW6303_BAT_TYPE_3_6V,         // 3.6V, 磷酸铁锂
    SW6303_BAT_TYPE_3_65V_1,      // 3.65V, 磷酸铁锂
    SW6303_BAT_TYPE_3_65V_2,      // 3.65V, 磷酸铁锂
}SW6303_BatType_E;

// 电池节数设置
typedef enum : u8
{
    SW6303_BAT_CELL_2 = 0,    // 2节
    SW6303_BAT_CELL_2_2,    // 2节
    SW6303_BAT_CELL_2_3,    // 2节
    SW6303_BAT_CELL_3,       // 3节
    SW6303_BAT_CELL_4,       // 4节
    SW6303_BAT_CELL_5,       // 5节
    SW6303_BAT_CELL_6,       // 6节 (不支持4.5V电池类型)
    SW6303_BAT_CELL_7,       // 7节 (仅支持磷酸铁锂电池)
}SW6303_BatCell_E;

// 5V限压门限设置
typedef enum : u8
{
    SW6303_5V_OVP_4_6V = 0,    // 4.6V
    SW6303_5V_OVP_4_2V,         // 4.2V
    SW6303_5V_OVP_4_3V,         // 4.3V
    SW6303_5V_OVP_4_4V,         // 4.4V
    SW6303_5V_OVP_4_5V,         // 4.5V
    SW6303_5V_OVP_4_7V,         // 4.7V
    SW6303_5V_OVP_4_8V,         // 4.8V
    SW6303_5V_OVP_4_9V,         // 4.9V
}SW6303_5V_Ovp_E;

// 9V限压门限设置
typedef enum : u8
{
    SW6303_9V_OVP_8_5V = 0,    // 8.5V
    SW6303_9V_OVP_8_2V,         // 8.2V
    SW6303_9V_OVP_8_3V,         // 8.3V
    SW6303_9V_OVP_8_4V,         // 8.4V
    SW6303_9V_OVP_8_6V,         // 8.6V
    SW6303_9V_OVP_8_7V,         // 8.7V
    SW6303_9V_OVP_8_8V,         // 8.8V
    SW6303_9V_OVP_8_9V,         // 8.9V
}SW6303_9V_Ovp_E;

// 10V限压门限设置
typedef enum : u8
{
    SW6303_10V_OVP_9_5V = 0,   // 9.5V
    SW6303_10V_OVP_9_2V,        // 9.2V
    SW6303_10V_OVP_9_3V,        // 9.3V
    SW6303_10V_OVP_9_4V,        // 9.4V
    SW6303_10V_OVP_9_6V,        // 9.6V
    SW6303_10V_OVP_9_7V,        // 9.7V
    SW6303_10V_OVP_9_8V,        // 9.8V
    SW6303_10V_OVP_9_9V,        // 9.9V
}SW6303_10V_Ovp_E;

// 12V限压门限设置
typedef enum : u8
{
    SW6303_12V_OVP_11_5V = 0,   // 11.5V
    SW6303_12V_OVP_11_2V,        // 11.2V
    SW6303_12V_OVP_11_3V,        // 11.3V
    SW6303_12V_OVP_11_4V,        // 11.4V
    SW6303_12V_OVP_11_6V,        // 11.6V
    SW6303_12V_OVP_11_7V,        // 11.7V
    SW6303_12V_OVP_11_8V,        // 11.8V
    SW6303_12V_OVP_11_9V,        // 11.9V
}SW6303_12V_Ovp_E;

// 15V限压门限设置
typedef enum : u8
{
    SW6303_15V_OVP_14_5V = 0,   // 14.5V
    SW6303_15V_OVP_14_4V,        // 14.4V
    SW6303_15V_OVP_14_3V,        // 14.3V
    SW6303_15V_OVP_14_2V,        // 14.2V
    SW6303_15V_OVP_14_1V,        // 14.1V
    SW6303_15V_OVP_14_0V,        // 14.0V
    SW6303_15V_OVP_13_9V,        // 13.9V
    SW6303_15V_OVP_13_8V,        // 13.8V
}SW6303_15V_Ovp_E;

// 20V限压门限设置
typedef enum : u8
{
    SW6303_20V_OVP_19_5V = 0,   // 19.5V
    SW6303_20V_OVP_19_4V,        // 19.4V
    SW6303_20V_OVP_19_3V,        // 19.3V
    SW6303_20V_OVP_19_2V,        // 19.2V
    SW6303_20V_OVP_19_1V,        // 19.1V
    SW6303_20V_OVP_19_0V,        // 19.0V
    SW6303_20V_OVP_18_9V,        // 18.9V
    SW6303_20V_OVP_18_8V,        // 18.8V
}SW6303_20V_Ovp_E;

// 磷酸铁锂电池涓流充电门限设置 (N为电池节数)
typedef enum : u8
{
    SW6303_FE_TRICKLE_2_75V = 0,   // 2.75V*N
    SW6303_FE_TRICKLE_2_1V,        // 2.1V*N
    SW6303_FE_TRICKLE_2_5V,        // 2.5V*N
    SW6303_FE_TRICKLE_2_8V,        // 2.8V*N
}SW6303_FeTrickle_E;

// 涓流充电电流设置
typedef enum : u8
{
    SW6303_TRICKLE_I_100MA = 0,   // 100mA
    SW6303_TRICKLE_I_200MA,       // 200mA
    SW6303_TRICKLE_I_300MA,       // 300mA
    SW6303_TRICKLE_I_400MA,       // 400mA
}SW6303_TrickleI_E;

// 涓流充电电压迟滞设置 (N为电池节数)
typedef enum : u8
{
    SW6303_TRICKLE_HYST_0_1V = 0,   // 0.1V*N
    SW6303_TRICKLE_HYST_0_2V,       // 0.2V*N
    SW6303_TRICKLE_HYST_0_3V,       // 0.3V*N
    SW6303_TRICKLE_HYST_0_4V,       // 0.4V*N
}SW6303_TrickleHyst_E;

// 恒流充电超时设置
typedef enum : u8
{
    SW6303_CC_TIMEOUT_22H = 0,    // 22h
    SW6303_CC_TIMEOUT_33H,        // 33h
    SW6303_CC_TIMEOUT_48H,        // 48h
    SW6303_CC_TIMEOUT_INVALID,    // 关闭恒流充电超时
}SW6303_CcTimeout_E;

// 充电截至电流设置
typedef enum : u8
{
    SW6303_CHG_END_I_200MA = 0,   // 200mA
    SW6303_CHG_END_I_100MA,       // 100mA
    SW6303_CHG_END_I_300MA,       // 300mA
    SW6303_CHG_END_I_400MA,       // 400mA
}SW6303_ChgEndI_E;

// C口5V充电电流设置
typedef enum : u8
{
    SW6303_C_5V_I_3_0A = 0,    // 3.0A
    SW6303_C_5V_I_1_8A,         // 1.8A
    SW6303_C_5V_I_1_9A,         // 1.9A
    SW6303_C_5V_I_2_0A,         // 2.0A
    SW6303_C_5V_I_2_1A,         // 2.1A
    SW6303_C_5V_I_2_2A,         // 2.2A
    SW6303_C_5V_I_2_3A,         // 2.3A
    SW6303_C_5V_I_2_4A,         // 2.4A
    SW6303_C_5V_I_2_5A,         // 2.5A
    SW6303_C_5V_I_2_6A,         // 2.6A
    SW6303_C_5V_I_2_7A,         // 2.7A
    SW6303_C_5V_I_2_8A,         // 2.8A
    SW6303_C_5V_I_2_9A,         // 2.9A
    SW6303_C_5V_I_3_1A,         // 3.1A
    SW6303_C_5V_I_3_2A,         // 3.2A
    SW6303_C_5V_I_3_3A,         // 3.3A
}SW6303_C5V_I_E;

// B口和L口5V充电电流设置
typedef enum : u8
{
    SW6303_BL_5V_I_2_0A = 0,    // 2.0A
    SW6303_BL_5V_I_1_6A,        // 1.6A
    SW6303_BL_5V_I_1_7A,        // 1.7A
    SW6303_BL_5V_I_1_8A,        // 1.8A
    SW6303_BL_5V_I_1_9A,        // 1.9A
    SW6303_BL_5V_I_2_1A,        // 2.1A
    SW6303_BL_5V_I_2_2A,        // 2.2A
    SW6303_BL_5V_I_2_3A,        // 2.3A
}SW6303_BL5V_I_E;

// 充电NTC低温保护门限设置
typedef enum : u8
{
    SW6303_CHG_NTC_LOW_0C = 0,    // 0°C
    SW6303_CHG_NTC_LOW_5C,         // 5°C
    SW6303_CHG_NTC_LOW_10C,        // 10°C
    SW6303_CHG_NTC_LOW_15C,        // 15°C
    SW6303_CHG_NTC_LOW_20C,        // 20°C
    SW6303_CHG_NTC_LOW_M5C,       // -5°C
    SW6303_CHG_NTC_LOW_M10C,       // -10°C
    SW6303_CHG_NTC_LOW_INVALID,    // 不生效
}SW6303_ChgNtcLow_E;

// 充电NTC高温保护门限设置
typedef enum : u8
{
    SW6303_CHG_NTC_HIGH_45C = 0,   // 45°C
    SW6303_CHG_NTC_HIGH_50C,       // 50°C
    SW6303_CHG_NTC_HIGH_55C,       // 55°C
    SW6303_CHG_NTC_HIGH_60C,       // 60°C
}SW6303_ChgNtcHigh_E;

// 边充边放设置定义
typedef enum : u8
{
    SW6303_BYPASS_ALLOW = 0,    // 允许5V边充边放
    SW6303_BYPASS_CHG_PRIORITY, // 充电优先，禁止5V边充边放
}SW6303_Bypass_E;

// SW6303充电配置对象定义
typedef enum : u8
{
    SW6303_CHG_CFG_OBJ_ALL = 0,         // 配置所有充电寄存器
    SW6303_CHG_CFG_OBJ_CFG0,            // 充电配置0 (REG0x107)
    SW6303_CHG_CFG_OBJ_CFG1,            // 充电配置1 (REG0x108)
    SW6303_CHG_CFG_OBJ_CFG2,            // 充电配置2 (REG0x109)
    SW6303_CHG_CFG_OBJ_CFG3,            // 充电配置3 (REG0x10A)
    SW6303_CHG_CFG_OBJ_CFG4,            // 充电配置4 (REG0x10B)
    SW6303_CHG_CFG_OBJ_CFG5,            // 充电配置5 (REG0x10C)
    SW6303_CHG_CFG_OBJ_CFG6,            // 充电配置6 (REG0x10D)
    SW6303_CHG_CFG_OBJ_CFG7,            // 充电配置7 (REG0x10E)
    SW6303_CHG_CFG_OBJ_CFG8,            // 充电配置8 (REG0x10F)
    SW6303_CHG_CFG_OBJ_MAX
}SW6303_ChgCfgObj_E;

// 充电配置参数结构
#pragma pack(1)
typedef struct
{
    // REG0x107: 充电配置0
    bool                bFcFixPower;          // 快充是否按照固定功率充电 (bit4: 0-按最大电流, 1-按固定功率)
    bool                bAutoDetectInPwr;      // 输入最高功率设置方式 (bit3: 0-自动, 1-手动设置)
    SW6303_ChgInPwr_E   eInPwr;              // 输入最高功率设置 (bit[2:0])
    
    // REG0x108: 充电配置1
    bool                bAutoDetectBatType;    // 电池类型设置方式 (bit7: 0-自动, 1-手动设置)
    SW6303_BatType_E    eBatType;            // 电池类型设置 (bit[6:4])
    bool                bAutoDetectBatCell;    // 电池节数设置方式 (bit3: 0-自动, 1-手动设置)
    SW6303_BatCell_E    eBatCell;            // 电池节数设置 (bit[2:0])
    
    // REG0x109: 充电配置2
    bool                bChgAbove1_5V;        // 电池电压低于1.5V时是否充电 (bit7: 0-允许, 1-不允许)
    SW6303_5V_Ovp_E     e5VOvp;              // 5V限压门限设置 (bit[6:4])
    bool                bTrickleTimeout;      // 涓流充电超时设置 (bit3: 0-40分钟, 1-关闭超时)
    SW6303_9V_Ovp_E     e9VOvp;              // 9V限压门限设置 (bit[2:0])
    
    // REG0x10A: 充电配置3
    bool                bAdapterPullDisable;    // 适配器拉挂使能 (bit7: 0-使能, 1-禁止)
    SW6303_10V_Ovp_E    e10VOvp;             // 10V限压门限设置 (bit[6:4])
    SW6303_Bypass_E     eBypassMode;         // 边充边放设置 (bit3: 0-允许5V边充边放, 1-充电优先，禁止5V边充边放)
    SW6303_12V_Ovp_E    e12VOvp;             // 12V限压门限设置 (bit[2:0])
    
    // REG0x10B: 充电配置4
    bool                bBypassHoldChange;     // 边充边放hold门限设置 (bit7: 0-提高欠压到4.8V, 1-不变)
    SW6303_15V_Ovp_E    e15VOvp;             // 15V限压门限设置 (bit[6:4])
    SW6303_20V_Ovp_E    e20VOvp;             // 20V限压门限设置 (bit[2:0])
    
    // REG0x10C: 充电配置5
    bool                bLimitVoltageAdapt;   // 限压门限自适应 (bit3: 0-电流>3A降低限压, 1-不降低)
    
    // REG0x10D: 充电配置6
    SW6303_FeTrickle_E  eFeTrickle;          // 磷酸铁锂电池涓流充电门限 (bit[7:6])
    SW6303_TrickleI_E   eTrickleI;           // 涓流充电电流 (bit[5:4])
    SW6303_TrickleHyst_E eTrickleHyst;        // 涓流充电电压迟滞 (bit[3:2])
    SW6303_CcTimeout_E  eCcTimeout;          // 恒流充电超时设置 (bit[1:0])
    
    // REG0x10E: 充电配置7
    SW6303_ChgEndI_E   eChgEndI;            // 充电截至电流设置 (bit[7:6])
    SW6303_C5V_I_E     eC5VI;               // C口5V充电电流设置 (bit[3:0])
    
    // REG0x10F: 充电配置8
    SW6303_BL5V_I_E    eBL5VI;              // B口和L口5V充电电流设置 (bit[7:5])
    SW6303_ChgNtcLow_E  eChgNtcLow;         // 充电时NTC低温保护门限 (bit[4:2])
    SW6303_ChgNtcHigh_E eChgNtcHigh;        // 充电时NTC高温保护门限 (bit[1:0])
}SW6303_ChgCfg_T;
#pragma pack()

// 端口状态参数结构
#pragma pack(1)
typedef struct
{
    // C1口状态
    bool            bC1SrcConn;         // C1作为source的连接状态 (bit7: 0-未连接, 1-连接)
    bool            bC1SinkConn;        // C1作为sink的连接状态 (bit6: 0-未连接, 1-连接)
    bool            bC1Online;          // C1口的在线状态 (bit1: 0-不在线, 1-在线)
    
    // C2口状态
    bool            bC2SrcConn;         // C2作为source的连接状态 (bit5: 0-未连接, 1-连接)
    bool            bC2SinkConn;        // C2作为sink的连接状态 (bit4: 0-未连接, 1-连接)
    bool            bC2Online;          // C2口的在线状态 (bit0: 0-不在线, 1-在线)
    
    // A口状态
    bool            bAOnline;           // A口在线状态 (bit2: 0-不在线, 1-在线)
}SW6303_PortStatus_T;
#pragma pack()

// 异常事件类型定义 (REG0x15)
typedef enum : u8
{
    SW6303_EVENT_CAP_CALC_DONE = 0,     // 电量计量完成指示 (bit5)
    SW6303_EVENT_UVLO,                   // 电池欠压UVLO事件指示 (bit4)
    SW6303_EVENT_CHG_ABNORMAL,           // 充电异常事件指示 (bit3)
    SW6303_EVENT_DCHG_ABNORMAL,          // 放电异常事件指示 (bit2)
    SW6303_EVENT_KEY,                    // 按键事件指示 (bit1)
    SW6303_EVENT_SCENE_CHANGE,           // 场景变化事件指示 (bit0)
    SW6303_EVENT_MAX
}SW6303_EventType_E;

// 系统状态结构体 (REG0x18)
#pragma pack(1)
typedef struct
{
    bool            bChgAbnormal;        // 充电异常触发事件指示 (bit7: 0-无异常, 1-异常导致充电关闭)
    bool            bDchgAbnormal;       // 放电异常触发事件指示 (bit6: 0-无异常, 1-异常导致放电关闭)
    bool            bChgStatus;          // 充电状态指示位 (bit5: 0-充电关闭, 1-充电打开)
    bool            bDchgStatus;         // 放电状态指示位 (bit4: 0-放电关闭, 1-放电打开)
    bool            bC1PathStatus;       // C1口通路状态指示位 (bit3: 0-关闭, 1-打开)
    bool            bC2PathStatus;       // C2口通路状态指示位 (bit2: 0-关闭, 1-打开)
    bool            bAPathStatus;        // A口通路状态指示位 (bit0: 0-关闭, 1-打开)
}SW6303_SysStatus_T;
#pragma pack()

// 系统异常状态0结构体 (REG0x2A) - 放电异常
#pragma pack(1)
typedef struct
{
    bool            bDchgBatOvp;         // 放电时电池过压异常 (bit6)
    bool            bDchgChipOtp;        // 放电时芯片过温异常 (bit5)
    bool            bDchgNtcOtp;         // 放电时NTC过温异常 (bit4)
    bool            bDchgVbusOcp;        // 放电时VBUS过载异常 (bit3)
    bool            bDchgVbusShort;      // 放电时VBUS短路异常 (bit2)
    bool            bDchgVbusSlowOvp;    // 放电时VBUS慢速过压保护异常 (bit1)
    bool            bDchgVbusFastOvp;    // 放电时VBUS快速过压保护异常 (bit0)
}SW6303_AbnormalStatus0_T;
#pragma pack()

// 系统异常状态1结构体 (REG0x2B) - 充电异常
#pragma pack(1)
typedef struct
{
    bool            bBatBelow1_5V;       // 电池电压低于1.5V异常 (bit7)
    bool            bChgTimeout;         // 充电超时异常 (bit6)
    bool            bChgFull;            // 充满事件指示 (bit5)
    bool            bChgBatOvp;          // 充电时电池过压异常 (bit4)
    bool            bChgChipOtp;         // 充电时芯片过温异常 (bit3)
    bool            bChgNtcOtp;          // 充电时NTC过温异常 (bit2)
    bool            bChgVbusSlowOvp;     // 充电时VBUS慢速过压异常 (bit1)
    bool            bChgVbusFastOvp;     // 充电时VBUS快速过压异常 (bit0)
}SW6303_AbnormalStatus1_T;
#pragma pack()

// 按键事件类型定义 (REG0x2C bit[7:6])
typedef enum : u8
{
    SW6303_KEY_EVENT_NONE = 0,     // 未发生按键事件
    SW6303_KEY_EVENT_SHORT,        // 发生短按
    SW6303_KEY_EVENT_DOUBLE,       // 发生双击
    SW6303_KEY_EVENT_LONG,         // 发生长按
}SW6303_KeyEvent_E;

// 系统异常状态2结构体 (REG0x2C)
#pragma pack(1)
typedef struct
{
    SW6303_KeyEvent_E   eKeyEvent;          // 按键事件指示 (bit[7:6])
    bool                bDchg62368LowTemp;  // 放电时62368低温异常 (bit4)
    bool                bChg62368LowTemp;   // 充电时62368低温异常 (bit3)
    bool                bChg62368Otp;       // 充电62368过温异常 (bit2)
    bool                bDchgDpdmOvp;       // 放电时DPDM 5.5V过压异常 (bit1)
    bool                bDchgCcOvp;         // 放电时CC 5.5V过压异常 (bit0)
}SW6303_AbnormalStatus2_T;
#pragma pack()

// 异常事件状态结构体 (REG0x15)
#pragma pack(1)
typedef struct
{
    bool            bCapCalcDone;        // 电量计量完成指示 (bit5) - 写1清零
    bool            bUvloEvent;          // 电池欠压UVLO事件指示 (bit4) - 写1清零
    bool            bChgAbnormalEvent;   // 充电异常事件指示 (bit3) - 写1清零
    bool            bDchgAbnormalEvent;  // 放电异常事件指示 (bit2) - 写1清零
    bool            bKeyEvent;           // 按键事件指示 (bit1) - 写1清零
    bool            bSceneChangeEvent;   // 场景变化事件指示 (bit0) - 写1清零
}SW6303_AbnormalEvent_T;
#pragma pack()

// 放电配置参数结构
#pragma pack(1)
typedef struct
{
    // REG0x100: 放电配置0
    bool            bTempLoopEnable;          // 恒温环使能 (bit7: 0-使能, 1-禁止)
    SW6303_DchgTempThr_E   eTempThr;          // 恒温环阈值 (bit[6:4])
    bool            bPwrAutoDetect;           // 输出最大功率自动检测 (bit3: 0-自动, 1-手动设置)
    SW6303_OutPwr_E       eOutPwr;            // 输出功率设置 (bit[2:0])
    
    // REG0x101: 放电配置1
    SW6303_MultiPortMaxI_E  eMultiPortMaxI;   // 多口输出最大电流 (bit[7:6])
    SW6303_VbusIOffset_E    eVbusIOffset;     // VBUS限流固定偏移 (bit[5:4])
    SW6303_VbusVOffset_E    eVbusVOffset;     // 输出电压固定偏移 (bit[3:2])
    SW6303_LineComp_E       eLineComp;        // 线补设置 (bit[1:0])
    
    // REG0x102: 放电配置2
    bool            bFcLineCompEnable;      // 快充模式线补使能 (bit7: 0-使能, 1-禁止)
    bool            bVbusFastOvpEnable;    // VBUS快速过压保护控制 (bit2: 0-打开, 1-关闭)
    
    // REG0x103: 放电配置3
    SW6303_DchgIbatDef_E   eDchgIbatDef;     // 放电IBAT限流默认值 (bit[7:6])
    u8              ucLipoUvloThr;         // 磷酸铁锂电池欠压门限 (bit[3:0], 0:2.75V*N, 1~12:1.9~3.0*N,0.1V/step)
    
    // REG0x104: 放电配置4
    SW6303_UvloHyst_E      eUvloHyst;         // UVLO迟滞 (bit[6:4])
    SW6303_LipoUvlo_E     eLipoUvlo;         // 三元锂电池欠压门限 (bit[2:0])
    
    // REG0x106: 放电配置5
    SW6303_Inductor_E     eInductor;         // 电感设置 (bit7)
    SW6303_DchgNtcHigh_E   eNtcHighThr;      // 放电NTC高温门限 (bit[5:4])
    SW6303_DchgNtcLow_E    eNtcLowThr;       // 放电NTC低温门限 (bit[2:0])
}SW6303_DchgCfg_T;
#pragma pack()

// 通用函数声明
s8 c_usb_sw6303_read_bytes(const I2cObj_T *p_i2c_obj, u16 reg_addr, u8 *buf, u8 len);
s8 c_usb_sw6303_write_bytes(const I2cObj_T *p_i2c_obj, u16 reg_addr, const u8 *buf, u8 len);
s8 c_usb_sw6303_high_reg_addr_enable(const I2cObj_T *p_i2c_obj, bool en);
s8 c_usb_sw6303_write_enable(const I2cObj_T *p_i2c_obj, bool en);
s8 c_usb_sw6303_set_low_power_state(const I2cObj_T *p_i2c_obj, bool en);
s8 c_usb_sw6303_set_typec_role(const I2cObj_T *p_i2c_obj, SW6303_TYPEC_ROLE_E role);
s8 c_usb_sw6303_set_force_ctrl_param(const I2cObj_T *p_i2c_obj, SW6303_CtrlObj_E obj, SW6303_ForceCtrl_T *param);
s8 c_usb_sw6303_set_chg_cfg(const I2cObj_T *p_i2c_obj, SW6303_ChgCfgObj_E obj, SW6303_ChgCfg_T *param);
s8 c_usb_sw6303_set_dischg_cfg(const I2cObj_T *p_i2c_obj, SW6303_DchgCfgObj_E obj, SW6303_DchgCfg_T *param);
s8 c_usb_sw6303_set_work_mode(const I2cObj_T *p_i2c_obj, SW6303_Mode_E mode, bool en);
s16 s_usb_sw6303_get_adc_data(const I2cObj_T *p_i2c_obj, ADC_Sel_E adc_sel);
s8 c_usb_sw6303_reset(const I2cObj_T *p_i2c_obj);
s8 c_usb_sw6303_get_port_status(const I2cObj_T *p_i2c_obj, SW6303_PortStatus_T *p_status);
s16 s_usb_sw6303_get_chg_target_volt(const I2cObj_T *p_i2c_obj);

// 异常事件相关函数声明
s8 c_usb_sw6303_get_sys_status(const I2cObj_T *p_i2c_obj, SW6303_SysStatus_T *p_status);
s8 c_usb_sw6303_get_abnormal_event(const I2cObj_T *p_i2c_obj, SW6303_AbnormalEvent_T *p_event);
s8 c_usb_sw6303_clear_abnormal_event(const I2cObj_T *p_i2c_obj, SW6303_EventType_E event_type);
s8 c_usb_sw6303_get_abnormal_status0(const I2cObj_T *p_i2c_obj, SW6303_AbnormalStatus0_T *p_status);
s8 c_usb_sw6303_get_abnormal_status1(const I2cObj_T *p_i2c_obj, SW6303_AbnormalStatus1_T *p_status);
s8 c_usb_sw6303_get_abnormal_status2(const I2cObj_T *p_i2c_obj, SW6303_AbnormalStatus2_T *p_status);

#endif  // USB_SW6303_COMMON_H_

