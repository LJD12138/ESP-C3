# EasyFlash组件CMakeLists.txt
# 功能：轻量级嵌入式Flash存储管理中间件
# 说明：仅编译核心库(src)与移植层(port)，struct2json/types插件未启用不参与编译
#       ef_iap.c/ef_log.c内部由EF_USING_IAP/EF_USING_LOG宏包裹，未启用时编译为空单元

idf_component_register(
    SRCS
        "src/easyflash.c"
        "src/ef_env.c"
        "src/ef_utils.c"
        "src/ef_log.c"
        "src/ef_iap.c"
        "port/ef_port.c"
    INCLUDE_DIRS
        "inc"
        "port"
    PRIV_REQUIRES
        Print
        Flash
        main
        freertos
)
