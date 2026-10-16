# ==============================================================================
# WiFi组件CMakeLists.txt
# 功能：WiFi连接管理任务与网络状态维护组件
# ==============================================================================

idf_component_register(
    SRCS
        "wifi_task.c"
        "wifi_queue_task.c"
        "wifi_queue_task_init.c"
        "wifi_queue_task_work.c"
        "wifi_queue_task_connect.c"
        "wifi_queue_task_reconnect.c"
        "wifi_queue_task_disconnect.c"
        "wifi_queue_task_stop.c"
        "wifi_queue_task_scan.c"
        "wifi_queue_task_sntp.c"
        "wifi_queue_task_weather.c"
    INCLUDE_DIRS
        "."
        ".."
    PRIV_REQUIRES
        esp_wifi
        nvs_flash
        esp_event
        esp_netif
        freertos
        main
        Print
        EasyLogger
        TaskQueue
        lwip
        esp_http_client
        json
        EasyFlash
)
