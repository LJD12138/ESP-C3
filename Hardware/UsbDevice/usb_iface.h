#ifndef USB_IFACE_H_
#define USB_IFACE_H_

#include "main.h"
#include "i2c2.h"
#include "driver/gpio.h"

// #define     	usbIC1_SCL_RCU          				RCU_GPIOC
// #define     	usbIC1_SCL_PORT     					GPIOC
#define     	usbIC1_SCL_PIN          				GPIO_NUM_3

// #define     	usbIC1_SDA_RCU          				RCU_GPIOC
// #define     	usbIC1_SDA_PORT     					GPIOC
#define     	usbIC1_SDA_PIN          				GPIO_NUM_2

// #define     	usbIC2_SCL_RCU          				RCU_GPIOC
// #define     	usbIC2_SCL_PORT     					GPIOC
#define     	usbIC2_SCL_PIN          				GPIO_NUM_4

// #define     	usbIC2_SDA_RCU          				RCU_GPIOC
// #define     	usbIC2_SDA_PORT     					GPIOC
#define     	usbIC2_SDA_PIN          				GPIO_NUM_6

// #define     	usbIC2_SCL_RCU          				RCU_GPIOC
// #define     	usbIC2_SCL_PORT     					GPIOC
#define     	usbIC3_SCL_PIN          				GPIO_NUM_4

// #define     	usbIC2_SDA_RCU          				RCU_GPIOC
// #define     	usbIC2_SDA_PORT     					GPIOC
#define     	usbIC3_SDA_PIN          				GPIO_NUM_5



//#define     	usbPD_EN_RCU          					RCU_GPIOA
//#define     	usbPD_EN_PORT     						GPIOA
//#define     	usbPD_EN_PIN          					GPIO_PIN_7
//#define     	usbPD_EN_ON()         					GPIO_BOP(usbPD_EN_PORT) = (uint32_t)usbPD_EN_PIN   //gpio_bit_set
//#define     	usbPD_EN_OFF()        					GPIO_BC(usbPD_EN_PORT) = (uint32_t)usbPD_EN_PIN


// #define     	usbPOWER_EN_RCU       					RCU_GPIOB
// #define     	usbPOWER_EN_PORT      					GPIOB
#define     	usbPOWER_EN_PIN       					GPIO_NUM_14
#define     	usbPOWER_EN_ON()      					gpio_set_level(usbPOWER_EN_PIN, 1)
#define     	usbPOWER_EN_OFF()     					gpio_set_level(usbPOWER_EN_PIN, 0)


extern I2cObj_T tUSB_IC1_I2C;
extern I2cObj_T tUSB_IC2_I2C;
extern I2cObj_T tUSB_IC3_I2C;

void vUsb_IfaceInit(void);

#endif

