# Key���CMakeLists.txt
# ���ܣ���������Ͱ������ܴ������
file(GLOB SRCS "md_light_iface.c" "*.c")
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
