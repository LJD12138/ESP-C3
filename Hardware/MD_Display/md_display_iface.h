/***********************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display
 * File    : md_display_iface.h
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示屏硬件底层接口定义头文件 (ST7789 SPI + FT6336 I2C触摸屏)
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_DISPLAY_IFACE_H
#define MD_DISPLAY_IFACE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardDISPLAY_EN)
#include "driver/gpio.h"
#include "driver/spi_master.h"

/* ESP-LCD 驱动头文件 */
#include "esp_lcd_types.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

//****************************************************Macros********************************************************************//
#if __has_include("esp_lcd_touch.h")
#include "esp_lcd_touch.h"
#define			ESP_LCD_TOUCH_ENABLED					1
#else
#define			ESP_LCD_TOUCH_ENABLED					0
#endif  /* __has_include("esp_lcd_touch.h") */

#include "lvgl.h"

#define         dispTFT_WIDTH                           320U
#define         dispTFT_HEIGHT                          240U

/* SPI 引脚定义 - J4接口 */
#define			dispTFT_MOSI_PIN						GPIO_NUM_5	/* IO5_LCD_MOSI */
#define			dispTFT_CLK_PIN							GPIO_NUM_3	/* IO3_LCD_SCK */
#define			dispTFT_CS_PIN							GPIO_NUM_4	/* IO4_LCD_CS */

/* LCD 控制引脚定义 */
#define			dispTFT_RES_PIN							GPIO_NUM_NC	/* 未使用硬件复位，使用软件复位 */
#define			dispTFT_DC_PIN							GPIO_NUM_6	/* IO6_LCD_DC */
#define			dispTFT_BL_PIN							GPIO_NUM_2	/* IO2_LCD_BL */

/* I2C 引脚定义 - 触摸屏 FT6336 */
#define			dispTOUCH_SDA_PIN						GPIO_NUM_0	/* IO0_I2C_SDA */
#define			dispTOUCH_SCL_PIN						GPIO_NUM_1	/* IO1_I2C_SCL */
#define			dispTOUCH_INT_PIN						GPIO_NUM_NC	/* 触摸中断引脚 (可选) */
#define			dispTOUCH_RST_PIN						GPIO_NUM_NC	/* 触摸复位引脚 (可选) */

/* 端口电平控制宏 */
#define			dispTFT_CS_H()						    gpio_set_level(dispTFT_CS_PIN, 1)
#define			dispTFT_CS_L()						    gpio_set_level(dispTFT_CS_PIN, 0)

#define			dispTFT_DC_H()						    gpio_set_level(dispTFT_DC_PIN, 1)
#define			dispTFT_DC_L()						    gpio_set_level(dispTFT_DC_PIN, 0)

#define			dispTFT_BL_H()						    gpio_set_level(dispTFT_BL_PIN, 0)
#define			dispTFT_BL_L()						    gpio_set_level(dispTFT_BL_PIN, 1)

/* 显示驱动模式: 1=使用 ESP-LCD 框架, 0=传统模式 */
#define			dispUSE_ESP_LCD_FRAMEWORK				1

/* LCD 传输缓冲区与总线参数 */
#define			dispLCD_BUF_SIZE						4096
#define			dispSPI_CLK_SPEED_HZ					(40 * 1000 * 1000)	/* 40MHz */
#define			dispDMA_QUEUE_SIZE						10

/* I2C 触摸屏配置 */
#define			dispTOUCH_NUM						    I2C_NUM_0
#define			dispTOUCH_CLK_SPEED					    400000	/* 400kHz */


//****************************************************Globals*******************************************************************//
#if (dispUSE_ESP_LCD_FRAMEWORK)
extern esp_lcd_panel_io_handle_t tLcdPanelIoHandle;
#else
extern spi_device_handle_t       tSpiDevHandle;
#endif  /* dispUSE_ESP_LCD_FRAMEWORK */

extern lv_display_t             *tpDisp;

//****************************************************Extern********************************************************************//
void vDisp_IfaceInit(void);
void vDisp_SetBacklight(uint8_t uc_on);
uint8_t ucDisp_GetBacklight(void);
void vDisp_WriteData8(spi_device_handle_t handle, uint8_t uc_data);
void vDisp_WriteBuffer(spi_device_handle_t handle, const uint8_t *puc_data, int len);
esp_err_t eDisp_SPI_Write_Data_DMA_Batch(spi_device_handle_t handle, const uint8_t *puc_data, int total_len);

#endif  /* boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_DISPLAY_IFACE_H */
