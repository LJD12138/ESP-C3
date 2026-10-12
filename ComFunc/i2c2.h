/***********************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\ComFunc
 * File    : i2c2.h
 * Date    : 2026-10-11
 * Author  : LJD(291483914@qq.com)
 * Desc    : 软件模拟 I2C 通信接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef I2C2_H_
#define I2C2_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (1)
//****************************************************Macros********************************************************************//
#define			i2cCRC_BUFF_MAX_LEN						32

/* 兼容原有枚举命名 */
#define			AddrType_7bit							I2C_ADDR_7BIT
#define			AddrType_8bit							I2C_ADDR_8BIT

//****************************************************Types*********************************************************************//
typedef enum
{
	I2C_ADDR_7BIT = 0,	/* 7位从机地址 */
	I2C_ADDR_8BIT,		/* 8位从机地址 */
}I2cAddrType_E;

typedef I2cAddrType_E I2C_AddrType;				/* 兼容旧类型别名 */

typedef struct
{
	#if (boardIC_TYPE == boardIC_GD32F30X || boardIC_TYPE == boardIC_GD32F50X)
	vu8					Addr;				/* 从机地址 */
	I2C_AddrType		AddrType;			/* 地址类型 */
	vu32				ulGPIO_PORT_SCL;	/* SCL端口 */
	vu32				ulGPIO_PIN_SCL;		/* SCL引脚 */
	vu32				ulGPIO_PORT_SDA;	/* SDA端口 */
	vu32				ulGPIO_PIN_SDA;		/* SDA引脚 */
	vu16				usDelay;			/* 延时参数 */
	vu8					ucBuffLen;			/* 缓冲区长度 */
	vu8					ucLostCnt;			/* 丢包计数 */
	#elif (boardIC_TYPE == boardIC_STM32H7XX)
	vu8					Addr;				/* 从机地址 */
	I2C_AddrType		AddrType;			/* 地址类型 */
	GPIO_TypeDef*		ulGPIO_PORT_SCL;	/* SCL端口 */
	vu32				ulGPIO_PIN_SCL;		/* SCL引脚 */
	GPIO_TypeDef*		ulGPIO_PORT_SDA;	/* SDA端口 */
	vu32				ulGPIO_PIN_SDA;		/* SDA引脚 */
	vu16				usDelay;			/* 延时参数 */
	vu8					ucBuffLen;			/* 缓冲区长度 */
	vu8					ucLostCnt;			/* 丢包计数 */
	#elif (boardIC_TYPE == boardIC_ESP32C3 || boardIC_TYPE == boardIC_ESP32C6 || boardIC_TYPE == boardIC_ESP32C61)
	vu8					Addr;				/* 从机地址 */
	I2C_AddrType		AddrType;			/* 地址类型 */
	vu32				ulGPIO_PORT_SCL;	/* 兼容占位 (可填0) */
	vu32				ulGPIO_PIN_SCL;		/* SCL引脚编号 (gpio_num_t) */
	vu32				ulGPIO_PORT_SDA;	/* 兼容占位 (可填0) */
	vu32				ulGPIO_PIN_SDA;		/* SDA引脚编号 (gpio_num_t) */
	vu16				usDelay;			/* 延时参数 */
	vu8					ucBuffLen;			/* 缓冲区长度 */
	vu8					ucLostCnt;			/* 丢包计数 */
	#elif (boardIC_TYPE == boardIC_STM32G4XX)
	#error "boardIC_STM32G4XX 未定义"
	#endif  /* boardIC_TYPE */
}I2cObj_T;

//****************************************************Globals*******************************************************************//

//****************************************************Extern********************************************************************//
void vI2C_ObjInit(I2cObj_T *p_i2c_obj);
void vI2C_BusReset(const I2cObj_T *p_i2c_obj);
s8   cI2C_WriteData(const I2cObj_T *p_i2c_obj, const u8 *p_buf, u16 len);
s8   cI2C_ReadData(const I2cObj_T *p_i2c_obj, u8 *p_buf, u16 len);
s8   cI2C_WriteBytes(const I2cObj_T *p_i2c_obj, u8 reg_addr, const u8 *p_buf, u8 len);
s8   cI2C_ReadBytes(const I2cObj_T *p_i2c_obj, u8 reg_addr, u8 *p_buf, u8 len);
s8   cI2C_WriteBytes1(const I2cObj_T *p_i2c_obj, u16 reg_addr, const u8 *p_buf, u8 len);
s8   cI2C_ReadBytes1(const I2cObj_T *p_i2c_obj, u16 reg_addr, u8 *p_buf, u8 len);
s8   cI2C_WriteBytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, const u8 *p_buf, u8 len);
s8   cI2C_ReadBytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, u8 *p_buf, u8 len);
s8   cI2C_Read2BytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, u8 *p_buf);
s8   cI2C_Write1BytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, const u8 *p_buf);

#endif  /* 1 */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* I2C2_H_ */
