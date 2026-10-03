/*******************************************************************************************************************************
 * Project : G02
 * Module  : G:\TempFile\G02\Hardware\MD_Display
 * File    : md_display_iface.c
 * Date    : 2026-01-23 17:26:05
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示屏接口层 - ESP32 ESP-LCD框架适配（ST7789驱动 + FT6336触摸屏）
 * -------------------------------------------------------
 * History :
 * 2026-02-07 适配NV3007驱动，使用ESP-LCD IO层+自定义API方式
 * 2026-02-24 完全采用ESP-LCD框架，使用esp_lcd_panel_nv3007_init
 * 2026-02-24 优化：移除未使用的DMA异步传输函数和冗余代码
 * 2026-03-04 修改：适配ST7789 LCD驱动和FT6336触摸屏驱动
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
*******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_iface.h"

#if(boardDISPLAY)
#include "MD_Display/md_display_api.h"  /* 包含LCD_W/LCD_H定义 */
#include "lvgl.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "string.h"

/* ST7789 驱动头文件 - 条件编译 */
#if __has_include("esp_lcd_st7789.h")
#include "esp_lcd_st7789.h"
#endif

/* FT6336 触摸屏驱动头文件 - 条件编译 */
#if ESP_LCD_TOUCH_ENABLED
#if __has_include("esp_lcd_touch_ft6x36.h")
#include "esp_lcd_touch_ft6x36.h"
#endif
#endif

//****************************************************Macros*******************************************************************//
#define TAG "DISP_IFACE"

/* LCD面板IO句柄 - ESP-LCD IO层 */
#if(dispUSE_ESP_LCD_FRAMEWORK)
esp_lcd_panel_io_handle_t P_panel_io = NULL;
#endif  //dispUSE_ESP_LCD_FRAMEWORK

/* LCD面板句柄 - ST7789面板驱动 */
#if(dispUSE_ESP_LCD_FRAMEWORK)
esp_lcd_panel_handle_t P_panel_handle = NULL;
#endif  //dispUSE_ESP_LCD_FRAMEWORK

/* 触摸屏句柄 */
esp_lcd_touch_handle_t P_touch_handle = NULL;

/* SPI设备句柄 - 用于传统SPI传输（兼容模式） */
spi_device_handle_t LCD_Handle = NULL;

// contains callback functions
lv_display_t *display = NULL;

//****************************************************Static Function Declaration************************************************//

static void lcd_gpio_init(void);
static esp_err_t lcd_spi_init(void);
static esp_err_t touch_i2c_init(void);

#if(dispUSE_ESP_LCD_FRAMEWORK)
static bool lcd_dma_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);
#endif  //dispUSE_ESP_LCD_FRAMEWORK

//****************************************************Function Implementation****************************************************/

/***********************************************************************************************************************
-----功能    GPIO口初始化
-----说明        配置LCD控制引脚（DC、BLK）和触摸屏I2C引脚
-----传入参数    无
-----输出参数    无
-----返回值      无
************************************************************************************************************************/ 
static void lcd_gpio_init(void)
{
    /* LCD DC引脚配置 */
    gpio_config_t t_io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << dispLCD_GPIO_DC_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&t_io_conf);
    
    /* 配置背光引脚（输入输出模式支持读取状态） */
    t_io_conf.pin_bit_mask = (1ULL << dispLCD_GPIO_BLK_PIN);
    t_io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    t_io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&t_io_conf);
    
    /* 初始状态设置 */
    dispLCD_DC_SET();
    dispLCD_BLK_CLR();
}

/***********************************************************************************************************************
-----功能    SPI总线初始化
-----说明        初始化SPI2主机，配置DMA通道
-----传入参数    无
-----输出参数    无
-----返回值      ESP_OK成功，其他失败
************************************************************************************************************************/
static esp_err_t lcd_spi_init(void)
{
    const spi_bus_config_t t_buscfg = {
        .sclk_io_num = dispSPI_GPIO_CLK_PIN,
        .mosi_io_num = dispSPI_GPIO_MOSI_PIN,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_W * LCD_H * 2,
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };
    
    esp_err_t e_ret = spi_bus_initialize(SPI2_HOST, &t_buscfg, SPI_DMA_CH_AUTO);
    if (e_ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus initialization failed: %s", esp_err_to_name(e_ret));
        return e_ret;
    }

    return ESP_OK;
}

/***********************************************************************************************************************
-----功能    I2C总线初始化（用于触摸屏）
-----说明        初始化I2C0主机
-----传入参数    无
-----输出参数    无
-----返回值      ESP_OK成功，其他失败
************************************************************************************************************************/
static esp_err_t touch_i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = dispTOUCH_I2C_SDA_PIN,
        .scl_io_num = dispTOUCH_I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = dispTOUCH_I2C_CLK_SPEED,
    };
    
    esp_err_t ret = i2c_param_config(dispTOUCH_I2C_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = i2c_driver_install(dispTOUCH_I2C_NUM, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

/***********************************************************************************************************************
-----功能    接口初始化（外部调用接口）- ESP-LCD框架版本
-----说明        使用ESP-LCD框架初始化ST7789显示屏和FT6336触摸屏
-----传入参数    无
-----输出参数    无
-----返回值      无
************************************************************************************************************************/
void vDisp_IfaceInit(void)
{
    vDisp_PrintDmaMemoryInfo();
    
    /* 1. 初始化GPIO */
    lcd_gpio_init();
    
    /* 2. 初始化SPI总线 */
    if (lcd_spi_init() != ESP_OK) {
        ESP_LOGE(TAG, "SPI initialization failed");
        return;
    }
    
#if(dispUSE_ESP_LCD_FRAMEWORK)
    /* 3. 创建ESP-LCD IO层（SPI），注册DMA传输完成回调 */
    esp_lcd_panel_io_spi_config_t t_io_config = {
        .dc_gpio_num = dispLCD_GPIO_DC_PIN,
        .cs_gpio_num = dispSPI_GPIO_CS_PIN,
        .pclk_hz = dispSPI_CLK_SPEED_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = dispDMA_QUEUE_SIZE,
        .on_color_trans_done = lcd_dma_trans_done_cb,
        .user_ctx = &display,
    };
    
    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &t_io_config, &P_panel_io) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LCD panel IO");
        return;
    }
    
    /* 4. 使用ESP-LCD框架初始化ST7789面板 */
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = dispLCD_GPIO_RES_PIN,
        .rgb_endian = LCD_RGB_ENDIAN_RGB,  /* ST7789使用BGR顺序 */
        .bits_per_pixel = 16,
    };
    
    if (esp_lcd_new_panel_st7789(P_panel_io, &panel_config, &P_panel_handle) != ESP_OK) {
        ESP_LOGE(TAG, "ST7789 panel initialization failed");
        return;
    }
    
    /* 5. 配置ST7789面板 */
    esp_lcd_panel_reset(P_panel_handle);                    /* 硬件复位 */
    esp_lcd_panel_init(P_panel_handle);                     /* 初始化 */
    esp_lcd_panel_swap_xy(P_panel_handle, true);            /* 交换XY轴（横屏） */
    esp_lcd_panel_mirror(P_panel_handle, false, true);     /* X轴镜像*/
    esp_lcd_panel_invert_color(P_panel_handle, true);       /* 关闭颜色反转 */
    // esp_lcd_panel_set_gap(P_panel_handle, 0, 0);         /* 无间隙 */
    esp_lcd_panel_disp_on_off(P_panel_handle, true);        /* 开启显示 */

#else
    /* 传统DMA模式：添加SPI设备到总线 */
    const spi_device_interface_config_t t_devcfg = {
        .clock_speed_hz = dispSPI_CLK_SPEED_HZ,
        .mode = 0,
        .spics_io_num = dispSPI_GPIO_CS_PIN,
        .queue_size = dispDMA_QUEUE_SIZE,
        .flags = SPI_DEVICE_NO_DUMMY,
    };
    
    if (spi_bus_add_device(SPI2_HOST, &t_devcfg, &LCD_Handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device");
        return;
    }
    
    ESP_LOGI(TAG, "SPI device added: CS=%d, CLK=%d MHz", 
             dispSPI_GPIO_CS_PIN, dispSPI_CLK_SPEED_HZ / 1000000);
    
    /* 使用传统方式初始化LCD */
    vDisp_LCD_Init();
#endif
    
    /* 6. 开启背光 */
    vDisp_SetBacklight(1);
}

/***********************************************************************************************************************
-----功能    触摸屏初始化
-----说明        初始化FT6336触摸屏控制器
-----传入参数    无
-----输出参数    无
-----返回值      ESP_OK成功，其他失败
************************************************************************************************************************/
esp_err_t eDisp_TouchInit(void)
{
#if ESP_LCD_TOUCH_ENABLED
    /* 1. 初始化I2C总线 */
    if (touch_i2c_init() != ESP_OK) {
        ESP_LOGE(TAG, "Touch I2C initialization failed");
        return ESP_FAIL;
    }
    
    /* 2. 创建触摸IO句柄 */
    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_FT6x36_CONFIG();
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    
    esp_err_t ret = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)dispTOUCH_I2C_NUM, 
                                              &io_config, &tp_io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create touch IO handle: %s", esp_err_to_name(ret));
        return ret;
    }
    
    /* 3. 配置触摸屏 */
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_W,
        .y_max = LCD_H,
        .rst_gpio_num = dispTOUCH_GPIO_RST_PIN,
        .int_gpio_num = dispTOUCH_GPIO_INT_PIN,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 1,       /* 与LCD一致，交换XY */
            .mirror_x = 0,      /* X轴不镜像 */
            .mirror_y = 0,      /* Y轴镜像 */
        },
        .process_coordinates = NULL,
        .interrupt_callback = NULL,
        .user_data = NULL,
        .driver_data = NULL,
    };
    
    /* 4. 创建FT6336触摸驱动 */
    ret = esp_lcd_touch_new_i2c_ft6x36(tp_io_handle, &tp_cfg, &P_touch_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "FT6336 touch initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    return ESP_OK;
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

/***********************************************************************************************************************
-----功能    设置背光亮度
-----说明        控制LCD背光开关
-----传入参数    on: 0关闭，非0开启
-----输出参数    无
-----返回值      无
************************************************************************************************************************/
void vDisp_SetBacklight(uint8_t uc_on)
{
    if (uc_on) {
        dispLCD_BLK_SET();
    } else {
        dispLCD_BLK_CLR();
    }
}

/***********************************************************************************************************************
-----功能    获取背光状态
-----说明        读取LCD背光当前状态
-----传入参数    无
-----输出参数    无
-----返回值      0关闭，1开启
************************************************************************************************************************/
uint8_t ucDisp_GetBacklight(void)
{
    return gpio_get_level(dispLCD_GPIO_BLK_PIN);
}

/***********************************************************************************************************************
-----功能    打印DMA内存信息
-----说明        显示可用DMA内存大小，用于调试内存分配问题
-----传入参数    无
-----输出参数    无
-----返回值      无
************************************************************************************************************************/
void vDisp_PrintDmaMemoryInfo(void)
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
-----功能    SPI写入单字节（兼容原有驱动API）
-----说明        仅用于传统DMA模式
-----传入参数    handle: SPI设备句柄
                data: 要发送的字节数据
-----输出参数    无
-----返回值      无
************************************************************************************************************************/
void vDisp_SPI_WR_Bus(spi_device_handle_t handle, uint8_t uc_data)
{
    spi_transaction_t t_trans = {
        .length = 8,
        .tx_data[0] = uc_data,
        .flags = SPI_TRANS_USE_TXDATA,
    };
    
    esp_err_t e_ret = spi_device_polling_transmit(handle, &t_trans);
    if (e_ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transmit failed: %s", esp_err_to_name(e_ret));
    }
}

/***********************************************************************************************************************
-----功能    SPI写入数据块（兼容原有驱动API）
-----说明        仅用于传统DMA模式
-----传入参数    handle: SPI设备句柄
                puc_data: 数据指针
                len: 数据长度
-----输出参数    无
-----返回值      无
************************************************************************************************************************/
void vDisp_SPI_Write_Data(spi_device_handle_t handle, const uint8_t *puc_data, int len)
{
    spi_transaction_t t_trans = {
        .length = len * 8,
        .tx_buffer = puc_data,
        .rx_buffer = NULL,
    };
    
    esp_err_t e_ret = spi_device_polling_transmit(handle, &t_trans);
    if (e_ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transmit failed: %s", esp_err_to_name(e_ret));
    }
}

/***********************************************************************************************************************
-----功能    SPI DMA批量写入数据
-----说明        分块传输大数据量，仅用于传统DMA模式
-----传入参数    handle: SPI设备句柄
                puc_data: 数据指针
                total_len: 总数据长度
-----输出参数    无
-----返回值      ESP_OK成功，其他失败
************************************************************************************************************************/
esp_err_t eDisp_SPI_Write_Data_DMA_Batch(spi_device_handle_t handle, const uint8_t *puc_data, int total_len)
{
    esp_err_t e_ret = ESP_OK;
    int offset = 0;
    
    while (offset < total_len) {
        int chunk_size = (total_len - offset > dispLCD_BUF_SIZE) ? dispLCD_BUF_SIZE : (total_len - offset);
        
        spi_transaction_t t_trans = {
            .length = chunk_size * 8,
            .tx_buffer = puc_data + offset,
            .rx_buffer = NULL,
        };
        
        e_ret = spi_device_polling_transmit(handle, &t_trans);
        if (e_ret != ESP_OK) {
            ESP_LOGE(TAG, "SPI transmit failed at offset %d: %s", offset, esp_err_to_name(e_ret));
            break;
        }
        offset += chunk_size;
    }
    
    return e_ret;
}

/***********************************************************************************************************************
-----功能    DMA传输完成回调函数
-----说明        ESP-LCD异步DMA传输完成后通知LVGL
-----传入参数    panel_io: 面板IO句柄
                edata: 事件数据
                user_ctx: 用户上下文
-----输出参数    无
-----返回值      false表示不要从ISR中唤醒高优先级任务
************************************************************************************************************************/
#if(dispUSE_ESP_LCD_FRAMEWORK)
static bool lcd_dma_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    /* 使用全局display变量通知LVGL刷新完成 - LVGL v9 API */
    if (display != NULL) {
        lv_display_flush_ready(display);
    }
    return false;
}
#endif  //dispUSE_ESP_LCD_FRAMEWORK

#endif  //boardDISPLAY
