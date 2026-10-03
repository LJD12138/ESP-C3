/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Print/print_queue_task.h"

#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"
#include "Print/print_task.h"

#define       	printTASK_APP_INFO_CYCLE_TIME               		50

//****************************************************函数声明****************************************************//

	
/*****************************************************************************************************************
-----函数功能    任务函数:任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_print_queue_task_reply_app_info(Task_T *tp_task)
{
	TaskInParam_U u_in_param;
	u_in_param.usTaskInParam = tp_task->usInParam;
	
	switch(tp_task->ucStep)
	{
		case 0:
		{
			if(b_relay_bms_app_info(u_in_param.usTaskInParam) > 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
		}
		break;
		
		case 1:
		{
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;

		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt>(5000/printTASK_APP_INFO_CYCLE_TIME))  //等待超时
	{
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	#if(boardUSE_OS)
	vTaskDelay(printTASK_APP_INFO_CYCLE_TIME);
	#endif  //boardUSE_OS
}

#endif  //boardPRINT_IFACE

