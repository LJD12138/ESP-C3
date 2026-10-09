#ifndef MD_MPPT_QUEUE_TASK_H_
#define MD_MPPT_QUEUE_TASK_H_

#include "board_config.h"

#if(boardMPPT_IFACE)
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos/freertos.h"
#include "freertos/task.h"
#endif


#define       	mpptTASK_CYCLE_TIME               		100  //任务时间


bool bMppt_QueueInit(void);

//队列函数
void v_mppt_queue_task_init(Task_T *tp_task);
void v_mppt_queue_task_main(Task_T *tp_task);
void v_mppt_queue_task_set_chg_pwr(Task_T *tp_task);
void v_mppt_queue_task_err_process(Task_T *tp_task);

#endif  //boardMPPT_IFACE

#endif  //MD_MPPT_QUEUE_TASK_H_
