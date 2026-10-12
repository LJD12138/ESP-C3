# MD_Display CMakeLists.txt - ST7789 + FT6336 + UniDisplay 框架

# 明确列出显示组件源文件
set(SRCS
    "md_display_task.c"
    "md_display_api.c"
    "md_display_iface.c"
    "lv_port_disp.c"
    "lv_port_indev.c"
    "md_display_data.c"
    "md_display_err.c"
    "md_display_page_init.c"
    "md_display_page_boot.c"
    "md_display_page_work.c"
    "md_display_page_closing.c"
    "md_display_page_shut_down.c"
    "md_display_page_fault.c"
    "md_display_page_update.c"
    "md_display_page_eng.c"
)

# 包含 eez_ui 目录下的所有源文件
file(GLOB EEZ_UI_SRCS "eez_ui/*.c")

set(SRCS ${SRCS} ${EEZ_UI_SRCS})

idf_component_register(
    SRCS
        ${SRCS}
    INCLUDE_DIRS
        "."
        ".."
        "eez_ui"
    REQUIRES
        driver
        esp_lcd
        freertos
        main
        Print
        lvgl
        UniDisplay
        espressif__esp_lcd_touch
        lambage__esp_lcd_touch_ft6336u
)
