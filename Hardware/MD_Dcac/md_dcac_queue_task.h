#ifndef MD_DCAC_QUEUE_TASK_H_
#define MD_DCAC_QUEUE_TASK_H_

#include "board_config.h"
#if(boardDCAC_IFACE)
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos/freertos.h"
#include "freertos/task.h"
#endif

bool bDcac_QueueInit(void);

//¶ÓÁÐº¯Êý
void v_dcac_queue_task_init(Task_T *tp_task);
void v_dcac_queue_task_main(Task_T *tp_task);
void v_dcac_queue_task_ctrl_sw(Task_T *tp_task);
void v_dcac_queue_task_err_proc(Task_T *tp_task);

#endif  //boardDCAC_IFACE

#endif  //MD_DCAC_QUEUE_TASK_H_
