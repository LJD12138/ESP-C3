/*******************************************************************************************************************************
 * Project : G02
 * Module  : G:\TempFile\G02\Hardware\MD_Display
 * File    : md_display_api.h
 * Date    : 2026-01-23 17:48:59
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示屏API层头文件 - ST7789版本
 * -------------------------------------------------------
 * History :
 * 2026-02-07 添加新的API函数声明
 * 2026-02-24 优化：移除未使用的函数声明，更新函数命名规范
 * 2026-03-04 修改：适配ST7789驱动，分辨率320x240
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
*******************************************************************************************************************************/

#ifndef MD_DISPLAY_API_H
#define MD_DISPLAY_API_H

//****************************************************Includes******************************************************************//
#include "board_config.h"
#include "MD_Display/md_display_iface.h"

#if(boardDISPLAY)

//****************************************************Macros*******************************************************************//
/* 定义画笔颜色 - RGB565格式 */
#define WHITE       0xFFFF
#define BLACK       0x0000
#define BLUE        0x001F
#define BRED        0xF81F
#define GRED        0xFFE0
#define GBLUE       0x07FF
#define RED         0xF800
#define MAGENTA     0xF81F
#define GREEN       0x07E0
#define CYAN        0x7FFF
#define YELLOW      0xFFE0
#define BROWN       0xBC40
#define BRRED       0xFC07
#define GRAY        0x8430
#define DARKBLUE    0x01CF
#define LIGHTBLUE   0x7D7C
#define GRAYBLUE    0x5458
#define LIGHTGREEN  0x841F
#define LGRAY       0xC618
#define LGRAYBLUE   0xA651
#define LBBLUE      0x2B12

/* 定义液晶分辨率 - ST7789 320x240横屏 */
#define USE_HORIZONTAL  1

#define LCD_W       320
#define LCD_H       240

//****************************************************Parameter Initialization************************************************//
/* SPI设备句柄 - 用于传统SPI传输 */
extern spi_device_handle_t LCD_Handle;

//****************************************************Function Declaration****************************************************//

/* ==================== 基础操作函数 ==================== */
void vDisp_LCD_WR_REG(uint8_t uc_reg);
void vDisp_LCD_WR_Byte(uint8_t uc_dat);
void vDisp_LCD_WR_HalfWord(uint16_t us_dat);
void vDisp_LCD_WR_DATA(const uint8_t *puc_dat, int len);

/* ==================== 窗口设置 ==================== */
void vDisp_LCD_Address_Set(uint16_t us_xs, uint16_t us_ys, uint16_t us_xe, uint16_t us_ye);

/* ==================== 填充函数 ==================== */
void vDisp_LCD_Fill(uint16_t us_xs, uint16_t us_ys, uint16_t us_xe, uint16_t us_ye, uint16_t us_color);
void vDisp_LCD_Clear(uint16_t us_color);

/* ==================== 初始化和控制 ==================== */
void vDisp_LCD_Init(void);
void vDisp_LCD_Enter_Sleep(void);
void vDisp_LCD_Exit_Sleep(void);

/* ==================== 信息获取 ==================== */
uint16_t usDisp_LCD_GetWidth(void);
uint16_t usDisp_LCD_GetHeight(void);

#endif  /* boardDISPLAY */

#endif  /* MD_DISPLAY_API_H */
