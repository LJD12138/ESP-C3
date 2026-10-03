#ifndef MD_BMS_QUEUE_TASK_H_
#define MD_BMS_QUEUE_TASK_H_

#include "board_config.h"

#if(boardBMS_IFACE)
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos/freertos.h"
#include "freertos/task.h"
#endif

bool bBms_QueueInit(void);

//¶ÓÁÐº¯Êý
void v_bms_queue_task_init(Task_T *tp_task);
void v_bms_queue_task_clt_switch(Task_T *tp_task);
void v_bms_queue_task_main(Task_T *tp_task);
void v_bms_queue_task_cali(Task_T *tp_task);
void v_bms_queue_task_err(Task_T *tp_task);
void v_bms_queue_task_get_app_info(Task_T *tp_task);
void v_bms_queue_task_req_set_cmd(Task_T *tp_task);

#endif  //boardBMS_IFACE

#endif  //MD_BMS_QUEUE_FUNC_H_
