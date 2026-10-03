# Protocol组件CMakeLists.txt
# 功能：通信协议处理组件（白库协议和Modbus协议）

idf_component_register(
    SRCS
        "Baiku/baiku_proto.c"
        "Modbus/modbus_proto.c"
    INCLUDE_DIRS
        "Baiku"
        "Modbus"
        "."
    REQUIRES
        freertos
        ComFunc
)
