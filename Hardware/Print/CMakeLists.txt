# Print组件CMakeLists.txt
# 功能：打印任务和通信协议处理组件

idf_component_register(
    SRCS
        "print_task.c"
        "print_queue_task.c"
        "print_queue_task_main.c"
        "print_api.c"
        "print_iface.c"
        "print_prot_frame.c"
    INCLUDE_DIRS
        ".."
    REQUIRES
        driver
        esp_driver_uart
        freertos
        main
        ComFunc
        TaskQueue
        Protocol
)
