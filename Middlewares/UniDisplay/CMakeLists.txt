# UniDisplay组件CMakeLists.txt
# 功能：通用跨平台显示核心框架 (页面注册/状态路由/弹窗栈/背光息屏/轮询引擎)

idf_component_register(
    SRCS
        "uni_disp_core.c"
        "uni_disp_event.c"
        "uni_disp_port.c"
    INCLUDE_DIRS
        "."
        "../../main"
        "../../Hardware"
    REQUIRES
        freertos
        main
)
