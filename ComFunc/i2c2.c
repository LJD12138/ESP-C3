/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\ComFunc
 * File    : i2c2.c
 * Date    : 2026-10-11
 * Author  : LJD(291483914@qq.com)
 * Desc    : 软件模拟 I2C 通信接口实现，提供 GPIO 模拟时序、无阻塞延时与 CRC 校验封装
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "i2c2.h"
#include "check.h"
#include <string.h>

#if (boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif  /* boardUSE_OS */

#if (1)
//****************************************************Macros********************************************************************//
#define			ACK										1
#define			NACK									0

#if (boardIC_TYPE == boardIC_GD32F30X || boardIC_TYPE == boardIC_GD32F50X)
#define			I2C_LOW(GPIO, PIN)						GPIO_BC(GPIO) = (uint32_t)(PIN)
#define			I2C_HIGH(GPIO, PIN)						GPIO_BOP(GPIO) = (uint32_t)(PIN)
#elif (boardIC_TYPE == boardIC_STM32H7XX)
#define			I2C_LOW(GPIO, PIN)						(HAL_GPIO_WritePin(GPIO, PIN, GPIO_PIN_RESET))
#define			I2C_HIGH(GPIO, PIN)						(HAL_GPIO_WritePin(GPIO, PIN, GPIO_PIN_SET))
#elif (boardIC_TYPE == boardIC_ESP32C3 || boardIC_TYPE == boardIC_ESP32C6 || boardIC_TYPE == boardIC_ESP32C61)
#include "driver/gpio.h"
#define			I2C_LOW(GPIO, PIN)						gpio_set_level((gpio_num_t)(PIN), 0)
#define			I2C_HIGH(GPIO, PIN)						gpio_set_level((gpio_num_t)(PIN), 1)
#elif (boardIC_TYPE == boardIC_STM32G4XX)
#error "boardIC_STM32G4XX 未定义"
#endif  /* boardIC_TYPE */

//****************************************************Parameter Initialization**************************************************//

//****************************************************Function Declaration******************************************************//
static void v_delay(uint16_t us_cnt);
static void v_delay_low(uint16_t us_cnt);
static void v_i2c_set_scl_gpio_output(const I2cObj_T *p_i2c_obj);
static void v_i2c_set_sda_gpio_output(const I2cObj_T *p_i2c_obj);
static void v_i2c_set_sda_gpio_input(const I2cObj_T *p_i2c_obj);
static bool b_i2c_read_sda_gpio(const I2cObj_T *p_i2c_obj);
static void v_i2c_set_start(const I2cObj_T *p_i2c_obj);
static void v_i2c_set_stop(const I2cObj_T *p_i2c_obj);
static void v_i2c_set_ack(const I2cObj_T *p_i2c_obj);
static void v_i2c_set_no_ack(const I2cObj_T *p_i2c_obj);
static s8   c_i2c_wait_ack(const I2cObj_T *p_i2c_obj);
static void v_i2c_write_byte(const I2cObj_T *p_i2c_obj, u8 txd);
static u8   uc_i2c_read_byte(const I2cObj_T *p_i2c_obj, u8 ack);

/***********************************************************************************************************************
 * 函数功能    : 软件延时，降低 I2C 速率
 * 说明(备注)  : 内部时序控制 (加入 nop 防止优化消除)
 * 传入参数    : us_cnt: 延时计数值
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_delay(uint16_t us_cnt)
{
	while (us_cnt--)
	{
		__asm volatile ("nop");
	}
}

/***********************************************************************************************************************
 * 函数功能    : 软件半延时
 * 说明(备注)  : 内部时序控制 (加入 nop 防止优化消除)
 * 传入参数    : us_cnt: 延时计数值
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_delay_low(uint16_t us_cnt)
{
	us_cnt >>= 1;
	while (us_cnt--)
	{
		__asm volatile ("nop");
	}
}

/***********************************************************************************************************************
 * 函数功能    : 设置 SCL 为推挽输出
 * 说明(备注)  : 时钟线配置
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_i2c_set_scl_gpio_output(const I2cObj_T *p_i2c_obj)
{
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(p_i2c_obj->ulGPIO_PORT_SCL, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, p_i2c_obj->ulGPIO_PIN_SCL);
	gpio_output_options_set(p_i2c_obj->ulGPIO_PORT_SCL, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, p_i2c_obj->ulGPIO_PIN_SCL);
	#elif (boardIC_TYPE == boardIC_GD32F30X)
	gpio_init(p_i2c_obj->ulGPIO_PORT_SCL, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, p_i2c_obj->ulGPIO_PIN_SCL);
	#elif (boardIC_TYPE == boardIC_STM32H7XX)
	GPIO_InitTypeDef gpio_init_struct = {0};
	gpio_init_struct.Pin   = p_i2c_obj->ulGPIO_PIN_SCL;
	gpio_init_struct.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_struct.Pull  = GPIO_NOPULL;
	gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(p_i2c_obj->ulGPIO_PORT_SCL, &gpio_init_struct);
	#elif (boardIC_TYPE == boardIC_ESP32C3 || boardIC_TYPE == boardIC_ESP32C6 || boardIC_TYPE == boardIC_ESP32C61)
	gpio_config_t io_conf = {
		.pin_bit_mask = (1ULL << p_i2c_obj->ulGPIO_PIN_SCL),
		.mode         = GPIO_MODE_OUTPUT,
		.pull_up_en   = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type    = GPIO_INTR_DISABLE,
	};
	gpio_config(&io_conf);
	#elif (boardIC_TYPE == boardIC_STM32G4XX)
	#error "boardIC_STM32G4XX 未定义"
	#endif  /* boardIC_TYPE */
}

/***********************************************************************************************************************
 * 函数功能    : 设置 SDA 为推挽输出
 * 说明(备注)  : 数据发送时配置
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_i2c_set_sda_gpio_output(const I2cObj_T *p_i2c_obj)
{
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(p_i2c_obj->ulGPIO_PORT_SDA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, p_i2c_obj->ulGPIO_PIN_SDA);
	gpio_output_options_set(p_i2c_obj->ulGPIO_PORT_SDA, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, p_i2c_obj->ulGPIO_PIN_SDA);
	#elif (boardIC_TYPE == boardIC_GD32F30X)
	gpio_init(p_i2c_obj->ulGPIO_PORT_SDA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, p_i2c_obj->ulGPIO_PIN_SDA);
	#elif (boardIC_TYPE == boardIC_STM32H7XX)
	GPIO_InitTypeDef gpio_init_struct = {0};
	gpio_init_struct.Pin   = p_i2c_obj->ulGPIO_PIN_SDA;
	gpio_init_struct.Mode  = GPIO_MODE_OUTPUT_PP;
	gpio_init_struct.Pull  = GPIO_NOPULL;
	gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(p_i2c_obj->ulGPIO_PORT_SDA, &gpio_init_struct);
	#elif (boardIC_TYPE == boardIC_ESP32C3 || boardIC_TYPE == boardIC_ESP32C6 || boardIC_TYPE == boardIC_ESP32C61)
	gpio_config_t io_conf = {
		.pin_bit_mask = (1ULL << p_i2c_obj->ulGPIO_PIN_SDA),
		.mode         = GPIO_MODE_OUTPUT,
		.pull_up_en   = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type    = GPIO_INTR_DISABLE,
	};
	gpio_config(&io_conf);
	#elif (boardIC_TYPE == boardIC_STM32G4XX)
	#error "boardIC_STM32G4XX 未定义"
	#endif  /* boardIC_TYPE */
}

/***********************************************************************************************************************
 * 函数功能    : 设置 SDA 为上拉输入
 * 说明(备注)  : 数据接收及 ACK 采样时配置
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_i2c_set_sda_gpio_input(const I2cObj_T *p_i2c_obj)
{
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(p_i2c_obj->ulGPIO_PORT_SDA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, p_i2c_obj->ulGPIO_PIN_SDA);
	#elif (boardIC_TYPE == boardIC_GD32F30X)
	gpio_init(p_i2c_obj->ulGPIO_PORT_SDA, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, p_i2c_obj->ulGPIO_PIN_SDA);
	#elif (boardIC_TYPE == boardIC_STM32H7XX)
	GPIO_InitTypeDef gpio_init_struct = {0};
	gpio_init_struct.Pin  = p_i2c_obj->ulGPIO_PIN_SDA;
	gpio_init_struct.Mode = GPIO_MODE_INPUT;
	gpio_init_struct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(p_i2c_obj->ulGPIO_PORT_SDA, &gpio_init_struct);
	#elif (boardIC_TYPE == boardIC_ESP32C3 || boardIC_TYPE == boardIC_ESP32C6 || boardIC_TYPE == boardIC_ESP32C61)
	gpio_config_t io_conf = {
		.pin_bit_mask = (1ULL << p_i2c_obj->ulGPIO_PIN_SDA),
		.mode         = GPIO_MODE_INPUT,
		.pull_up_en   = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type    = GPIO_INTR_DISABLE,
	};
	gpio_config(&io_conf);
	#elif (boardIC_TYPE == boardIC_STM32G4XX)
	#error "boardIC_STM32G4XX 未定义"
	#endif  /* boardIC_TYPE */
}

/***********************************************************************************************************************
 * 函数功能    : 读取 SDA 电平信号
 * 说明(备注)  : 采样 SDA 引脚输入电平
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : true: 高电平; false: 低电平
 ************************************************************************************************************************/
static bool b_i2c_read_sda_gpio(const I2cObj_T *p_i2c_obj)
{
	#if (boardIC_TYPE == boardIC_GD32F30X || boardIC_TYPE == boardIC_GD32F50X)
	return (((uint32_t)RESET != (GPIO_ISTAT(p_i2c_obj->ulGPIO_PORT_SDA) & (p_i2c_obj->ulGPIO_PIN_SDA))) ? true : false);
	#elif (boardIC_TYPE == boardIC_STM32H7XX)
	return ((HAL_GPIO_ReadPin(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA) != GPIO_PIN_RESET) ? true : false);
	#elif (boardIC_TYPE == boardIC_ESP32C3 || boardIC_TYPE == boardIC_ESP32C6 || boardIC_TYPE == boardIC_ESP32C61)
	return (gpio_get_level((gpio_num_t)p_i2c_obj->ulGPIO_PIN_SDA) != 0);
	#else
	return false;
	#endif  /* boardIC_TYPE */
}

/***********************************************************************************************************************
 * 函数功能    : 发送起始信号 (START)
 * 说明(备注)  : 当 SCL 为高电平时，SDA 产生由高到低的跳变
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_i2c_set_start(const I2cObj_T *p_i2c_obj)
{
	v_i2c_set_sda_gpio_output(p_i2c_obj);

	I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
	I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
	v_delay(p_i2c_obj->usDelay);
	I2C_LOW(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
	v_delay(p_i2c_obj->usDelay);
	I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
}

/***********************************************************************************************************************
 * 函数功能    : 发送停止信号 (STOP)
 * 说明(备注)  : 当 SCL 为高电平时，SDA 产生由低到高的跳变
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_i2c_set_stop(const I2cObj_T *p_i2c_obj)
{
	v_i2c_set_sda_gpio_output(p_i2c_obj);

	I2C_LOW(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
	I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
	v_delay(p_i2c_obj->usDelay);
	I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
	v_delay(p_i2c_obj->usDelay);
	I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
}

/***********************************************************************************************************************
 * 函数功能    : 主机产生 ACK 应答信号
 * 说明(备注)  : 主机接收数据后拉低 SDA 通知从机继续发送
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_i2c_set_ack(const I2cObj_T *p_i2c_obj)
{
	I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
	v_i2c_set_sda_gpio_output(p_i2c_obj);
	I2C_LOW(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
	v_delay_low(p_i2c_obj->usDelay);
	I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
	v_delay(p_i2c_obj->usDelay);
	I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
}

/***********************************************************************************************************************
 * 函数功能    : 主机产生 NACK 非应答信号
 * 说明(备注)  : 主机接收最后一个字节后保持 SDA 为高，通知从机停止发送
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_i2c_set_no_ack(const I2cObj_T *p_i2c_obj)
{
	I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
	v_i2c_set_sda_gpio_output(p_i2c_obj);
	I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
	v_delay_low(p_i2c_obj->usDelay);
	I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
	v_delay(p_i2c_obj->usDelay);
	I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
}

/***********************************************************************************************************************
 * 函数功能    : 等待从机 ACK 应答
 * 说明(备注)  : 主机释放 SDA 为输入，并检测从机是否拉低 SDA
 * 传入参数    : p_i2c_obj: I2C 对象
 * 输出参数    : 无
 * 返回值      : 1: 成功接收到 ACK; -1: 超时未接收到 ACK
 ************************************************************************************************************************/
static s8 c_i2c_wait_ack(const I2cObj_T *p_i2c_obj)
{
	uint8_t uc_err_time = 0;

	v_i2c_set_sda_gpio_input(p_i2c_obj);

	I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
	v_delay_low(p_i2c_obj->usDelay);
	I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
	v_delay_low(p_i2c_obj->usDelay);

	while (b_i2c_read_sda_gpio(p_i2c_obj))
	{
		uc_err_time++;
		if (uc_err_time > 250)
		{
			v_i2c_set_stop(p_i2c_obj);
			return -1;
		}
	}

	I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 发送一个字节数据
 * 说明(备注)  : 高位在前 (MSB)，在 SCL 低电平时切换 SDA 数据
 * 传入参数    : p_i2c_obj: I2C 对象; txd: 待发送字节
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_i2c_write_byte(const I2cObj_T *p_i2c_obj, u8 txd)
{
	uint8_t uc_t;

	v_i2c_set_sda_gpio_output(p_i2c_obj);
	I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);

	for (uc_t = 0; uc_t < 8; uc_t++)
	{
		if ((txd & 0x80) >> 7)
			I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
		else
			I2C_LOW(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
		txd <<= 1;
		v_delay_low(p_i2c_obj->usDelay);
		I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
		v_delay(p_i2c_obj->usDelay);
		I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
		v_delay_low(p_i2c_obj->usDelay);
	}
}

/***********************************************************************************************************************
 * 函数功能    : 读取一个字节数据
 * 说明(备注)  : 采样 SDA 接收 8 位数据，并在结束后发送 ACK 或 NACK
 * 传入参数    : p_i2c_obj: I2C 对象; ack: 1 发送 ACK, 0 发送 NACK
 * 输出参数    : 无
 * 返回值      : 接收到的字节数据
 ************************************************************************************************************************/
static u8 uc_i2c_read_byte(const I2cObj_T *p_i2c_obj, u8 ack)
{
	uint8_t uc_i;
	uint8_t uc_receive = 0;

	v_i2c_set_sda_gpio_input(p_i2c_obj);

	for (uc_i = 0; uc_i < 8; uc_i++)
	{
		I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
		v_delay_low(p_i2c_obj->usDelay);
		I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
		uc_receive <<= 1;
		if (b_i2c_read_sda_gpio(p_i2c_obj))
			uc_receive++;
		v_delay_low(p_i2c_obj->usDelay);
	}

	if (!ack)
		v_i2c_set_no_ack(p_i2c_obj);
	else
		v_i2c_set_ack(p_i2c_obj);

	return uc_receive;
}

/***********************************************************************************************************************
 * 函数功能    : I2C 对象初始化
 * 说明(备注)  : 配置从机地址格式、引脚为推挽输出并拉高总线
 * 传入参数    : p_i2c_obj: I2C 对象指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vI2C_ObjInit(I2cObj_T *p_i2c_obj)
{
	if (p_i2c_obj == NULL)
		return;

	if (p_i2c_obj->AddrType == AddrType_7bit)
	{
		p_i2c_obj->Addr = (p_i2c_obj->Addr << 1) & 0xFE;
		p_i2c_obj->AddrType = AddrType_8bit;
	}

	v_i2c_set_scl_gpio_output(p_i2c_obj);
	v_i2c_set_sda_gpio_output(p_i2c_obj);

	I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
	I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
}

/***********************************************************************************************************************
 * 函数功能    : I2C 总线死锁恢复 (9 脉冲复位)
 * 说明(备注)  : 当从机异常拉低 SDA 时，通过 9 个时钟脉冲恢复总线
 * 传入参数    : p_i2c_obj: I2C 对象指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vI2C_BusReset(const I2cObj_T *p_i2c_obj)
{
	uint8_t uc_i;

	if (p_i2c_obj == NULL)
		return;

	v_i2c_set_sda_gpio_input(p_i2c_obj);
	v_i2c_set_scl_gpio_output(p_i2c_obj);

	for (uc_i = 0; uc_i < 9; uc_i++)
	{
		if (b_i2c_read_sda_gpio(p_i2c_obj))
			break;
		I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
		v_delay_low(p_i2c_obj->usDelay);
		I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
		v_delay_low(p_i2c_obj->usDelay);
	}

	v_i2c_set_stop(p_i2c_obj);
}

/***********************************************************************************************************************
 * 函数功能    : 连续写数据 (无寄存器地址)
 * 说明(备注)  : 适合无内部寄存器地址的器件
 * 传入参数    : p_i2c_obj: I2C 对象; p_buf: 发送缓冲区; len: 数据长度
 * 输出参数    : 无
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_WriteData(const I2cObj_T *p_i2c_obj, const u8 *p_buf, u16 len)
{
	uint16_t us_idx;

	if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
		return -128;

	v_i2c_set_start(p_i2c_obj);
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));

	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -1;

	for (us_idx = 0; us_idx < len; us_idx++)
	{
		v_i2c_write_byte(p_i2c_obj, p_buf[us_idx]);
		if (c_i2c_wait_ack(p_i2c_obj) < 0)
			return -2;
	}

	v_i2c_set_stop(p_i2c_obj);
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 连续读数据 (无寄存器地址)
 * 说明(备注)  : 适合无内部寄存器地址的器件
 * 传入参数    : p_i2c_obj: I2C 对象; p_buf: 接收缓冲区; len: 数据长度
 * 输出参数    : p_buf: 接收到的数据
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_ReadData(const I2cObj_T *p_i2c_obj, u8 *p_buf, u16 len)
{
	if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
		return -128;

	v_i2c_set_start(p_i2c_obj);
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 1));

	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -1;

	while (len)
	{
		if (len == 1)
			*p_buf = uc_i2c_read_byte(p_i2c_obj, NACK);
		else
			*p_buf = uc_i2c_read_byte(p_i2c_obj, ACK);
		p_buf++;
		len--;
	}

	v_i2c_set_stop(p_i2c_obj);
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 写单字节寄存器数据
 * 说明(备注)  : 8 位寄存器地址
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 8 位寄存器地址; p_buf: 发送数据; len: 长度
 * 输出参数    : 无
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_WriteBytes(const I2cObj_T *p_i2c_obj, u8 reg_addr, const u8 *p_buf, u8 len)
{
	uint8_t uc_idx;

	if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
		return -128;

	v_i2c_set_start(p_i2c_obj);
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));

	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -1;

	v_i2c_write_byte(p_i2c_obj, reg_addr);
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -2;

	for (uc_idx = 0; uc_idx < len; uc_idx++)
	{
		v_i2c_write_byte(p_i2c_obj, p_buf[uc_idx]);
		if (c_i2c_wait_ack(p_i2c_obj) < 0)
			return -3;
	}

	v_i2c_set_stop(p_i2c_obj);
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 读单字节寄存器数据
 * 说明(备注)  : 8 位寄存器地址
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 8 位寄存器地址; p_buf: 接收数据; len: 长度
 * 输出参数    : p_buf: 接收到的数据
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_ReadBytes(const I2cObj_T *p_i2c_obj, u8 reg_addr, u8 *p_buf, u8 len)
{
	if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
		return -128;

	v_i2c_set_start(p_i2c_obj);
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -1;

	v_i2c_write_byte(p_i2c_obj, reg_addr);
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -2;

	v_i2c_set_start(p_i2c_obj);
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 1));
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -3;

	while (len)
	{
		if (len == 1)
			*p_buf = uc_i2c_read_byte(p_i2c_obj, NACK);
		else
			*p_buf = uc_i2c_read_byte(p_i2c_obj, ACK);
		len--;
		p_buf++;
	}

	v_i2c_set_stop(p_i2c_obj);
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 写入双字节寄存器数据
 * 说明(备注)  : 16 位寄存器地址 (高位在前)
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 16 位寄存器地址; p_buf: 发送数据; len: 长度
 * 输出参数    : 无
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_WriteBytes1(const I2cObj_T *p_i2c_obj, u16 reg_addr, const u8 *p_buf, u8 len)
{
	uint8_t uc_idx;

	if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
		return -128;

	v_i2c_set_start(p_i2c_obj);
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -1;

	/* 写寄存器高地址 */
	v_i2c_write_byte(p_i2c_obj, (uint8_t)(reg_addr >> 8));
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -2;

	/* 写寄存器低地址 */
	v_i2c_write_byte(p_i2c_obj, (uint8_t)(reg_addr & 0xFF));
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -2;

	for (uc_idx = 0; uc_idx < len; uc_idx++)
	{
		v_i2c_write_byte(p_i2c_obj, p_buf[uc_idx]);
		if (c_i2c_wait_ack(p_i2c_obj) < 0)
			return -3;
	}

	v_i2c_set_stop(p_i2c_obj);
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 读取双字节寄存器数据
 * 说明(备注)  : 16 位寄存器地址 (高位在前)
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 16 位寄存器地址; p_buf: 接收数据; len: 长度
 * 输出参数    : p_buf: 接收到的数据
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_ReadBytes1(const I2cObj_T *p_i2c_obj, u16 reg_addr, u8 *p_buf, u8 len)
{
	if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
		return -128;

	v_i2c_set_start(p_i2c_obj);
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -1;

	v_i2c_write_byte(p_i2c_obj, (uint8_t)(reg_addr >> 8));
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -2;

	v_i2c_write_byte(p_i2c_obj, (uint8_t)(reg_addr & 0xFF));
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -2;

	v_i2c_set_start(p_i2c_obj);
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 1));
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -3;

	while (len)
	{
		if (len == 1)
			*p_buf = uc_i2c_read_byte(p_i2c_obj, NACK);
		else
			*p_buf = uc_i2c_read_byte(p_i2c_obj, ACK);
		len--;
		p_buf++;
	}

	v_i2c_set_stop(p_i2c_obj);
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 写数据 (带寄存器地址和 CRC8 校验)
 * 说明(备注)  : AFE 专用，定长栈缓冲区防护，避免 VLA 动态栈开销
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 寄存器地址; p_buf: 发送数据; len: 长度
 * 输出参数    : 无
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_WriteBytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, const u8 *p_buf, u8 len)
{
	uint8_t uca_buff[i2cCRC_BUFF_MAX_LEN];
	uint8_t uc_crc_data = 0;
	uint8_t uc_idx;

	if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
		return -128;

	if (p_i2c_obj->ucBuffLen <= 2 || p_i2c_obj->ucBuffLen > i2cCRC_BUFF_MAX_LEN)
		return -1;

	if (p_i2c_obj->ucBuffLen < (len + 2))
		return -2;

	uca_buff[0] = p_i2c_obj->Addr;
	uca_buff[1] = reg_addr;
	memcpy(&uca_buff[2], p_buf, len);

	uc_crc_data = ucCheck_CRC8cal(uca_buff, len + 2);

	v_i2c_set_start(p_i2c_obj);
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -3;

	v_i2c_write_byte(p_i2c_obj, reg_addr);
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -4;

	for (uc_idx = 0; uc_idx < len; uc_idx++)
	{
		v_i2c_write_byte(p_i2c_obj, p_buf[uc_idx]);
		if (c_i2c_wait_ack(p_i2c_obj) < 0)
			return -5;
	}

	v_i2c_write_byte(p_i2c_obj, uc_crc_data);
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -6;

	v_i2c_set_stop(p_i2c_obj);
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 读数据 (带寄存器地址和 CRC8 校验)
 * 说明(备注)  : AFE 专用，定长栈缓冲区防护
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 寄存器地址; p_buf: 接收数据; len: 长度
 * 输出参数    : p_buf: 接收到的校验后有效数据
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_ReadBytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, u8 *p_buf, u8 len)
{
	uint8_t uca_buff[i2cCRC_BUFF_MAX_LEN];
	uint8_t uc_crc_data = 0;
	uint8_t uc_idx;

	if ((p_i2c_obj == NULL) || (p_buf == NULL && len > 0))
		return -128;

	if (p_i2c_obj->ucBuffLen <= 4 || p_i2c_obj->ucBuffLen > i2cCRC_BUFF_MAX_LEN)
		return -1;

	if (p_i2c_obj->ucBuffLen < (len + 4))
		return -2;

	uca_buff[0] = p_i2c_obj->Addr;
	uca_buff[1] = reg_addr;
	uca_buff[2] = len;
	uca_buff[3] = p_i2c_obj->Addr | 0x01;

	v_i2c_set_start(p_i2c_obj);
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -3;

	v_i2c_write_byte(p_i2c_obj, reg_addr);
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -4;

	v_i2c_write_byte(p_i2c_obj, len);
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -5;

	v_i2c_set_start(p_i2c_obj);
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 1));
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -6;

	for (uc_idx = 0; uc_idx < len; uc_idx++)
	{
		uca_buff[4 + uc_idx] = uc_i2c_read_byte(p_i2c_obj, ACK);
	}

	uc_crc_data = uc_i2c_read_byte(p_i2c_obj, NACK);
	v_i2c_set_stop(p_i2c_obj);

	if (uc_crc_data == ucCheck_CRC8cal(uca_buff, 4 + len))
	{
		for (uc_idx = 0; uc_idx < len; uc_idx++)
		{
			p_buf[uc_idx] = uca_buff[4 + uc_idx];
		}
	}
	else
		return -7;

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 读取两字节数据 (带 CRC8 校验)
 * 说明(备注)  : AFE 专用
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 寄存器地址; p_buf: 接收数据 (2 字节)
 * 输出参数    : p_buf: 接收到的数据
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_Read2BytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, u8 *p_buf)
{
	uint8_t uca_buff[i2cCRC_BUFF_MAX_LEN];
	uint8_t uc_crc_data = 0;
	uint8_t uc_idx;

	if ((p_i2c_obj == NULL) || (p_buf == NULL))
		return -128;

	if (p_i2c_obj->ucBuffLen <= 4 || p_i2c_obj->ucBuffLen > i2cCRC_BUFF_MAX_LEN)
		return -1;

	if (p_i2c_obj->ucBuffLen < (2 + 4))
		return -2;

	uca_buff[0] = p_i2c_obj->Addr;
	uca_buff[1] = reg_addr;
	uca_buff[2] = p_i2c_obj->Addr | 0x01;

	v_i2c_set_start(p_i2c_obj);
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -3;

	v_i2c_write_byte(p_i2c_obj, reg_addr);
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -4;

	v_i2c_set_start(p_i2c_obj);
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 1));
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -6;

	for (uc_idx = 0; uc_idx < 2; uc_idx++)
	{
		uca_buff[3 + uc_idx] = uc_i2c_read_byte(p_i2c_obj, ACK);
	}

	uc_crc_data = uc_i2c_read_byte(p_i2c_obj, NACK);
	v_i2c_set_stop(p_i2c_obj);

	if (uc_crc_data == ucCheck_CRC8cal(uca_buff, 5))
	{
		for (uc_idx = 0; uc_idx < 2; uc_idx++)
		{
			p_buf[uc_idx] = uca_buff[3 + uc_idx];
		}
	}
	else
		return -7;

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 写入单字节数据 (带 CRC8 校验)
 * 说明(备注)  : AFE 专用
 * 传入参数    : p_i2c_obj: I2C 对象; reg_addr: 寄存器地址; p_buf: 发送数据 (1 字节)
 * 输出参数    : 无
 * 返回值      : 1: 成功; <0: 失败错误码
 ************************************************************************************************************************/
s8 cI2C_Write1BytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, const u8 *p_buf)
{
	uint8_t uca_buff[i2cCRC_BUFF_MAX_LEN];
	uint8_t uc_crc_data = 0;
	const uint8_t uc_len = 1;

	if ((p_i2c_obj == NULL) || (p_buf == NULL))
		return -128;

	if (p_i2c_obj->ucBuffLen <= 2 || p_i2c_obj->ucBuffLen > i2cCRC_BUFF_MAX_LEN)
		return -1;

	if (p_i2c_obj->ucBuffLen < (uc_len + 2))
		return -2;

	uca_buff[0] = p_i2c_obj->Addr;
	uca_buff[1] = reg_addr;
	memcpy(&uca_buff[2], p_buf, uc_len);

	uc_crc_data = ucCheck_CRC8cal(uca_buff, uc_len + 2);

	v_i2c_set_start(p_i2c_obj);
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -3;

	v_i2c_write_byte(p_i2c_obj, reg_addr);
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -4;

	v_i2c_write_byte(p_i2c_obj, p_buf[0]);
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -5;

	v_i2c_write_byte(p_i2c_obj, uc_crc_data);
	if (c_i2c_wait_ack(p_i2c_obj) < 0)
		return -6;

	v_i2c_set_stop(p_i2c_obj);
	return 1;
}

#endif  /* 1 */
