/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display
 * File    : lv_port_disp.c
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : LVGL显示端口适配层
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "lv_port_disp.h"

#if (boardDISPLAY_EN)
#include <stdbool.h>
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_iface.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "sdkconfig.h"

#if (dispUSE_ESP_LCD_FRAMEWORK)
#if __has_include("esp_lcd_st7789.h")
#include "esp_lcd_st7789.h"
#endif  /* __has_include("esp_lcd_st7789.h") */
#endif  /* dispUSE_ESP_LCD_FRAMEWORK */

#if (boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

//****************************************************Macros********************************************************************//
/* 显示缓冲区配置: 80行局部双缓冲 (全帧DMA分块由6次减为3次, 渲染/传输重叠更深;
 * 单缓冲 51.2KB x2 片内DMA内存, 堆不足时 buf2 分配失败自动降级单缓冲) */
#define			DISP_BUF_LINES							80U
#define			BUF_SIZE								(DISP_HOR_RES * DISP_BUF_LINES * sizeof(lv_color_t))

//****************************************************Parameter Initialization**************************************************//
#if (dispUSE_ESP_LCD_FRAMEWORK)
static esp_lcd_panel_handle_t t_lcd_panel_handle = NULL;
#endif  /* dispUSE_ESP_LCD_FRAMEWORK */

//****************************************************Function Declaration******************************************************//
static void lcd_flush_cb(lv_display_t *tpDisp, const lv_area_t *area, uint8_t *px_map);

#if (dispUSE_ESP_LCD_FRAMEWORK)
static esp_err_t lcd_panel_st7789_init(void);
#endif  /* dispUSE_ESP_LCD_FRAMEWORK */


/***********************************************************************************************************************
 * 函数功能    : 初始化LVGL显示驱动
 * 说明(备注)  : 创建LVGL显示设备，分配DMA显存缓冲区并注册刷新回调
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void lv_port_disp_init(void)
{
	/* 1. 初始化 ST7789 屏幕面板驱动 */
	#if (dispUSE_ESP_LCD_FRAMEWORK)
	if (lcd_panel_st7789_init() != ESP_OK)
	{
		ESP_LOGE("LVGL_DISP", "Failed to initialize ST7789 panel driver");
		return;
	}
	#endif  /* dispUSE_ESP_LCD_FRAMEWORK */

	/* 2. 创建显示设备 */
	tpDisp = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
	if (tpDisp == NULL)
	{
		ESP_LOGE("LVGL_DISP", "Failed to create tpDisp");
		return;
	}

	/* 3. 分配DMA缓冲区 (片内连续DMA内存) */
	lv_color_t *p_buf_1 = (lv_color_t *)heap_caps_malloc(BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
	if (p_buf_1 == NULL)
	{
		ESP_LOGE("LVGL_DISP", "Failed to allocate tpDisp buffer 1");
		lv_display_delete(tpDisp);
		tpDisp = NULL;
		return;
	}

	lv_color_t *p_buf_2 = (lv_color_t *)heap_caps_malloc(BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
	if (p_buf_2 == NULL)
		ESP_LOGW("LVGL_DISP", "Failed to allocate buffer 2, using single buffer mode");

	/* 4. 设置显示缓冲区 */
	lv_display_set_buffers(tpDisp, p_buf_1, p_buf_2, BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

	/* 5. 设置刷新回调函数 */
	lv_display_set_flush_cb(tpDisp, lcd_flush_cb);

	/* 6. 启用软件旋转支持 */
	lv_display_set_rotation(tpDisp, LV_DISPLAY_ROTATION_0);

	/* 7. 使用原生大端RGB565格式，直接免除flush_cb中CPU软转字节序开销 */
	lv_display_set_color_format(tpDisp, LV_COLOR_FORMAT_RGB565_SWAPPED);

	/* 8. 设置为默认显示器 */
	lv_display_set_default(tpDisp);
}

/***********************************************************************************************************************
 * 函数功能    : LVGL刷新回调函数
 * 说明(备注)  : 将LVGL渲染好的显存区域推送到屏幕
 * 传入参数    : tpDisp: 显示句柄, area: 区域坐标, px_map: 像素缓冲区指针
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void lcd_flush_cb(lv_display_t *tpDisp, const lv_area_t *area, uint8_t *px_map)
{
	int16_t  x1 = area->x1;
	int16_t  y1 = area->y1;
	int16_t  x2 = area->x2;
	int16_t  y2 = area->y2;

	/* 参数有效性检查 */
	if (px_map == NULL)
	{
		ESP_LOGW("LVGL_DISP", "Invalid flush px_map");
		lv_display_flush_ready(tpDisp);
		return;
	}

	#if (dispUSE_ESP_LCD_FRAMEWORK)
	/* 硬件DMA异步传输，字节序已由LVGL9绘图阶段直接以SWAPPED格式生成，无需CPU遍历转换 */
	esp_err_t e_ret = esp_lcd_panel_draw_bitmap(t_lcd_panel_handle, x1, y1, x2 + 1, y2 + 1, px_map);
	if (e_ret != ESP_OK)
	{
		ESP_LOGE("LVGL_DISP", "Draw bitmap failed: %d", e_ret);
		/* 传输失败时立即通知LVGL，避免任务卡死 */
		lv_display_flush_ready(tpDisp);
	}
	#else
	lv_display_flush_ready(tpDisp);
	#endif  /* dispUSE_ESP_LCD_FRAMEWORK */
}

/***********************************************************************************************************************
 * 函数功能    : ST7789 液晶控制器硬件驱动初始化
 * 说明(备注)  : 创建 ST7789 面板对象并配置显示方向与色彩特性
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : esp_err_t: ESP_OK 成功, 其他失败
 ************************************************************************************************************************/
#if (dispUSE_ESP_LCD_FRAMEWORK)
 static esp_err_t lcd_panel_st7789_init(void)
{
	esp_lcd_panel_dev_config_t panel_config = {
		.reset_gpio_num = dispTFT_RES_PIN,
		.rgb_endian     = LCD_RGB_ENDIAN_RGB,
		.bits_per_pixel = 16,
	};

	esp_err_t ret = esp_lcd_new_panel_st7789(tLcdPanelIoHandle, &panel_config, &t_lcd_panel_handle);
	if (ret != ESP_OK)
	{
		ESP_LOGE("LVGL_DISP", "ST7789 panel initialization failed: %s", esp_err_to_name(ret));
		return ret;
	}

	esp_lcd_panel_reset(t_lcd_panel_handle);
	esp_lcd_panel_init(t_lcd_panel_handle);
	esp_lcd_panel_swap_xy(t_lcd_panel_handle, true);
	esp_lcd_panel_mirror(t_lcd_panel_handle, true, false);
	esp_lcd_panel_invert_color(t_lcd_panel_handle, true);
	esp_lcd_panel_disp_on_off(t_lcd_panel_handle, true);

	return ESP_OK;
}
#endif  /* dispUSE_ESP_LCD_FRAMEWORK */

#endif  /* boardDISPLAY_EN */