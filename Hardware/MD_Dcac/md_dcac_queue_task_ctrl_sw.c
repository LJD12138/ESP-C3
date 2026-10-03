/*****************************************************************************************************************
*                                                                                                                *
 *                                         队列函数                                                  			*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Dcac/md_dcac_queue_task.h"

#if(boardDCAC_IFACE)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#define       	dcacTASK_CTRL_SW_CYCLE_TIME               	50

//****************************************************函数声明****************************************************//



/*****************************************************************************************************************
-----函数功能    任务函数:控制开关
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_dcac_queue_task_ctrl_sw(Task_T *tp_task)
{
	TaskInParam_U  u_param;
	TaskInParam_U  u_reply_param;
	u_param.usTaskInParam = tp_task->usInParam;

	SwitchType_E type = (SwitchType_E)u_param.tTaskParam.ucParam;
	switch(tp_task->ucStep)
	{
		case 0:
		{
			tp_task->usStepRepeatCnt++;
			if(tp_task->usStepRepeatCnt > 3)
			{
				if(uPrint.tFlag.bDcacTask)
					log_w("bDcacTask:对象%d数据发送失败次数过多,退出开关任务", u_param.tTaskParam.ucObj);
				
				goto loop_end;
			}

			switch(u_param.tTaskParam.ucObj)
			{
				case 3:
				{
					if(type == ST_ON)
						bDcac_SetAcState(OO_DISCHG, IOS_STARTING);   //开启
					else 
						bDcac_SetAcState(OO_DISCHG, IOS_CLOSING);    //关闭	
				}
				break;
				
				default:
						break;
			}

			u_reply_param = u_param;
			if(c_dcac_cs_switch(u_reply_param) > 0)
			{
				//校验数据
				u8 len = lwrb_get_full(&tDcacReplyBuff);
				if(len != sizeof(u_reply_param) || tDcacReplyBuff.buff == NULL)
				{
					if(uPrint.tFlag.bDcacTask && uPrint.tFlag.bImportant)
						log_w("bDcacTask:返回数据错误,长度%d", len);
					break;
				}
				
				//读取数据
				lwrb_read(&tDcacReplyBuff, (u8*)&u_reply_param, sizeof(u_reply_param));

				cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
			}
			else
				break;
		}
		
		case 1:
		{
			//校验数据
			if(u_param.tTaskParam.ucObj != u_reply_param.tTaskParam.ucObj)
			{
				if(uPrint.tFlag.bDcacTask && uPrint.tFlag.bImportant)
					log_w("bDcacTask:返回对象%d错误,当前操作对象%d", 
						u_reply_param.tTaskParam.ucObj, u_param.tTaskParam.ucObj);
				cQueue_GotoStep(tp_task, STEP_FORWARD);
				break;
			}

			//处理数据
			switch(u_param.tTaskParam.ucObj)
			{
				case 3:
				{
					if(type == ST_ON)
					{
						if(u_reply_param.tTaskParam.ucParam >= IOS_STARTING)
						{
							bDcac_SetAcState(OO_DISCHG, IOS_WORK);   //开启
							cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
							return;
						}
						else
						{
							if(uPrint.tFlag.bDcacTask && uPrint.tFlag.bImportant)
								log_w("bDcacTask:开启返回参数%d错误", u_reply_param.tTaskParam.ucParam);
						}
					}
					else 
					{
						if(u_reply_param.tTaskParam.ucParam <= IOS_SHUT_DOWN)
						{
							bDcac_SetAcState(OO_DISCHG, IOS_SHUT_DOWN);    //关闭
							cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
							return;
						}
						else
						{
							if(uPrint.tFlag.bDcacTask && uPrint.tFlag.bImportant)
								log_w("bDcacTask:关闭返回参数%d错误", u_reply_param.tTaskParam.ucParam);
						}				
					}
				}

				default:
				if(uPrint.tFlag.bDcacTask && uPrint.tFlag.bImportant)
					log_w("bDcacTask:当前操作对象错误%d", u_param.tTaskParam.ucObj);
				break;
			}

			cQueue_GotoStep(tp_task, STEP_FORWARD);
		}
		break;

		case 2:
		{
			if(uPrint.tFlag.bDcacTask)
                sMyPrint("bDcacTask:对象%d----开关完成----\r\n",u_param.tTaskParam.ucObj);
			
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}break;
			
		default:
		    cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (3000 / dcacTASK_CTRL_SW_CYCLE_TIME))  //等待超时
	{
		if(uPrint.tFlag.bDcacTask)
			log_w("bDcacTask:控制逆变输出超时退出,步骤%d", tp_task->ucStep);
		
		loop_end:
		if(type == ST_ON)
		{
			if(u_param.tTaskParam.ucObj == 3)
			{
				u_reply_param.tTaskParam.ucObj = u_param.tTaskParam.ucObj;
				u_reply_param.tTaskParam.ucParam = ST_OFF;
				c_dcac_cs_switch(u_reply_param);
				bDcac_SetAcState(OO_DISCHG, IOS_SHUT_DOWN);
			}
		}
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	vTaskDelay(dcacTASK_CTRL_SW_CYCLE_TIME);
}

#endif  //boardDCAC_IFACE
