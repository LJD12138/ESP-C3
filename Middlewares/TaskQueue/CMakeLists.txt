# TaskQueue组件CMakeLists.txt
# 功能：队列任务管理组件（Task_T 调度 + 遥测旁挂）

idf_component_register(
    SRCS
        "queue_task.c"
        "queue_telemetry.c"
    INCLUDE_DIRS
        "."
        "../../ComFunc"
        "../LightweightRingBuffer"
        "../../main"
    REQUIRES
        freertos
        LightweightRingBuffer
)
# 说明：main.h 通过 INCLUDE_DIRS("../../main") 提供,不放入 REQUIRES,
#       避免 main <-> TaskQueue 循环依赖导致链接顺序错乱
