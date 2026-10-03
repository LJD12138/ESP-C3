# ComFunc组件CMakeLists.txt
# 功能：通用功能组件（I2C、校验、过滤、函数、队列任务）

idf_component_register(
    SRCS
        "ntc.c"
        "i2c2.c"
        "check.c"
        "filtration.c"
        "function.c"
        "queue_task.c"
    INCLUDE_DIRS
        "."
        "../Middlewares/LightweightRingBuffer"
        "../main"
    PRIV_REQUIRES
        freertos
        LightweightRingBuffer
        driver
        main
)
