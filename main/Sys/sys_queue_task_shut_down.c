/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Key/key_task.h"
// #include "MD_Bms/md_bms_task.h"


#include "gpio_init.h"

#define     	sysTASK_SHUT_DOWN_CYCLE_TIME					sysTASK_CYCLE_TIME //任务时间

/***********************************************************************************************************************
-----函数功能    系统初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/  
void v_sys_queue_task_shut_down(Task_T *tp_task)
{
    if(tSysInfo.eDevState!= DS_SHUT_DOWN)
		bSys_SetDevState(DS_SHUT_DOWN, false);

	//充电唤醒原由MPPT模块触发(SO_MPPT),MPPT模块已移除,关机状态下不再自动唤醒
	
    switch (tp_task->ucStep)
    {
		//有按键按下
		case 0:
		{
			if(bKey_PowerIsPress() == true)
				tp_task->usStepWaitCnt = 0;

			//有任务退出
			if(lwrb_get_full(&tp_task->tQueueBuff))                 //队列里面有任务
				cQueue_GotoStep( tp_task, STEP_END );  //结束
			
			//等待主机请求关闭
			tp_task->usStepWaitCnt++;
			if(tp_task->usStepWaitCnt >= (6000 / sysTASK_SHUT_DOWN_CYCLE_TIME))
			{
				tp_task->usStepWaitCnt = 0;
				cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
				
				if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) 
					sMyPrint("bSysTask:等待主机关闭超时,再次关闭BMS关闭\r\n");
			}
		}
		break;
		
		case 1:
		{
			// if(cBms_Switch(SO_KEY, ST_OFF, false) < 0)  //操作失败
			// {
			// 	if(uPrint.tFlag.bSysTask) 
			// 		sMyPrint("bSysTask:关闭BMS失败\r\n");
				
			// 	vTaskDelay(500);
			// }
			// else 
			// 	cQueue_GotoStep( tp_task, STEP_FORWARD );  //上一步
		}
		break;
		
        default:
				cQueue_GotoStep( tp_task, STEP_END );  //结束
			break;
    }
	
	vTaskDelay(sysTASK_SHUT_DOWN_CYCLE_TIME);
}

