/*****************************************************************************************************************
*                                                                                                                *
 *                                         队列函数                                                  			*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Dcac/md_dcac_queue_task.h"

#if(boardDCAC_IFACE)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "MD_Dcac/md_dcac_iface.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#define       	dcacTASK_INIT_CYCLE_TIME               		100

//****************************************************函数声明****************************************************//



/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_dcac_queue_task_init(Task_T *tp_task)
{
	static vu16 us_run_cnt = 0;

	switch (tp_task->ucStep)
    {
		case 0:
        {
			if(us_run_cnt < 0xffff)
				us_run_cnt++;

			if(us_run_cnt >= (800 / dcacTASK_INIT_CYCLE_TIME))
				dcacPOWER_EN_ON();

			if(us_run_cnt < (1000 / dcacTASK_INIT_CYCLE_TIME))
				break;

			if(b_dcac_cs_init() == true)
				cQueue_GotoStep(tp_task, STEP_NEXT);
			else
			{
				vTaskDelay(500);
				break;
			}
        }
		
		case 1:
        {
			tSysInfo.uInit.tFinish.bIF_DcacTask = 1;
			bDcac_SetAcState(OO_ALL, IOS_SHUT_DOWN);
			if(uPrint.tFlag.bDcacTask)
				sMyPrint("bDcacTask:初始化DCAC----初始化完成----\r\n");
			
			cQueue_GotoStep(tp_task, STEP_END);
        }
		break;

		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (3000 / dcacTASK_INIT_CYCLE_TIME))  //等待超时
	{
		if(uPrint.tFlag.bDcacTask)
			log_w("bDcacTask:BMS初始化任务等待超时");
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	vTaskDelay(dcacTASK_INIT_CYCLE_TIME);
}

#endif  //boardDCAC_IFACE
