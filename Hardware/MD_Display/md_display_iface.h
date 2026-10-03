/*******************************************************************************************************************************
 * Project : G02
 * Module  : G:\TempFile\G02\Hardware\MD_Display
 * File    : md_display_iface.h
 * Date    : 2026-01-23 17:23:34
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示屏接口层头文件 - ST7789 + FT6336 版本
 * -------------------------------------------------------
 * History :
 * 2026-02-07 添加ESP-LCD面板句柄和背光控制API
 * 2026-02-24 优化：移除未使用的函数声明，统一宏命名风格
 * 2026-03-04 修改：适配ST7789 LCD驱动和FT6336触摸屏驱动
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
*******************************************************************************************************************************/
#ifndef MD_DISPLAY_IFACE_H
#define MD_DISPLAY_IFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================includes====================================*/
#include "board_config.h"

/* ==========================================GPIO定义====================================*/
/* SPI引脚 - J4接口 */
#define     dispSPI_GPIO_MOSI_PIN       GPIO_NUM_5      /* IO5_LCD_MOSI */
#define     dispSPI_GPIO_CLK_PIN        GPIO_NUM_3      /* IO3_LCD_SCK */
#define     dispSPI_GPIO_CS_PIN         GPIO_NUM_4      /* IO4_LCD_CS */

/* LCD控制引脚 */
#define     dispLCD_GPIO_RES_PIN        GPIO_NUM_NC     /* 未使用硬件复位，使用软件复位 */
#define     dispLCD_GPIO_DC_PIN         GPIO_NUM_6      /* IO6_LCD_DC */
#define     dispLCD_GPIO_BLK_PIN        GPIO_NUM_2      /* IO2_LCD_BL */

/* I2C引脚 - 触摸屏 FT6336 */
#define     dispTOUCH_I2C_SDA_PIN       GPIO_NUM_0      /* IO0_I2C_SDA */
#define     dispTOUCH_I2C_SCL_PIN       GPIO_NUM_1      /* IO1_I2C_SCL */
#define     dispTOUCH_GPIO_INT_PIN      GPIO_NUM_NC     /* 触摸中断引脚（可选） */
#define     dispTOUCH_GPIO_RST_PIN      GPIO_NUM_NC     /* 触摸复位引脚（可选） */

/* 端口电平控制宏 */
#define     dispSPI_CS_SET()            gpio_set_level(dispSPI_GPIO_CS_PIN, 1)
#define     dispSPI_CS_CLR()            gpio_set_level(dispSPI_GPIO_CS_PIN, 0)

#define     dispLCD_DC_SET()            gpio_set_level(dispLCD_GPIO_DC_PIN, 1)
#define     dispLCD_DC_CLR()            gpio_set_level(dispLCD_GPIO_DC_PIN, 0)

#define     dispLCD_BLK_SET()           gpio_set_level(dispLCD_GPIO_BLK_PIN, 0)
#define     dispLCD_BLK_CLR()           gpio_set_level(dispLCD_GPIO_BLK_PIN, 1)

/* 显示驱动模式选择: 1=使用ESP-LCD框架, 0=使用传统DMA方式(备用) */
#define dispUSE_ESP_LCD_FRAMEWORK   1

#if (boardDISPLAY)
/* 跳过lv_conf.h，使用Kconfig配置 */
#ifndef LV_CONF_SKIP
#define LV_CONF_SKIP 1
#endif
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"

/* ESP-LCD 头文件 - 需要 esp_lcd 组件 */
#include "esp_lcd_types.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

/* 触摸屏头文件 - 条件编译避免未安装组件时出错 */
#if __has_include("esp_lcd_touch.h")
#include "esp_lcd_touch.h"
#define ESP_LCD_TOUCH_ENABLED   1
#else
#define ESP_LCD_TOUCH_ENABLED   0
#endif

#include "lvgl.h"

/* ==========================================macros======================================*/
/* LCD最大传输字节数 */
#define dispLCD_BUF_SIZE            4096

/* SPI时钟配置 */
#define dispSPI_CLK_SPEED_HZ        (40 * 1000 * 1000)  /* ESP-LCD模式：40MHz */

/* DMA传输队列大小 */
#define dispDMA_QUEUE_SIZE          10

/* I2C配置 */
#define dispTOUCH_I2C_NUM           I2C_NUM_0
#define dispTOUCH_I2C_CLK_SPEED     400000              /* 400kHz */

/* ==========================================globals=====================================*/
/* LCD面板IO句柄 */
#if(dispUSE_ESP_LCD_FRAMEWORK)
extern esp_lcd_panel_io_handle_t P_panel_io;
#endif  //dispUSE_ESP_LCD_FRAMEWORK

/* LCD面板句柄 */
#if(dispUSE_ESP_LCD_FRAMEWORK)
extern esp_lcd_panel_handle_t P_panel_handle;
#endif  //dispUSE_ESP_LCD_FRAMEWORK

/* 触摸屏句柄 - 条件编译 */
#if ESP_LCD_TOUCH_ENABLED
extern esp_lcd_touch_handle_t P_touch_handle;
#else
typedef void* esp_lcd_touch_handle_t;
extern esp_lcd_touch_handle_t P_touch_handle;
#endif

/* SPI设备句柄 - 用于传统SPI传输 */
extern spi_device_handle_t LCD_Handle;

extern lv_display_t *display;

/* ==========================================extern======================================*/
/* 接口初始化 */
void vDisp_IfaceInit(void);

/* 背光控制 */
void vDisp_SetBacklight(uint8_t uc_on);
uint8_t ucDisp_GetBacklight(void);

/* SPI底层传输函数（用于兼容原有驱动API） */
void vDisp_SPI_WR_Bus(spi_device_handle_t handle, uint8_t uc_data);
void vDisp_SPI_Write_Data(spi_device_handle_t handle, const uint8_t *puc_data, int len);
esp_err_t eDisp_SPI_Write_Data_DMA_Batch(spi_device_handle_t handle, const uint8_t *puc_data, int total_len);

/* DMA内存信息查询 */
void vDisp_PrintDmaMemoryInfo(void);

/* 触摸屏初始化 */
esp_err_t eDisp_TouchInit(void);

/* DMA传输完成回调已内置在ESP-LCD IO配置中，直接使用全局display变量通知LVGL */

#endif  /* boardDISPLAY */

#ifdef __cplusplus
}
#endif

#endif  /* MD_DISPLAY_IFACE_H */
