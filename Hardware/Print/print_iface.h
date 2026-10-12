/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_iface.h
 * Date    : 2026-09-23
 * Author  : LJD(291483914@qq.com)
 * Desc    : 打印与串口底层硬件接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef PRINT_IFACE_H_
#define PRINT_IFACE_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardPRINT_IFACE)
#include "gpio_init.h"
#include "driver/uart.h"
#include "driver/gpio.h"

//****************************************************Macros********************************************************************//
#if(boardPRINT_IFACE == 1)
//RX
#define     	printUSART_GPIO_RX_PIN          		gpioUSART0_GPIO_RX_PIN
//TX
#define     	printUSART_GPIO_TX_PIN          		gpioUSART0_GPIO_TX_PIN
//串口
#if(!boardPRINT_IFACE_DMA_EN)
#define     	printUSART_IRQ_EN       				1
#endif  /* boardPRINT_IFACE_DMA_EN */
#define     	printUSART               				UART_NUM_0
#define     	printUSART_BAUD          				115200

#elif(boardPRINT_IFACE == 2)


#elif(boardPRINT_IFACE == 3)


#elif(boardPRINT_IFACE == 4)


#elif(boardPRINT_IFACE == 5)

#endif
/*-------------------------------------------------------------------------------------*/


#if(boardPRINT_485_IFACE_EN)
#define     	printGPIO_485_TX_EN_RCU       			RCU_GPIOA
#define     	printGPIO_485_TX_EN_PORT      			GPIOA
#define     	printGPIO_485_TX_EN_PIN       			GPIO_PIN_8
#define     	printGPIO_485_TX_EN_ON()      			GPIO_BOP(printGPIO_485_TX_EN_PORT) = (uint32_t)printGPIO_485_TX_EN_PIN   //使能发送
#define     	printGPIO_485_TX_EN_OFF()     			GPIO_BC(printGPIO_485_TX_EN_PORT) = (uint32_t)printGPIO_485_TX_EN_PIN   //使能接收
#endif //boardPRINT_485_IFACE_EN


//****************************************************Globals*******************************************************************//

//****************************************************Types*********************************************************************//

//****************************************************Extern********************************************************************//
void vPrint_IfaceInit(void);
void vPrint_IfaceDeInit(void);

bool bPrint_DataSendStart(uint16_t us_len);
bool bPrint_CheckSendFinish(void);

#if (boardPRINT_485_IFACE_EN)
void vPrint_485TransEnable(bool b_en);
#endif  /* boardPRINT_485_IFACE_EN */

#if (boardLOW_POWER)
void vPrint_EnterLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* boardPRINT_IFACE */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* PRINT_IFACE_H_ */

