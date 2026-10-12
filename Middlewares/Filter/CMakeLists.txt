# ==============================================================================
# Filter组件CMakeLists.txt
# 功能：通用数字信号滤波算法中间件（去极值均值/滑动递推/加权递推/一阶低通Q15/卡尔曼/限幅消抖）
# ==============================================================================

idf_component_register(
    SRCS
        "src/filter.c"
        "src/filter_test.c"
    INCLUDE_DIRS
        "inc"
        "../../main"
        "../../Hardware"
)
# 说明：main.h 通过 INCLUDE_DIRS("../../main") 提供,不放入 REQUIRES,
#       避免 main <-> Filter 循环依赖导致链接顺序错乱

