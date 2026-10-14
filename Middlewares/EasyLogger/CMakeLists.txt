# EasyLogger组件CMakeLists.txt
# 功能：超轻量级、高性能的C/C++日志库
# 说明：仅编译核心库(src)与移植层(port)，Flash/File插件未启用不参与编译

idf_component_register(
    SRCS
        "src/elog.c"
        "src/elog_Init.c"
        "src/elog_utils.c"
        "src/elog_async.c"
        "src/elog_buf.c"
        "port/elog_port.c"
    INCLUDE_DIRS
        "inc"
        "port"
    PRIV_REQUIRES
        Print
        main
        freertos
)
