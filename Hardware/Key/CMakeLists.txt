# Key组件CMakeLists.txt
# 功能：按键任务和按键功能处理组件

idf_component_register(
    SRCS
        "key_task.c"
        "key_func.c"
    INCLUDE_DIRS
        ".."
    PRIV_REQUIRES
        driver
        freertos
        main
        ComFunc
        Print
        MD_Display
        lvgl
)
