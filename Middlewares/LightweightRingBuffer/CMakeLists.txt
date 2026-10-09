# LightweightRingBuffer组件CMakeLists.txt
# 功能：轻量级环形缓冲区中间件

idf_component_register(
    SRCS
        "lwrb.c"
    INCLUDE_DIRS
        "."
)
