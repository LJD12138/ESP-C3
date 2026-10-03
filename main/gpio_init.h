#ifndef GPIO_INIT_H_
#define GPIO_INIT_H_

#include "main.h"

#include "driver/uart.h"
#include "driver/gpio.h"

// #define     	gpioASSIST_OPEN_RCU               		RCU_GPIOC
// #define     	gpioASSIST_OPEN_PORT              		GPIOC
#define     	gpioASSIST_OPEN_PIN               		GPIO_NUM_0
#define     	gpioASSIST_OPEN_ON()              		gpio_set_level(gpioASSIST_OPEN_PIN, 1)
#define     	gpioASSIST_OPEN_OFF()             		gpio_set_level(gpioASSIST_OPEN_PIN, 0)

//************************************USART0***************************
#define     	gpioUSART0_REMAP_EN               		0
#if(!gpioUSART0_REMAP_EN)
// #define     	gpioUSART0_GPIO_RX_RCU                 	RCU_GPIOA
// #define     	gpioUSART0_GPIO_RX_PORT                	GPIOA
#define     	gpioUSART0_GPIO_RX_PIN                 	UART_PIN_NO_CHANGE
// #define     	gpioUSART0_GPIO_TX_RCU                 	RCU_GPIOA
// #define     	gpioUSART0_GPIO_TX_PORT                	GPIOA
#define     	gpioUSART0_GPIO_TX_PIN                 	UART_PIN_NO_CHANGE
#else
//´®¿Ú0ÖØÓ³Éä
#define     	gpioUSART0_GPIO_RX_RCU                 	RCU_GPIOB
#define     	gpioUSART0_GPIO_RX_PORT                	GPIOB
#define     	gpioUSART0_GPIO_RX_PIN                 	GPIO_PIN_7
#define     	gpioUSART0_GPIO_TX_RCU                 	RCU_GPIOB
#define     	gpioUSART0_GPIO_TX_PORT                	GPIOB
#define     	gpioUSART0_GPIO_TX_PIN                 	GPIO_PIN_6
#endif  //gpioUSART0_REMAP_EN
//DMA 
// #define     	gpioUSART0_DMA                    		DMA0
// #define     	gpioUSART0_DMA_RCU                		RCU_DMA0
// #define     	gpioUSART0_DMA_RX_CH              		DMA_CH4
// #define     	gpioUSART0_DMA_TX_CH              		DMA_CH3
// #define     	gpioUSART0_DMA_TX_IRQ             		DMA0_Channel3_IRQn
// #define     	gpioUSART0_DMA_TX_IRQ_HANDLER     		DMA0_Channel3_IRQHandler

//************************************USART1***************************
// #define     	gpioUSART1_GPIO_RX_RCU                 	RCU_GPIOA
// #define     	gpioUSART1_GPIO_RX_PORT                	GPIOA
#define     	gpioUSART1_GPIO_RX_PIN                 	GPIO_NUM_26
// #define     	gpioUSART1_GPIO_TX_RCU                 	RCU_GPIOA
// #define     	gpioUSART1_GPIO_TX_PORT                	GPIOA
#define     	gpioUSART1_GPIO_TX_PIN                 	GPIO_NUM_25

//DMA 
// #define     	gpioUSART1_DMA                    		DMA0
// #define     	gpioUSART1_DMA_RCU                		RCU_DMA0
// #define     	gpioUSART1_DMA_RX_CH              		DMA_CH5
// #define     	gpioUSART1_DMA_TX_CH              		DMA_CH6
// #define     	gpioUSART1_DMA_TX_IRQ             		DMA0_Channel6_IRQn
// #define     	gpioUSART1_DMA_TX_IRQ_HANDLER     		DMA0_Channel6_IRQHandler

//************************************USART2***************************
// #define     	gpioUSART2_GPIO_RX_RCU                 	RCU_GPIOB
// #define     	gpioUSART2_GPIO_RX_PORT                	GPIOB
#define     	gpioUSART2_GPIO_RX_PIN                 	GPIO_NUM_12
// #define     	gpioUSART2_GPIO_TX_RCU                 	RCU_GPIOB
// #define     	gpioUSART2_GPIO_TX_PORT                	GPIOB
#define     	gpioUSART2_GPIO_TX_PIN                 	GPIO_NUM_13

//DMA 
// #define     	gpioUSART2_DMA                    		DMA0
// #define     	gpioUSART2_DMA_RCU                		RCU_DMA0
// #define     	gpioUSART2_DMA_RX_CH              		DMA_CH2
// #define     	gpioUSART2_DMA_TX_CH              		DMA_CH1
// #define     	gpioUSART2_DMA_TX_IRQ             		DMA0_Channel1_IRQn
// #define     	gpioUSART2_DMA_TX_IRQ_HANDLER     		DMA0_Channel1_IRQHandler

//************************************UART3***************************
// #define     	gpioUART3_GPIO_RX_RCU                  	RCU_GPIOC
// #define     	gpioUART3_GPIO_RX_PORT                 	GPIOC
// #define     	gpioUART3_GPIO_RX_PIN                  	GPIO_PIN_11
// #define     	gpioUART3_GPIO_TX_RCU                  	RCU_GPIOC
// #define     	gpioUART3_GPIO_TX_PORT                 	GPIOC
// #define     	gpioUART3_GPIO_TX_PIN                  	GPIO_PIN_10

//DMA 
// #define     	gpioUART3_DMA                     		DMA1
// #define     	gpioUART3_DMA_RCU                 		RCU_DMA1
// #define     	gpioUART3_DMA_RX_CH               		DMA_CH2
// #define     	gpioUART3_DMA_TX_CH               		DMA_CH4
// #define     	gpioUART3_DMA_TX_IRQ              		DMA1_Channel3_Channel4_IRQn
// #define     	gpioUART3_DMA_TX_IRQ_HANDLER      		DMA1_Channel3_4_IRQHandler
//************************************UART4******************************************
// #define			gpioUART4_GPIO_RX_RCU					__HAL_RCC_GPIOB_CLK_ENABLE();
// #define			gpioUART4_GPIO_RX_GPIO					GPIOB
// #define			gpioUART4_GPIO_RX_PIN					GPIO_PIN_8
// #define			gpioUART4_GPIO_RX_AF					GPIO_AF8_UART4

// #define			gpioUART4_GPIO_TX_RCU					__HAL_RCC_GPIOB_CLK_ENABLE();
// #define			gpioUART4_GPIO_TX_GPIO					GPIOB
// #define			gpioUART4_GPIO_TX_PIN					GPIO_PIN_9
// #define			gpioUART4_GPIO_TX_AF					GPIO_AF8_UART4

// //DMA 
// #define			gpioUART4_DMA							DMA1
// #define			gpioUART4_DMA_RCU						__HAL_RCC_DMA1_CLK_ENABLE();
// #define			gpioUART4_DMA_RX_REQ					DMA_REQUEST_UART4_RX
// #define			gpioUART4_DMA_TX_REQ					DMA_REQUEST_UART4_TX
// #define			gpioUART4_DMA_RX_CH						DMA1_Stream2
// #define			gpioUART4_DMA_TX_CH						DMA1_Stream3
// #define			gpioUART4_DMA_TX_IRQ					DMA1_Stream3_IRQn
// #define			gpioUART4_DMA_TX_IRQ_HANDLER			DMA1_Stream3_IRQHandler

// //************************************UART5******************************************
// #define			gpioUART5_GPIO_RX_RCU					__HAL_RCC_GPIOD_CLK_ENABLE();
// #define			gpioUART5_GPIO_RX_GPIO					GPIOD
// #define			gpioUART5_GPIO_RX_PIN					GPIO_PIN_2
// #define			gpioUART5_GPIO_RX_AF					GPIO_AF8_UART5

// #define			gpioUART5_GPIO_TX_RCU					__HAL_RCC_GPIOC_CLK_ENABLE();
// #define			gpioUART5_GPIO_TX_GPIO					GPIOC
// #define			gpioUART5_GPIO_TX_PIN					GPIO_PIN_12
// #define			gpioUART5_GPIO_TX_AF					GPIO_AF8_UART5

// //DMA 
// #define			gpioUART5_DMA							DMA1
// #define			gpioUART5_DMA_RCU						__HAL_RCC_DMA1_CLK_ENABLE();
// #define			gpioUART5_DMA_RX_REQ					DMA_REQUEST_UART5_RX
// #define			gpioUART5_DMA_TX_REQ					DMA_REQUEST_UART5_TX
// #define			gpioUART5_DMA_RX_CH						DMA1_Stream1
// #define			gpioUART5_DMA_TX_CH						DMA1_Stream0
// #define			gpioUART5_DMA_TX_IRQ					DMA1_Stream0_IRQn
// #define			gpioUART5_DMA_TX_IRQ_HANDLER			DMA1_Stream0_IRQHandler

// //************************************USART6******************************************
// #define			gpioUSART6_GPIO_RX_RCU					__HAL_RCC_GPIOC_CLK_ENABLE();
// #define			gpioUSART6_GPIO_RX_GPIO					GPIOC
// #define			gpioUSART6_GPIO_RX_PIN					GPIO_PIN_7
// #define			gpioUSART6_GPIO_RX_AF					GPIO_AF7_USART6

// #define			gpioUSART6_GPIO_TX_RCU					__HAL_RCC_GPIOC_CLK_ENABLE();
// #define			gpioUSART6_GPIO_TX_GPIO					GPIOC
// #define			gpioUSART6_GPIO_TX_PIN					GPIO_PIN_6
// #define			gpioUSART6_GPIO_TX_AF					GPIO_AF7_USART6

// //DMA 
// #define			gpioUSART6_DMA							DMA1
// #define			gpioUSART6_DMA_RCU						__HAL_RCC_DMA1_CLK_ENABLE();
// #define			gpioUSART6_DMA_RX_REQ					DMA_REQUEST_USART6_RX
// #define			gpioUSART6_DMA_TX_REQ					DMA_REQUEST_USART6_TX
// #define			gpioUSART6_DMA_RX_CH					DMA1_Stream4
// #define			gpioUSART6_DMA_TX_CH					DMA1_Stream5
// #define			gpioUSART6_DMA_TX_IRQ					DMA1_Stream5_IRQn
// #define			gpioUSART6_DMA_TX_IRQ_HANDLER			DMA1_Stream5_IRQHandler


void vGPIO_Init(void);
bool vGPIO_AssistBmsOpen(bool en);

#if(boardLOW_POWER)
void vGPIO_EnterLowPower(void);
void vGPIO_ExitLowPower(void);
#endif  //boardLOW_POWER
#endif
