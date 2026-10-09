#ifndef MD_MPPT_QUEUE_TASK_H_
#define MD_MPPT_QUEUE_TASK_H_

#include "main.h"
#include "board_config.h"
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos/freertos.h"
#include "freertos/task.h"
#endif


#define       	usbTASK_CYCLE_TIME               		100  //任务时间


extern s32 us_usb_total_out_pwr;
extern vu16 s_total_temp;

bool bUsb_QueueInit(void);

//队列函数
void v_usb_queue_task_init(Task_T *tp_task);
void v_usb_queue_task_closing(Task_T *tp_task);
void v_usb_queue_task_booting(Task_T *tp_task);
void v_usb_queue_task_err(Task_T *tp_task);
void v_usb_queue_task_work(Task_T *tp_task);


#endif  //MD_MPPT_QUEUE_TASK_H_
