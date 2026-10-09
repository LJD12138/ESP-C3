# Main组件CMakeLists.txt
# 功能：应用程序主入口和核心逻辑

# 收集所有源文件
file(GLOB_RECURSE MAIN_SRCS
    "*.c"
)

idf_component_register(
    SRCS ${MAIN_SRCS}
    INCLUDE_DIRS
        "."
        "Sys"
        ".."
    REQUIRES
        ComFunc
        Print
        Key
        MD_Display
        TaskQueue
        esp_lcd
)
