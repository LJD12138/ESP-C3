/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display
 * File    : md_display_iface.c
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示屏接口层实现 - ESP-LCD框架适配 (ST7789驱动 + FT6336触摸屏)
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_iface.h"

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_api.h"
#include "lvgl.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include <string.h>

//****************************************************Macros********************************************************************//
#define			TAG										"DISP_IFACE"

//****************************************************Parameter Initialization**************************************************//
#if (dispUSE_ESP_LCD_FRAMEWORK)
esp_lcd_panel_io_handle_t tLcdPanelIoHandle = NULL;
#else
spi_device_handle_t       tSpiDevHandle = NULL;
#endif  /* dispUSE_ESP_LCD_FRAMEWORK */

lv_display_t             *tpDisp = NULL;

static void lcd_gpio_init(void);
static esp_err_t lcd_spi_init(void);

#if (ESP_LCD_TOUCH_ENABLED)
esp_err_t e_touch_i2c_init(void);
#endif  /* ESP_LCD_TOUCH_ENABLED */

#if (dispUSE_ESP_LCD_FRAMEWORK)
static bool lcd_dma_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
#endif  /* dispUSE_ESP_LCD_FRAMEWORK */

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 显示控制 GPIO 初始化
 * 说明(备注)  : 配置 LCD 控制引脚 (DC、BLK)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void lcd_gpio_init(void)
{
	gpio_config_t t_io_conf = {
		.intr_type    = GPIO_INTR_DISABLE,
		.mode         = GPIO_MODE_OUTPUT,
		.pin_bit_mask = (1ULL << dispTFT_DC_PIN),
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.pull_up_en   = GPIO_PULLUP_DISABLE,
	};
	gpio_config(&t_io_conf);

	t_io_conf.pin_bit_mask = (1ULL << dispTFT_BL_PIN);
	t_io_conf.mode         = GPIO_MODE_INPUT_OUTPUT;
	t_io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
	gpio_config(&t_io_conf);

	dispTFT_DC_H();
	dispTFT_BL_L();
}

/***********************************************************************************************************************
 * 函数功能    : SPI 主机总线初始化
 * 说明(备注)  : 初始化 SPI2 主机并配置 DMA 传输通道
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : esp_err_t: ESP_OK 成功, 其他失败
 ************************************************************************************************************************/
static esp_err_t lcd_spi_init(void)
{
	const spi_bus_config_t t_buscfg = {
		.sclk_io_num     = dispTFT_CLK_PIN,
		.mosi_io_num     = dispTFT_MOSI_PIN,
		.miso_io_num     = -1,
		.quadwp_io_num   = -1,
		.quadhd_io_num   = -1,
		.max_transfer_sz = dispTFT_WIDTH * dispTFT_HEIGHT * 2,
		.flags           = SPICOMMON_BUSFLAG_MASTER,
	};

	esp_err_t e_ret = spi_bus_initialize(SPI2_HOST, &t_buscfg, SPI_DMA_CH_AUTO);
	if (e_ret != ESP_OK)
	{
		ESP_LOGE(TAG, "SPI bus initialization failed: %s", esp_err_to_name(e_ret));
		return e_ret;
	}

	return ESP_OK;
}

/***********************************************************************************************************************
 * 函数功能    : I2C 主机总线初始化 (触摸屏)
 * 说明(备注)  : 初始化 I2C0 主机接口
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : esp_err_t: ESP_OK 成功, 其他失败
 ************************************************************************************************************************/
#if (ESP_LCD_TOUCH_ENABLED)
 esp_err_t e_touch_i2c_init(void)
{
	i2c_config_t conf = {
		.mode             = I2C_MODE_MASTER,
		.sda_io_num       = dispTOUCH_SDA_PIN,
		.scl_io_num       = dispTOUCH_SCL_PIN,
		.sda_pullup_en    = GPIO_PULLUP_ENABLE,
		.scl_pullup_en    = GPIO_PULLUP_ENABLE,
		.master.clk_speed = dispTOUCH_CLK_SPEED,
	};

	esp_err_t ret = i2c_param_config(dispTOUCH_NUM, &conf);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(ret));
		return ret;
	}

	ret = i2c_driver_install(dispTOUCH_NUM, conf.mode, 0, 0, 0);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
		return ret;
	}

	return ESP_OK;
}
#endif  /* ESP_LCD_TOUCH_ENABLED */

/***********************************************************************************************************************
 * 函数功能    : 打印 DMA 内存信息
 * 说明(备注)  : 输出可用内部及 DMA 内存大小，供调试使用
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_print_dma_memory_info(void)
{
	size_t dma_free = heap_caps_get_free_size(MALLOC_CAP_DMA);
	size_t dma_largest = heap_caps_get_largest_free_block(MALLOC_CAP_DMA);
	size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

	ESP_LOGI(TAG, "========== DMA Memory Info ==========");
	ESP_LOGI(TAG, "DMA Free:        %d bytes (%d KB)", dma_free, dma_free / 1024);
	ESP_LOGI(TAG, "DMA Largest:     %d bytes (%d KB)", dma_largest, dma_largest / 1024);
	ESP_LOGI(TAG, "Internal Free:   %d bytes (%d KB)", internal_free, internal_free / 1024);
	ESP_LOGI(TAG, "=====================================");
}

/***********************************************************************************************************************
 * 函数功能    : DMA 传输完成回调函数
 * 说明(备注)  : 传输完成后通知 LVGL
 * 传入参数    : panel_io: 面板IO句柄, edata: 事件数据, user_ctx: 用户上下文
 * 输出参数    : 无
 * 返回值      : bool: false 无需唤醒高优先级任务
 ************************************************************************************************************************/
#if (dispUSE_ESP_LCD_FRAMEWORK)
 static bool lcd_dma_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
	(void)panel_io;
	(void)edata;
	(void)user_ctx;

	if (tpDisp != NULL)
		lv_display_flush_ready(tpDisp);

	return false;
}
#endif  /* dispUSE_ESP_LCD_FRAMEWORK */

/***********************************************************************************************************************
 * 函数功能    : 显示接口层总初始化
 * 说明(备注)  : 初始化 GPIO、SPI/I2C 总线并创建 ESP-LCD SPI IO 管道
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_IfaceInit(void)
{
	v_print_dma_memory_info();

	/* 1. 初始化 GPIO */
	lcd_gpio_init();

	/* 2. 初始化 SPI 总线 */
	if (lcd_spi_init() != ESP_OK)
	{
		ESP_LOGE(TAG, "SPI initialization failed");
		return;
	}

	/* 3. 创建 ESP-LCD SPI IO 层并注册 DMA 回调 */
	#if (dispUSE_ESP_LCD_FRAMEWORK)
	esp_lcd_panel_io_spi_config_t t_io_config = {
		.dc_gpio_num         = dispTFT_DC_PIN,
		.cs_gpio_num         = dispTFT_CS_PIN,
		.pclk_hz             = dispSPI_CLK_SPEED_HZ,
		.lcd_cmd_bits        = 8,
		.lcd_param_bits      = 8,
		.spi_mode            = 0,
		.trans_queue_depth   = dispDMA_QUEUE_SIZE,
		.on_color_trans_done = lcd_dma_trans_done_cb,
		.user_ctx            = &tpDisp,
	};

	if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &t_io_config, &tLcdPanelIoHandle) != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to create LCD panel IO");
		return;
	}

	#else
	const spi_device_interface_config_t t_devcfg = {
		.clock_speed_hz = dispSPI_CLK_SPEED_HZ,
		.mode           = 0,
		.spics_io_num   = dispTFT_CS_PIN,
		.queue_size     = dispDMA_QUEUE_SIZE,
		.flags          = SPI_DEVICE_NO_DUMMY,
	};

	if (spi_bus_add_device(SPI2_HOST, &t_devcfg, &tSpiDevHandle) != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to add SPI device");
		return;
	}
	
	#endif  /* dispUSE_ESP_LCD_FRAMEWORK */

	/* 4. 初始化触摸 I2C 总线接口 */
	#if (ESP_LCD_TOUCH_ENABLED)
	if (e_touch_i2c_init() != ESP_OK)
		ESP_LOGE(TAG, "Touch I2C bus initialization failed");
	#endif  /* ESP_LCD_TOUCH_ENABLED */

	/* 5. 默认开启背光 */
	vDisp_SetBacklight(1);
}

/***********************************************************************************************************************
 * 函数功能    : 设置背光开关
 * 说明(备注)  : 控制 LCD 背光使能引脚
 * 传入参数    : uc_on: 0 关闭, 非0 开启
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_SetBacklight(uint8_t uc_on)
{
	if (uc_on)
		dispTFT_BL_H();
	else
		dispTFT_BL_L();
}

/***********************************************************************************************************************
 * 函数功能    : 获取背光状态
 * 说明(备注)  : 读取背光 GPIO 当前电平
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : uint8_t: 0 关闭, 1 开启
 ************************************************************************************************************************/
uint8_t ucDisp_GetBacklight(void)
{
	/* 低电平有效点亮背光: GPIO 为 0 表示开启(1), GPIO 为 1 表示关闭(0) */
	return (gpio_get_level(dispTFT_BL_PIN) == 0) ? 1U : 0U;
}

/***********************************************************************************************************************
 * 函数功能    : SPI 写入单字节 (兼容传统模式)
 * 说明(备注)  : 轮询发送单字节
 * 传入参数    : handle: SPI 设备句柄, uc_data: 发送数据字节
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_WriteData8(spi_device_handle_t handle, uint8_t uc_data)
{
	spi_transaction_t t_trans = {
		.length    = 8,
		.tx_data   = {uc_data},
		.flags     = SPI_TRANS_USE_TXDATA,
	};

	esp_err_t e_ret = spi_device_polling_transmit(handle, &t_trans);
	if (e_ret != ESP_OK)
		ESP_LOGE(TAG, "SPI transmit failed: %s", esp_err_to_name(e_ret));
}

/***********************************************************************************************************************
 * 函数功能    : SPI 写入数据块 (兼容传统模式)
 * 说明(备注)  : 轮询发送数据缓冲区
 * 传入参数    : handle: SPI 设备句柄, puc_data: 数据指针, len: 数据长度
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_WriteBuffer(spi_device_handle_t handle, const uint8_t *puc_data, int len)
{
	spi_transaction_t t_trans = {
		.length    = len * 8,
		.tx_buffer = puc_data,
		.rx_buffer = NULL,
	};

	esp_err_t e_ret = spi_device_polling_transmit(handle, &t_trans);
	if (e_ret != ESP_OK)
		ESP_LOGE(TAG, "SPI transmit failed: %s", esp_err_to_name(e_ret));
}

/***********************************************************************************************************************
 * 函数功能    : SPI DMA 批量写入数据
 * 说明(备注)  : 分块传输大数据量
 * 传入参数    : handle: SPI 设备句柄, puc_data: 数据指针, total_len: 总长度
 * 输出参数    : 无
 * 返回值      : esp_err_t: ESP_OK 成功, 其他失败
 ************************************************************************************************************************/
esp_err_t eDisp_SPI_Write_Data_DMA_Batch(spi_device_handle_t handle, const uint8_t *puc_data, int total_len)
{
	esp_err_t e_ret = ESP_OK;
	int offset = 0;

	while (offset < total_len)
	{
		int chunk_size = (total_len - offset > dispLCD_BUF_SIZE) ? dispLCD_BUF_SIZE : (total_len - offset);

		spi_transaction_t t_trans = {
			.length    = chunk_size * 8,
			.tx_buffer = puc_data + offset,
			.rx_buffer = NULL,
		};

		e_ret = spi_device_polling_transmit(handle, &t_trans);
		if (e_ret != ESP_OK)
		{
			ESP_LOGE(TAG, "SPI transmit failed at offset %d: %s", offset, esp_err_to_name(e_ret));
			break;
		}
		offset += chunk_size;
	}

	return e_ret;
}
#endif  /* boardDISPLAY_EN */
