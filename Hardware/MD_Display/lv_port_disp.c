#include "lv_port_disp.h"
#include "../../main/board_config.h"

#if(boardDISPLAY)

/*********************
 *      INCLUDES
 *********************/
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_iface.h"
#include <stdbool.h>
#include "esp_heap_caps.h"
#include "sdkconfig.h"
#include "esp_log.h"

/* 跳过lv_conf.h，使用Kconfig配置 */
#ifndef LV_CONF_SKIP
#define LV_CONF_SKIP 1
#endif
#include "lvgl.h"

#if(boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

/*********************
 *      DEFINES
 *********************/
#ifndef MY_DISP_HOR_RES
    #define MY_DISP_HOR_RES    LCD_W
#endif

#ifndef MY_DISP_VER_RES
    #define MY_DISP_VER_RES    LCD_H
#endif

/* 显示缓冲区配置 */
#define BUF_LINES           60
#define BUF_SIZE            (MY_DISP_HOR_RES * BUF_LINES * sizeof(lv_color_t))

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lcd_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief 初始化LVGL显示驱动
 */
void lv_port_disp_init(void)
{
    /* 创建显示设备 */
    display = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    if (display == NULL) {
        ESP_LOGE("LVGL_DISP", "Failed to create display");
        return;
    }

    /* 分配DMA缓冲区 */
    buf1 = heap_caps_malloc(BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (buf1 == NULL) {
        ESP_LOGE("LVGL_DISP", "Failed to allocate display buffer 1");
        return;
    }
    ESP_LOGI("LVGL_DISP", "Buffer1 allocated: %d bytes", BUF_SIZE);

    buf2 = heap_caps_malloc(BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (buf2 == NULL) {
        ESP_LOGW("LVGL_DISP", "Failed to allocate buffer 2, using single buffer mode");
    } else {
        ESP_LOGI("LVGL_DISP", "Buffer2 allocated: %d bytes, using double buffer mode", BUF_SIZE);
    }

    /* 设置显示缓冲区 */
    lv_display_set_buffers(display, buf1, buf2, BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    /* 设置刷新回调函数 */
    lv_display_set_flush_cb(display, lcd_flush_cb);
    
    /* 启用软件旋转支持 */
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_0);

    /* 使用标准RGB565格式 */
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);

    /* 设置为默认显示器 */
    lv_display_set_default(display);

    ESP_LOGI("LVGL_DISP", "Display initialized: %dx%d", MY_DISP_HOR_RES, MY_DISP_VER_RES);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief LVGL刷新回调函数
 */
static void lcd_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    int16_t x1 = area->x1;
    int16_t y1 = area->y1;
    int16_t x2 = area->x2;
    int16_t y2 = area->y2;

    /* 计算像素数量 */
    uint32_t pixel_count = (x2 - x1 + 1) * (y2 - y1 + 1);

    /* 参数有效性检查 */
    if (px_map == NULL || pixel_count == 0) {
        ESP_LOGW("LVGL_DISP", "Invalid flush parameters: px_map=%p, pixel_count=%lu", px_map, pixel_count);
        lv_display_flush_ready(disp);
        return;
    }

    /* 转换字节序：LVGL小端 → LCD大端*/
    lv_draw_sw_rgb565_swap(px_map, pixel_count);

    /* 直接传输，不进行字节交换 - 按照官方例程方式 */
#if dispUSE_ESP_LCD_FRAMEWORK
    esp_err_t e_ret = esp_lcd_panel_draw_bitmap(P_panel_handle, x1, y1, x2 + 1, y2 + 1, px_map);
    if (e_ret != ESP_OK) {
        ESP_LOGE("LVGL_DISP", "Draw bitmap failed: %d", e_ret);
        /* 传输失败时立即通知LVGL，避免卡死 */
        lv_display_flush_ready(disp);
    }
    /* 注意：ESP-LCD异步DMA传输，flush_ready在DMA完成回调中调用 */
#else
    uint32_t byte_count = pixel_count * 2;
    vDisp_LCD_Address_Set(x1, y1, x2, y2);
    eDisp_SPI_Write_Data_DMA_Batch(LCD_Handle, (const uint8_t *)px_map, byte_count);
    /* 传统DMA方式是同步的，传输完成后立即通知LVGL */
    lv_display_flush_ready(disp);
#endif
}

#endif /* boardDISPLAY */