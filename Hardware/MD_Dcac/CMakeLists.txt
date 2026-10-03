# MD_Bms���CMakeLists.txt
# ���ܣ�BMS����ͱ������ܴ������

# �Զ��ռ���ǰĿ¼�����е�.cԴ�ļ�
file(GLOB SRCS "md_dcac_queue_task_err_proc.c" "md_dcac_rec_data_proc.c" "md_dcac_task.c" "md_dcac_rec_task.c" "md_dcac_queue_task_ctrl_sw.c" "md_dcac_queue_task_init.c" "md_dcac_queue_task.c" "md_dcac_prot_frame.c" "md_dcac_iface.c" "md_dcac_queue_task_main.c" "*.c")

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
