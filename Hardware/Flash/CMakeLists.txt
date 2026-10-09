# Key组件CMakeLists.txt
# 功能：按键任务和按键功能处理组件

idf_component_register(
    SRCS
        "flash_iface.c"
        "flash_esp32.c"
    INCLUDE_DIRS
        ".."
    PRIV_REQUIRES
        driver
        freertos
        Print
    REQUIRES esp_partition
    PRIV_REQUIRES spi_flash
)
