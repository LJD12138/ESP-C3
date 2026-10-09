/*****************************************************************************************************************
*                                            ES8311 音频编解码器驱动                                              *
*                                      封装层 - 基于espressif组件库                                              *
******************************************************************************************************************/
#ifndef BSP_ES8311_H_
#define BSP_ES8311_H_

#include "main.h"
#include <es8311.h>

// ES8311 I2C地址
#define     ES8311_I2C_ADDR                  ES8311_ADDRESS_0

// MIC增益
typedef es8311_mic_gain_t   Es8311_MicGain_E;
#define     ES8311_MIC_GAIN_0DB              ES8311_MIC_GAIN_0DB
#define     ES8311_MIC_GAIN_6DB              ES8311_MIC_GAIN_6DB
#define     ES8311_MIC_GAIN_12DB             ES8311_MIC_GAIN_12DB
#define     ES8311_MIC_GAIN_18DB             ES8311_MIC_GAIN_18DB
#define     ES8311_MIC_GAIN_24DB             ES8311_MIC_GAIN_24DB
#define     ES8311_MIC_GAIN_30DB             ES8311_MIC_GAIN_30DB
#define     ES8311_MIC_GAIN_36DB             ES8311_MIC_GAIN_36DB
#define     ES8311_MIC_GAIN_42DB             ES8311_MIC_GAIN_42DB

// 分辨率
typedef es8311_resolution_t Es8311_Resolution_E;
#define     ES8311_RESOLUTION_16             ES8311_RESOLUTION_16
#define     ES8311_RESOLUTION_18             ES8311_RESOLUTION_18
#define     ES8311_RESOLUTION_20             ES8311_RESOLUTION_20
#define     ES8311_RESOLUTION_24             ES8311_RESOLUTION_24
#define     ES8311_RESOLUTION_32             ES8311_RESOLUTION_32

// 时钟配置结构
typedef es8311_clock_config_t Es8311_ClockConfig_T;

// ES8311对象结构
typedef struct
{
    es8311_handle_t      hDev;            // 设备句柄
    Es8311_ClockConfig_T tClkCfg;         // 时钟配置
    Es8311_Resolution_E  eResIn;          // 输入分辨率
    Es8311_Resolution_E  eResOut;         // 输出分辨率
    DevState_E           eDevState;       // 设备状态
    bool                 bMuteEn;         // 静音使能
}Es8311_T;

extern Es8311_T   tEs8311;

// 接口函数
void    vEs8311_IfaceInit(void);
s8      cEs8311_Init(void);
s8      cEs8311_Config(Es8311_ClockConfig_T *p_clk_cfg, Es8311_Resolution_E e_res_in, Es8311_Resolution_E e_res_out);
s8      cEs8311_SetVolume(u8 uc_vol);
s8      cEs8311_GetVolume(u8 *p_vol);
s8      cEs8311_SetMicGain(Es8311_MicGain_E e_gain);
s8      cEs8311_SetMute(bool b_mute);
s8      cEs8311_ConfigMic(bool b_digital_mic);

#endif  //BSP_ES8311_H_