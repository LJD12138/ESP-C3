# EasyLogger组件CMakeLists.txt
# 功能：超轻量级、高性能的C/C++日志库

# # 收集核心源文件
# file(GLOB_RECURSE EASYLOGGER_SRCS
#     "src/*.c"
#     "port/*.c"
# )

# # 可选：编译Flash插件
# file(GLOB EASYLOGGER_FLASH_SRCS
#     "plugins/flash/*.c"
# )

# # 可选：编译File插件
# file(GLOB EASYLOGGER_FILE_SRCS
#     "plugins/file/*.c"
# )

# idf_component_register(
#     SRCS
#         ${EASYLOGGER_SRCS}
#         ${EASYLOGGER_FLASH_SRCS}
#         ${EASYLOGGER_FILE_SRCS}
#     INCLUDE_DIRS
#         "inc"
#         "plugins/flash"
#         "plugins/file"
#         "port"
# )
