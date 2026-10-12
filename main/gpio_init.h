/***********************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\main
 * File    : gpio_init.h
 * Date    : 2026-10-11
 * Author  : LJD(291483914@qq.com)
 * Desc    : 板载通用 GPIO 引脚映射及各串口外设硬件定义头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef GPIO_INIT_H_
#define GPIO_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#if (1)
//****************************************************Macros********************************************************************//
#define			gpioASSIST_OPEN_PIN						GPIO_NUM_0
#define			gpioASSIST_OPEN_ON()					gpio_set_level(gpioASSIST_OPEN_PIN, 1)
#define			gpioASSIST_OPEN_OFF()					gpio_set_level(gpioASSIST_OPEN_PIN, 0)

/* USART0 (默认调试控制台引脚) */
#define			gpioUSART0_REMAP_EN						0
#define			gpioUSART0_GPIO_RX_PIN					UART_PIN_NO_CHANGE
#define			gpioUSART0_GPIO_TX_PIN					UART_PIN_NO_CHANGE

/* USART1 */
#define			gpioUSART1_GPIO_RX_PIN					GPIO_NUM_26
#define			gpioUSART1_GPIO_TX_PIN					GPIO_NUM_25

/* USART2 */
#define			gpioUSART2_GPIO_RX_PIN					GPIO_NUM_12
#define			gpioUSART2_GPIO_TX_PIN					GPIO_NUM_13

//****************************************************Types*********************************************************************//

//****************************************************Globals*******************************************************************//

//****************************************************Extern********************************************************************//
void vGPIO_Init(void);
bool vGPIO_AssistBmsOpen(bool en);

#if (boardLOW_POWER)
void vGPIO_EnterLowPower(void);
void vGPIO_ExitLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* 1 */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* GPIO_INIT_H_ */
