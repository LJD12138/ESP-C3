# MD_Bms���CMakeLists.txt
# ���ܣ�BMS����ͱ������ܴ������

# �Զ��ռ���ǰĿ¼�����е�.cԴ�ļ�
file(GLOB SRCS "md_bms_queue_task_cali.c" "md_bms_queue_task_ctrl_switch.c" "md_bms_queue_task_err.c" "md_bms_queue_task_get_app_info.c" "md_bms_queue_task_main.c" "md_bms_queue_task_req_set_cmd.c" "md_bms_queue_task.c" "md_bms_rec_data_proc.c" "md_bms_task.c" "md_bms_prot_frame.c" "md_bms_iface.c" "md_bms_queue_task_init.c" "md_bms_rec_task.c" "*.c")

idf_component_register(
    SRCS
        ${SRCS}
    INCLUDE_DIRS
        ".."
    PRIV_REQUIRES
        driver
        esp_driver_uart
        freertos
        main
        ComFunc
        Protocol
        Print
)
