/*******************************************************************************************************************************
 * Project : G02
 * Module  : G:\TempFile\G02\Hardware\MD_Display
 * File    : md_display_api.c
 * Date    : 2026-01-23 17:48:41
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示屏API层 - 底层绘图函数和初始化
 * -------------------------------------------------------
 * History :
 * 2026-02-07 适配ESP-LCD驱动，优化绘图函数
 * 2026-02-24 优化：移除未使用的函数（SetCursor, FastFillColor, EnableTE等）
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "board_config.h"
#include "MD_Display/md_display_api.h"

#if(boardDISPLAY)
#include "MD_Display/md_display_iface.h"
#include "esp_log.h"
#include "string.h"

//****************************************************Macros*******************************************************************//
#define TAG "DISP_API"

/* 刷屏缓冲区 - 用于纯色填充 */
static uint8_t S_lcd_fill_buf[dispLCD_BUF_SIZE];

//****************************************************Static Function Declaration***********************************************/

/**
 * @brief 发送初始化命令序列
 */
static void lcd_send_init_commands(void);

//****************************************************Function Implementation**************************************************/

/**
 * @brief       向液晶写寄存器命令
 * @param       uc_reg: 要写的命令
 * @retval      无
 */
void vDisp_LCD_WR_REG(uint8_t uc_reg)
{
    dispLCD_DC_CLR();
    vDisp_SPI_WR_Bus(LCD_Handle, uc_reg);
    dispLCD_DC_SET();
}

/**
 * @brief       向液晶写一个字节数据
 * @param       uc_dat: 要写的数据
 * @retval      无
 */
void vDisp_LCD_WR_Byte(uint8_t uc_dat)
{
    dispLCD_DC_SET();
    vDisp_SPI_WR_Bus(LCD_Handle, uc_dat);
}

/**
 * @brief       向液晶写一个半字数据（16位）
 * @param       us_dat: 要写的数据
 * @retval      无
 */
void vDisp_LCD_WR_HalfWord(uint16_t us_dat)
{
    dispLCD_DC_SET();
    vDisp_SPI_WR_Bus(LCD_Handle, us_dat >> 8);
    vDisp_SPI_WR_Bus(LCD_Handle, us_dat & 0xFF);
}

/**
 * @brief       向液晶连续写入len个字节数据
 * @param       puc_dat: 要写的数据地址
 * @param       len：要写入字节长度
 * @retval      无
 */
void vDisp_LCD_WR_DATA(const uint8_t *puc_dat, int len)
{
    if (len == 0) return;
    dispLCD_DC_SET();
    vDisp_SPI_Write_Data(LCD_Handle, puc_dat, len);
}

/**
 * @brief       设置显示窗口
 * @param       us_xs:窗口列起始地址
 * @param       us_ys:坐标行起始地址
 * @param       us_xe:窗口列结束地址
 * @param       us_ye:坐标行结束地址
 * @retval      无
 */
void vDisp_LCD_Address_Set(uint16_t us_xs, uint16_t us_ys, uint16_t us_xe, uint16_t us_ye)
{
    int x_offset = 0, y_offset = 0;
    
    switch(USE_HORIZONTAL) {
        case 0: x_offset = 12; break;
        case 1: x_offset = 14; break;
        case 2: y_offset = 14; break;
        case 3: y_offset = 12; break;
    }
    
    vDisp_LCD_WR_REG(0x2a);
    vDisp_LCD_WR_HalfWord(us_xs + x_offset);
    vDisp_LCD_WR_HalfWord(us_xe + x_offset);
    vDisp_LCD_WR_REG(0x2b);
    vDisp_LCD_WR_HalfWord(us_ys + y_offset);
    vDisp_LCD_WR_HalfWord(us_ye + y_offset);
    vDisp_LCD_WR_REG(0x2c);
}

/**
 * @brief       指定颜色填充区域（使用缓冲区优化）
 * @param       us_xs:填充区域列起始地址
 * @param       us_ys:填充区域行起始地址
 * @param       us_xe:填充区域列结束地址
 * @param       us_ye:填充区域行结束地址
 * @param       us_color:填充颜色值
 * @retval      无
 */
void vDisp_LCD_Fill(uint16_t us_xs, uint16_t us_ys, uint16_t us_xe, uint16_t us_ye, uint16_t us_color)
{
    uint32_t pixel_count = (uint32_t)(us_xe - us_xs + 1) * (us_ye - us_ys + 1);
    uint32_t byte_count = pixel_count * 2;
    
    vDisp_LCD_Address_Set(us_xs, us_ys, us_xe, us_ye);
    
    uint8_t color_high = us_color >> 8;
    uint8_t color_low = us_color & 0xFF;
    
    /* 预填充缓冲区 */
    for (uint32_t i = 0; i < dispLCD_BUF_SIZE / 2; i++) {
        S_lcd_fill_buf[i * 2] = color_high;
        S_lcd_fill_buf[i * 2 + 1] = color_low;
    }
    
    /* 分批次发送数据 */
    while (byte_count > 0) {
        uint32_t bytes_to_send = (byte_count > dispLCD_BUF_SIZE) ? dispLCD_BUF_SIZE : byte_count;
        vDisp_LCD_WR_DATA(S_lcd_fill_buf, bytes_to_send);
        byte_count -= bytes_to_send;
    }
}

/**
 * @brief       初始化LCD
 * @param       无
 * @retval      无
 */
void vDisp_LCD_Init(void)
{
    lcd_send_init_commands();
    vDisp_LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, BLACK);
    vDisp_SetBacklight(1);
}

/**
 * @brief       发送初始化命令序列
 */
static void lcd_send_init_commands(void)
{
    /* 发送扩展寄存器解锁序列 */
    vDisp_LCD_WR_REG(0xff); vDisp_LCD_WR_Byte(0xa5);
    vDisp_LCD_WR_REG(0x9a); vDisp_LCD_WR_Byte(0x08);
    vDisp_LCD_WR_REG(0x9b); vDisp_LCD_WR_Byte(0x08);
    vDisp_LCD_WR_REG(0x9c); vDisp_LCD_WR_Byte(0xb0);
    vDisp_LCD_WR_REG(0x9d); vDisp_LCD_WR_Byte(0x16);
    vDisp_LCD_WR_REG(0x9e); vDisp_LCD_WR_Byte(0xc4);
    vDisp_LCD_WR_REG(0x8f); vDisp_LCD_WR_Byte(0x55); vDisp_LCD_WR_Byte(0x04);
    
    /* 电压设置 */
    vDisp_LCD_WR_REG(0x84); vDisp_LCD_WR_Byte(0x90);
    vDisp_LCD_WR_REG(0x83); vDisp_LCD_WR_Byte(0x7b);
    vDisp_LCD_WR_REG(0x85); vDisp_LCD_WR_Byte(0x33);
    
    /* Gamma校正设置 */
    vDisp_LCD_WR_REG(0x60); vDisp_LCD_WR_Byte(0x00);
    vDisp_LCD_WR_REG(0x70); vDisp_LCD_WR_Byte(0x00);
    vDisp_LCD_WR_REG(0x61); vDisp_LCD_WR_Byte(0x02);
    vDisp_LCD_WR_REG(0x71); vDisp_LCD_WR_Byte(0x02);
    vDisp_LCD_WR_REG(0x62); vDisp_LCD_WR_Byte(0x04);
    vDisp_LCD_WR_REG(0x72); vDisp_LCD_WR_Byte(0x04);
    vDisp_LCD_WR_REG(0x6c); vDisp_LCD_WR_Byte(0x29);
    vDisp_LCD_WR_REG(0x7c); vDisp_LCD_WR_Byte(0x29);
    vDisp_LCD_WR_REG(0x6d); vDisp_LCD_WR_Byte(0x31);
    vDisp_LCD_WR_REG(0x7d); vDisp_LCD_WR_Byte(0x31);
    vDisp_LCD_WR_REG(0x6e); vDisp_LCD_WR_Byte(0x0f);
    vDisp_LCD_WR_REG(0x7e); vDisp_LCD_WR_Byte(0x0f);
    vDisp_LCD_WR_REG(0x66); vDisp_LCD_WR_Byte(0x21);
    vDisp_LCD_WR_REG(0x76); vDisp_LCD_WR_Byte(0x21);
    vDisp_LCD_WR_REG(0x68); vDisp_LCD_WR_Byte(0x3A);
    vDisp_LCD_WR_REG(0x78); vDisp_LCD_WR_Byte(0x3A);
    vDisp_LCD_WR_REG(0x63); vDisp_LCD_WR_Byte(0x07);
    vDisp_LCD_WR_REG(0x73); vDisp_LCD_WR_Byte(0x07);
    vDisp_LCD_WR_REG(0x64); vDisp_LCD_WR_Byte(0x05);
    vDisp_LCD_WR_REG(0x74); vDisp_LCD_WR_Byte(0x05);
    vDisp_LCD_WR_REG(0x65); vDisp_LCD_WR_Byte(0x02);
    vDisp_LCD_WR_REG(0x75); vDisp_LCD_WR_Byte(0x02);
    vDisp_LCD_WR_REG(0x67); vDisp_LCD_WR_Byte(0x23);
    vDisp_LCD_WR_REG(0x77); vDisp_LCD_WR_Byte(0x23);
    vDisp_LCD_WR_REG(0x69); vDisp_LCD_WR_Byte(0x08);
    vDisp_LCD_WR_REG(0x79); vDisp_LCD_WR_Byte(0x08);
    vDisp_LCD_WR_REG(0x6a); vDisp_LCD_WR_Byte(0x13);
    vDisp_LCD_WR_REG(0x7a); vDisp_LCD_WR_Byte(0x13);
    vDisp_LCD_WR_REG(0x6b); vDisp_LCD_WR_Byte(0x13);
    vDisp_LCD_WR_REG(0x7b); vDisp_LCD_WR_Byte(0x13);
    vDisp_LCD_WR_REG(0x6f); vDisp_LCD_WR_Byte(0x00);
    vDisp_LCD_WR_REG(0x7f); vDisp_LCD_WR_Byte(0x00);
    
    /* 源驱动设置 */
    vDisp_LCD_WR_REG(0x50); vDisp_LCD_WR_Byte(0x00);
    vDisp_LCD_WR_REG(0x52); vDisp_LCD_WR_Byte(0xd6);
    vDisp_LCD_WR_REG(0x53); vDisp_LCD_WR_Byte(0x08);
    vDisp_LCD_WR_REG(0x54); vDisp_LCD_WR_Byte(0x08);
    vDisp_LCD_WR_REG(0x55); vDisp_LCD_WR_Byte(0x1e);
    vDisp_LCD_WR_REG(0x56); vDisp_LCD_WR_Byte(0x1c);
    
    /* GOA Map选择 */
    vDisp_LCD_WR_REG(0xa0);
    vDisp_LCD_WR_Byte(0x2b); vDisp_LCD_WR_Byte(0x24); vDisp_LCD_WR_Byte(0x00);
    
    /* 时序设置 */
    vDisp_LCD_WR_REG(0xa1); vDisp_LCD_WR_Byte(0x87);
    vDisp_LCD_WR_REG(0xa2); vDisp_LCD_WR_Byte(0x86);
    vDisp_LCD_WR_REG(0xa5); vDisp_LCD_WR_Byte(0x00);
    vDisp_LCD_WR_REG(0xa6); vDisp_LCD_WR_Byte(0x00);
    vDisp_LCD_WR_REG(0xa7); vDisp_LCD_WR_Byte(0x00);
    vDisp_LCD_WR_REG(0xa8); vDisp_LCD_WR_Byte(0x36);
    vDisp_LCD_WR_REG(0xa9); vDisp_LCD_WR_Byte(0x7e);
    vDisp_LCD_WR_REG(0xaa); vDisp_LCD_WR_Byte(0x7e);
    vDisp_LCD_WR_REG(0xB9); vDisp_LCD_WR_Byte(0x85);
    vDisp_LCD_WR_REG(0xBA); vDisp_LCD_WR_Byte(0x84);
    vDisp_LCD_WR_REG(0xBB); vDisp_LCD_WR_Byte(0x83);
    vDisp_LCD_WR_REG(0xBC); vDisp_LCD_WR_Byte(0x82);
    vDisp_LCD_WR_REG(0xBD); vDisp_LCD_WR_Byte(0x81);
    vDisp_LCD_WR_REG(0xBE); vDisp_LCD_WR_Byte(0x80);
    vDisp_LCD_WR_REG(0xBF); vDisp_LCD_WR_Byte(0x01);
    vDisp_LCD_WR_REG(0xC0); vDisp_LCD_WR_Byte(0x02);
    vDisp_LCD_WR_REG(0xc1); vDisp_LCD_WR_Byte(0x00);
    vDisp_LCD_WR_REG(0xc2); vDisp_LCD_WR_Byte(0x00);
    vDisp_LCD_WR_REG(0xc3); vDisp_LCD_WR_Byte(0x00);
    vDisp_LCD_WR_REG(0xc4); vDisp_LCD_WR_Byte(0x33);
    vDisp_LCD_WR_REG(0xc5); vDisp_LCD_WR_Byte(0x7e);
    vDisp_LCD_WR_REG(0xc6); vDisp_LCD_WR_Byte(0x7e);
    vDisp_LCD_WR_REG(0xC8); vDisp_LCD_WR_Byte(0x33); vDisp_LCD_WR_Byte(0x33);
    vDisp_LCD_WR_REG(0xC9); vDisp_LCD_WR_Byte(0x68);
    vDisp_LCD_WR_REG(0xCA); vDisp_LCD_WR_Byte(0x69);
    vDisp_LCD_WR_REG(0xCB); vDisp_LCD_WR_Byte(0x6a);
    vDisp_LCD_WR_REG(0xCC); vDisp_LCD_WR_Byte(0x6b);
    vDisp_LCD_WR_REG(0xCD); vDisp_LCD_WR_Byte(0x33); vDisp_LCD_WR_Byte(0x33);
    vDisp_LCD_WR_REG(0xCE); vDisp_LCD_WR_Byte(0x6c);
    vDisp_LCD_WR_REG(0xCF); vDisp_LCD_WR_Byte(0x6d);
    vDisp_LCD_WR_REG(0xD0); vDisp_LCD_WR_Byte(0x6e);
    vDisp_LCD_WR_REG(0xD1); vDisp_LCD_WR_Byte(0x6f);
    
    /* 其他设置 */
    vDisp_LCD_WR_REG(0xAB); vDisp_LCD_WR_Byte(0x03); vDisp_LCD_WR_Byte(0x67);
    vDisp_LCD_WR_REG(0xAC); vDisp_LCD_WR_Byte(0x03); vDisp_LCD_WR_Byte(0x6b);
    vDisp_LCD_WR_REG(0xAD); vDisp_LCD_WR_Byte(0x03); vDisp_LCD_WR_Byte(0x68);
    vDisp_LCD_WR_REG(0xAE); vDisp_LCD_WR_Byte(0x03); vDisp_LCD_WR_Byte(0x6c);
    vDisp_LCD_WR_REG(0xb3); vDisp_LCD_WR_Byte(0x00);
    vDisp_LCD_WR_REG(0xb4); vDisp_LCD_WR_Byte(0x00);
    vDisp_LCD_WR_REG(0xb5); vDisp_LCD_WR_Byte(0x00);
    vDisp_LCD_WR_REG(0xB6); vDisp_LCD_WR_Byte(0x32);
    vDisp_LCD_WR_REG(0xB7); vDisp_LCD_WR_Byte(0x7e);
    vDisp_LCD_WR_REG(0xB8); vDisp_LCD_WR_Byte(0x7e);
    
    /* 电源设置 */
    vDisp_LCD_WR_REG(0xe0); vDisp_LCD_WR_Byte(0x00);
    vDisp_LCD_WR_REG(0xe1); vDisp_LCD_WR_Byte(0x03); vDisp_LCD_WR_Byte(0x0f);
    vDisp_LCD_WR_REG(0xe2); vDisp_LCD_WR_Byte(0x04);
    vDisp_LCD_WR_REG(0xe3); vDisp_LCD_WR_Byte(0x01);
    vDisp_LCD_WR_REG(0xe4); vDisp_LCD_WR_Byte(0x0e);
    vDisp_LCD_WR_REG(0xe5); vDisp_LCD_WR_Byte(0x01);
    vDisp_LCD_WR_REG(0xe6); vDisp_LCD_WR_Byte(0x19);
    vDisp_LCD_WR_REG(0xe7); vDisp_LCD_WR_Byte(0x10);
    vDisp_LCD_WR_REG(0xe8); vDisp_LCD_WR_Byte(0x10);
    vDisp_LCD_WR_REG(0xea); vDisp_LCD_WR_Byte(0x12);
    vDisp_LCD_WR_REG(0xeb); vDisp_LCD_WR_Byte(0xd0);
    vDisp_LCD_WR_REG(0xec); vDisp_LCD_WR_Byte(0x04);
    vDisp_LCD_WR_REG(0xed); vDisp_LCD_WR_Byte(0x07);
    vDisp_LCD_WR_REG(0xee); vDisp_LCD_WR_Byte(0x07);
    vDisp_LCD_WR_REG(0xef); vDisp_LCD_WR_Byte(0x09);
    vDisp_LCD_WR_REG(0xf0); vDisp_LCD_WR_Byte(0xd0);
    vDisp_LCD_WR_REG(0xf1); vDisp_LCD_WR_Byte(0x0e);
    
    /* 扫描设置 */
    vDisp_LCD_WR_REG(0xf2);
    vDisp_LCD_WR_Byte(0x2c); vDisp_LCD_WR_Byte(0x1b); vDisp_LCD_WR_Byte(0x0b); vDisp_LCD_WR_Byte(0x20);
    
    /* 1 dot模式 */
    vDisp_LCD_WR_REG(0xe9); vDisp_LCD_WR_Byte(0x29);
    vDisp_LCD_WR_REG(0xec); vDisp_LCD_WR_Byte(0x04);
    
    /* TE设置 */
    vDisp_LCD_WR_REG(0x35); vDisp_LCD_WR_Byte(0x00);
    vDisp_LCD_WR_REG(0x44); vDisp_LCD_WR_Byte(0x00); vDisp_LCD_WR_Byte(0x10);
    vDisp_LCD_WR_REG(0x46); vDisp_LCD_WR_Byte(0x10);
    
    /* 退出扩展寄存器 */
    vDisp_LCD_WR_REG(0xff); vDisp_LCD_WR_Byte(0x00);
    
    /* 像素格式 */
    vDisp_LCD_WR_REG(0x3a); vDisp_LCD_WR_Byte(0x05);
    
    /* 内存访问控制（方向） */
    vDisp_LCD_WR_REG(0x36);
    const uint8_t madctl_table[4] = {0x00, 0xC0, 0x60, 0xA0};
    vDisp_LCD_WR_Byte(madctl_table[USE_HORIZONTAL % 4]);
    
    /* 退出休眠 */
    vDisp_LCD_WR_REG(0x11);
    vTaskDelay(pdMS_TO_TICKS(220));
    
    /* 开启显示 */
    vDisp_LCD_WR_REG(0x29);
    vTaskDelay(pdMS_TO_TICKS(200));
}

/**
 * @brief       液晶进入休眠
 * @param       无
 * @retval      无
 */
void vDisp_LCD_Enter_Sleep(void)
{
    vDisp_LCD_WR_REG(0x28);
    vTaskDelay(pdMS_TO_TICKS(120));
    vDisp_LCD_WR_REG(0x10);
    vTaskDelay(pdMS_TO_TICKS(50));
    vDisp_SetBacklight(0);
    
    ESP_LOGI(TAG, "LCD entered sleep mode");
}

/**
 * @brief       液晶退出休眠
 * @param       无
 * @retval      无
 */
void vDisp_LCD_Exit_Sleep(void)
{
    vDisp_LCD_WR_REG(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));
    vDisp_LCD_WR_REG(0x29);
    vDisp_SetBacklight(1);
    
    ESP_LOGI(TAG, "LCD exited sleep mode");
}

/**
 * @brief       清屏函数
 * @param       us_color: 填充颜色
 * @retval      无
 */
void vDisp_LCD_Clear(uint16_t us_color)
{
    vDisp_LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, us_color);
}

/**
 * @brief       获取显示屏宽度
 * @param       无
 * @retval      宽度像素数
 */
uint16_t usDisp_LCD_GetWidth(void)
{
    return LCD_W;
}

/**
 * @brief       获取显示屏高度
 * @param       无
 * @retval      高度像素数
 */
uint16_t usDisp_LCD_GetHeight(void)
{
    return LCD_H;
}

#endif  /* boardDISPLAY */
