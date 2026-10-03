# Key���CMakeLists.txt
# ���ܣ���������Ͱ������ܴ������

file(GLOB SRCS "usb_task.c" "usb_prot_frame_ic2.c" "usb_sw6303_common.c" "usb_prot_frame_ic3.c" "usb_queue_task_work.c" "usb_queue_task_init.c" "usb_prot_frame_ic1.c" "*.c")

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
