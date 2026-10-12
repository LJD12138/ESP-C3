# ==============================================================================
# Key组件CMakeLists.txt
# 功能：按键任务、按键硬件接口及按键业务分发处理组件
# ==============================================================================

idf_component_register(
    SRCS
        "key_iface.c"
        "key_task.c"
        "key_func.c"
        "key_func_eng.c"
    INCLUDE_DIRS
        "."
        ".."
    PRIV_REQUIRES
        driver
        freertos
        main
        ComFunc
        Print
        MD_Display
        lvgl
        MultiFuncKey
)
