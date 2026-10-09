# MD_Bms���CMakeLists.txt
# ���ܣ�BMS����ͱ������ܴ������

# �Զ��ռ���ǰĿ¼�����е�.cԴ�ļ�
file(GLOB SRCS "md_mppt_task.c" "md_mppt_queue_task_init.c" "md_mppt_queue_task_err_process.c" "md_mppt_queue_task_main.c" "md_mppt_rec_task.c" "md_mppt_rec_data_proc.c" "md_mppt_queue_task_set_chg_pwr.c" "md_mppt_prot_frame.c" "*.c")

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
