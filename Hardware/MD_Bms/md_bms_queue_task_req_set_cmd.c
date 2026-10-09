/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Bms/md_bms_queue_task.h"

#if(boardBMS_IFACE)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Print/print_task.h"
// #include "Updata/updata_task.h"

#define       	bmsTASK_SET_CMD_CYCLE_TIME               		50

//****************************************************函数声明****************************************************//

/*****************************************************************************************************************
-----函数功能    任务函数:请求升级任务
-----说明(备注)  none
-----传入参数    BMS_ErrState_N:
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_bms_queue_task_req_set_cmd(Task_T *tp_task)
{
	TaskInParam_U u_in_param;
	u_in_param.usTaskInParam = tp_task->usInParam;
	
	switch(tp_task->ucStep)
	{
		case 0:
		{
			if(c_bms_cs_req_updata(u_in_param.tTaskParam.ucParam) > 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
		}
		break;
		
		case 1:
		{
			vTaskDelay(1000);
			if(c_bms_cs_req_updata(u_in_param.tTaskParam.ucParam) > 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
		}
		break;

		case 2:
		{
			bQueue_Reset(tp_task);
			cBaiku_ResetRxBuff(tpBmsProtoRx);
			// cUpdata_ChSelect(CT_PRINT, UWS_CHECK, PT_BAIKU);
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;
			
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (5000 / bmsTASK_SET_CMD_CYCLE_TIME))  //等待超时
	{
		cQueue_GotoStep(tp_task, STEP_END);  //结束
		if(uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
			log_w("bBmsTask:设置指令任务超时,退出任务");
	}
	
	#if(boardUSE_OS)
	vTaskDelay(bmsTASK_SET_CMD_CYCLE_TIME);
	#endif  //boardUSE_OS
}

#endif  //boardBMS_IFACE
