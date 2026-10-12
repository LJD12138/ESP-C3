/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\Key
 * File    : key_iface.c
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : 按键硬件驱动底层接口实现文件 (ESP32-C3 GPIO驱动)
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Key/key_iface.h"

#if (boardKEY_EN)
#include "driver/gpio.h"


//****************************************************Parameter Initialization**************************************************//
typedef struct
{
	uint32_t			gpio;				/* GPIO 端口 */
	uint32_t			mode;				/* GPIO 模式 */
	uint32_t			pull;               /* GPIO 上下拉模式 */
	bool				bPressHigh;			/* 按下有效电平: true=高电平按下, false=低电平按下 */
	const char			*pName;				/* 按键调试名称 */
}KeyCfgGpio_T;

/* 按键引脚硬件配置表: 顺序需严格对应 KeyId_E */
static const KeyCfgGpio_T s_t_key_hw_config[keyNUM] =
{
	[keyPOWER] = {
		.gpio       = GPIO_NUM_9,
		.mode       = GPIO_MODE_INPUT,
		.pull       = GPIO_PULLUP_ONLY,
		.bPressHigh = false,
		.pName      = "Power",
	},
};

#define			KEY_HW_NUM								(sizeof(s_t_key_hw_config) / sizeof(s_t_key_hw_config[0]))

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 按键 GPIO 初始化
 * 说明(备注)  : 根据配置表统一配置 ESP32-C3 GPIO 输入模式及上下拉；未连接引脚自动跳过
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vKey_IfaceInit(void)
{
	for (uint8_t i = 0; i < KEY_HW_NUM; ++i)
	{
		if (s_t_key_hw_config[i].gpio < 0 || s_t_key_hw_config[i].gpio == GPIO_NUM_NC)
			continue;

		gpio_config_t io_conf = {
			.pin_bit_mask = (1ULL << (uint32_t)s_t_key_hw_config[i].gpio),
			.mode         = s_t_key_hw_config[i].mode,
			.pull_up_en   = (s_t_key_hw_config[i].pull == GPIO_PULLUP_ONLY || s_t_key_hw_config[i].pull == GPIO_PULLUP_PULLDOWN) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
			.pull_down_en = (s_t_key_hw_config[i].pull == GPIO_PULLDOWN_ONLY || s_t_key_hw_config[i].pull == GPIO_PULLUP_PULLDOWN) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
			.intr_type    = GPIO_INTR_DISABLE,
		};
		gpio_config(&io_conf);
	}
}

/***********************************************************************************************************************
 * 函数功能    : 查表读取按键电平状态
 * 说明(备注)  : 硬件层唯一电平数据源，极性归一化；按下返回 true，未按下或越界返回 false
 * 传入参数    : e_id: 按键ID枚举
 * 输出参数    : 无
 * 返回值      : bool: true 按下, false 未按下
 ************************************************************************************************************************/
bool bKey_IsPressById(KeyId_E e_id)
{
	uint8_t uc_idx = (uint8_t)e_id;
	if (uc_idx >= KEY_HW_NUM)
		return false;

	if (s_t_key_hw_config[uc_idx].gpio < 0 || s_t_key_hw_config[uc_idx].gpio == GPIO_NUM_NC)
		return false;

	bool b_pin_level = (gpio_get_level(s_t_key_hw_config[uc_idx].gpio) != 0);
	return (b_pin_level == s_t_key_hw_config[uc_idx].bPressHigh);
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 按键引脚及外设进入低功耗状态
 * 说明(备注)  : 配置唤醒源引脚低电平唤醒
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vKey_IoEnterLowPower(void)
{
	#if (keyGPIO_POWER_PIN != GPIO_NUM_NC)
	gpio_wakeup_enable(keyGPIO_POWER_PIN, GPIO_INTR_LOW_LEVEL);
	#endif  /* keyGPIO_POWER_PIN != GPIO_NUM_NC */
}

/***********************************************************************************************************************
 * 函数功能    : 按键引脚退出低功耗状态
 * 说明(备注)  : 重新初始化按键 GPIO
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vKey_IoExitLowPower(void)
{
	vKey_IfaceInit();
}
#endif  /* boardLOW_POWER */

#endif  /* boardKEY_EN */

