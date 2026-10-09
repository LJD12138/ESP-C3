# EasyFlash组件CMakeLists.txt
# 功能：轻量级嵌入式Flash存储管理中间件

# # 收集所有源文件
# file(GLOB_RECURSE EASYFLASH_SRCS
#     "src/*.c"
#     "port/*.c"
# )

# # 可选：编译类型插件
# file(GLOB EASYFLASH_TYPES_SRCS
#     "plugins/types/*.c"
# )

# # 可选：编译struct2json插件
# file(GLOB EASYFLASH_STRUCT2JSON_SRCS
#     "plugins/types/struct2json/src/*.c"
# )

# idf_component_register(
#     SRCS
#         ${EASYFLASH_SRCS}
#         ${EASYFLASH_TYPES_SRCS}
#         ${EASYFLASH_STRUCT2JSON_SRCS}
#     INCLUDE_DIRS
#         "inc"
#         "plugins/types"
#         "plugins/types/struct2json/inc"
#         "port"
# )
