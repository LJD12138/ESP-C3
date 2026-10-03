# Key组件CMakeLists.txt
# 功能：按键任务和按键功能处理组件
file(GLOB SRCS "*.c")
set(SRCS ${SRCS})

idf_component_register(
    SRCS
        ${SRCS}
    INCLUDE_DIRS
        ".."
    PRIV_REQUIRES
        driver
        freertos
        main
        ComFunc
        Print
)
