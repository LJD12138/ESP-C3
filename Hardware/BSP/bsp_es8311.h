/***********************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\BSP
 * File    : bsp_es8311.h
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : ES8311 音频编解码器驱动
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef BSP_ES8311_H_
#define BSP_ES8311_H_

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"
#include <es8311.h>

#if (1)

//****************************************************Macros********************************************************************//
// ES8311 I2C地址
#define			ES8311_I2C_ADDR							ES8311_ADDRESS_0

//****************************************************Types*********************************************************************//
// MIC增益
typedef es8311_mic_gain_t   Es8311_MicGain_E;
#define			ES8311_MIC_GAIN_0DB						ES8311_MIC_GAIN_0DB
#define			ES8311_MIC_GAIN_6DB						ES8311_MIC_GAIN_6DB
#define			ES8311_MIC_GAIN_12DB					ES8311_MIC_GAIN_12DB
#define			ES8311_MIC_GAIN_18DB					ES8311_MIC_GAIN_18DB
#define			ES8311_MIC_GAIN_24DB					ES8311_MIC_GAIN_24DB
#define			ES8311_MIC_GAIN_30DB					ES8311_MIC_GAIN_30DB
#define			ES8311_MIC_GAIN_36DB					ES8311_MIC_GAIN_36DB
#define			ES8311_MIC_GAIN_42DB					ES8311_MIC_GAIN_42DB

// 分辨率
typedef es8311_resolution_t Es8311_Resolution_E;
#define			ES8311_RESOLUTION_16					ES8311_RESOLUTION_16
#define			ES8311_RESOLUTION_18					ES8311_RESOLUTION_18
#define			ES8311_RESOLUTION_20					ES8311_RESOLUTION_20
#define			ES8311_RESOLUTION_24					ES8311_RESOLUTION_24
#define			ES8311_RESOLUTION_32					ES8311_RESOLUTION_32

// 时钟配置结构
typedef es8311_clock_config_t Es8311_ClockConfig_T;

//****************************************************Extern********************************************************************//
// 控制接口
void    vEs8311_IfaceInit(void);
s8      cEs8311_Init(void);
s8      cEs8311_Config(Es8311_ClockConfig_T *p_clk_cfg, Es8311_Resolution_E e_res_in, Es8311_Resolution_E e_res_out);
s8      cEs8311_SetVolume(u8 uc_vol);
s8      cEs8311_GetVolume(u8 *p_vol);
s8      cEs8311_SetMicGain(Es8311_MicGain_E e_gain);
s8      cEs8311_SetMute(bool b_mute);
s8      cEs8311_ConfigMic(bool b_digital_mic);

// 数据接口
bool                        bEs8311_IsIfaceInited(void);
DevState_E                  eEs8311_GetDevState(void);
bool                        bEs8311_GetMute(void);
es8311_handle_t             hEs8311_GetDevHandle(void);
Es8311_Resolution_E         eEs8311_GetResIn(void);
Es8311_Resolution_E         eEs8311_GetResOut(void);
const Es8311_ClockConfig_T *ptEs8311_GetClkCfg(void);

#endif  /* 1 */

#ifdef __cplusplus
}
#endif

#endif  /* BSP_ES8311_H_ */