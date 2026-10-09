# MD_Display CMakeLists.txt - ST7789 + FT6336 版本

# 明确列出所有源文件
set(SRCS
    "lvgl_ui.c"
    "md_display_task.c"
    "md_display_api.c"
    "md_display_iface.c"
    "lv_port_disp.c"
    "lv_port_indev.c"
)
# 包含 ui 目录下的所有源文件
file(GLOB UI_SRCS "ui/*.c")

set(SRCS ${SRCS} ${UI_SRCS})

idf_component_register(
    SRCS
        ${SRCS}
    INCLUDE_DIRS
        ".."
        "ui"
    REQUIRES
        driver
        esp_lcd
        freertos
        main
        Print
        lvgl
        espressif__esp_lcd_touch
        lambage__esp_lcd_touch_ft6336u
)
