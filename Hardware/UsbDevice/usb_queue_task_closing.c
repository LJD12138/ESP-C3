/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "UsbDevice/usb_queue_task.h"
#include "UsbDevice/usb_task.h"
#include "UsbDevice/usb_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "app_info.h"

#define       	usbTASK_CLOSING_CYCLE_TIME               		100


/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_usb_queue_task_closing(Task_T *tp_task)
{
	switch (tp_task->ucStep)
    {
		case 0:
		{
			bUsb_SetDevState(DS_CLOSING);

			// if(cUsb_CheckInVolt() < 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
		}
		break;

		case 1:
		{
		   if(c_usb_switch_dc_cs(false) > 0)
			{
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
				return;
			}
		}
		break;

		case 2:
		{
			bUsb_SetDevState(DS_SHUT_DOWN);
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;
			
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }
	
	//等待超时
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (5000 / usbTASK_CLOSING_CYCLE_TIME)) 
	{
		bUsb_SetErrCode(UEC_COLSE_FAULT, true);

		if(uPrint.tFlag.bUsbTask)
			log_w("bUsbTask:关闭任务等待超时,步骤%d", tp_task->ucStep);

		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	#if(boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, usbTASK_CLOSING_CYCLE_TIME);
	#endif  //boardUSE_OS
}
