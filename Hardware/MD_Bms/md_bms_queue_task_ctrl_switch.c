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
#include "MD_Light/md_light_task.h"
#include "MD_HeatManage/md_hm_task.h"

#define       	bmsTASK_CTRL_SWITCH_CYCLE_TIME               		50

/*****************************************************************************************************************
-----函数功能    任务函数:控制开关任务
-----说明(备注)  none
-----传入参数    BMS_ErrState_N:
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_bms_queue_task_clt_switch(Task_T *tp_task)
{
	s8 c_ret = 0;
	TaskInParam_U u_param;
	SwitchType_E e_sw_type;
	
	u_param.usTaskInParam = tp_task->usInParam;       //要设置BMS的状态

	if(u_param.tTaskParam.ucObj == 0)
		e_sw_type = (SwitchType_E)u_param.tTaskParam.ucParam;
	
	switch(tp_task->ucStep)
	{
		//------------------------------发送指令------------------------------------//
		case 0:
		{
			tp_task->usStepRepeatCnt++;
			if(tp_task->usStepRepeatCnt > 3)
			{
				if(uPrint.tFlag.bBmsTask)
					log_w("bBmsTask:对象%d数据发送失败次数过多,退出开关任务", u_param.tTaskParam.ucObj);
				
				goto loop_end;
			}
			
			switch(u_param.tTaskParam.ucObj)
			{
				//系统
				case 0:
				{
					if(e_sw_type == ST_ON)
						bBms_SetDevState(DS_BOOTING);
					else 
					{
						bBms_SetErrCode(BEC_CLEAR_ALL,false);  //清除所有错误
						bBms_SetDevState(DS_CLOSING);
					}
					c_ret = c_bms_cs_switch(u_param);
				}
				break;

				//照明
				case 2:
				//热管理
				case 9:
				{
					c_ret = c_bms_cs_switch(u_param);
				}
				break;
				
				default:
					if(uPrint.tFlag.bBmsTask && uPrint.tFlag.bImportant)
						log_w("bBmsTask:开关对象%d未定义",u_param.tTaskParam.ucObj);
					cQueue_GotoStep(tp_task, STEP_END);  //结束
				break;
			}

			if(c_ret > 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
			else
				break;
		}
		
		//------------------------------处理返回数据------------------------------------//
		case 1:
		{
			TaskInParam_U u_reply_param;

			//校验数据
			u8 len = lwrb_get_full(&tBmsReplyBuff);
			if(len != sizeof(u_reply_param) || tBmsReplyBuff.buff == NULL)
			{
				if(uPrint.tFlag.bBmsTask && uPrint.tFlag.bImportant)
					log_w("bBmsTask:BMS返回数据错误,长度%d", len);
				cQueue_GotoStep(tp_task, STEP_FORWARD);
				break;
			}
			
			//读取数据
			lwrb_read(&tBmsReplyBuff, (u8*)&u_reply_param, sizeof(u_reply_param));
			
			//校验数据
			if(u_param.tTaskParam.ucObj != u_reply_param.tTaskParam.ucObj)
			{
				if(uPrint.tFlag.bBmsTask && uPrint.tFlag.bImportant)
					log_w("bBmsTask:BMS返回对象%d错误,当前操作对象%d", 
						u_reply_param.tTaskParam.ucObj, u_param.tTaskParam.ucObj);
				cQueue_GotoStep(tp_task, STEP_FORWARD);
				break;
			}

			//处理数据
			switch(u_param.tTaskParam.ucObj)
			{
				//系统
				case 0:
				{
					if(u_reply_param.tTaskParam.ucParam > DS_UPDATA_MODE)
					{
						if(uPrint.tFlag.bBmsTask && uPrint.tFlag.bImportant)
							log_w("bBmsTask:BMS返回对象%d的参数%d超范围", 
								u_reply_param.tTaskParam.ucObj, u_reply_param.tTaskParam.ucParam);
						cQueue_GotoStep(tp_task, STEP_FORWARD);
						break;
					}

					if(e_sw_type == ST_ON)
					{
						if(u_reply_param.tTaskParam.ucParam >= DS_BOOTING)
						{
							bBms_SetDevState(DS_WORK);
							cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
							break;
						}
					}
					else 
					{
						if(u_reply_param.tTaskParam.ucParam <= DS_SHUT_DOWN)
						{
							bBms_SetDevState(DS_SHUT_DOWN);
							cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
							break;
						}	
					}

					cQueue_GotoStep(tp_task, STEP_FORWARD);
				}
				break;

				//照明
				case 2:
				//热管理
				case 9:
				{
					if((u_reply_param.tTaskParam.ucParam > LWM_TWINKLE && u_param.tTaskParam.ucObj == 2) ||
						(u_reply_param.tTaskParam.ucParam > FWM_GEAR_FULL && u_param.tTaskParam.ucObj == 9))
					{
						if(uPrint.tFlag.bBmsTask && uPrint.tFlag.bImportant)
							log_w("bBmsTask:BMS返回对象%d的参数%d超范围", 
								u_reply_param.tTaskParam.ucObj, u_reply_param.tTaskParam.ucParam);
						cQueue_GotoStep(tp_task, STEP_FORWARD);
						break;
					}

					if(u_param.tTaskParam.ucParam != u_reply_param.tTaskParam.ucParam)
					{
						if(uPrint.tFlag.bBmsTask && uPrint.tFlag.bImportant)
							log_w("bBmsTask:BMS返回对象%d的参数%d错误", 
								u_reply_param.tTaskParam.ucObj, u_reply_param.tTaskParam.ucParam);
						cQueue_GotoStep(tp_task, STEP_FORWARD);
						break;
					}

					cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
				}
				break;

				default:
						if(uPrint.tFlag.bBmsTask && uPrint.tFlag.bImportant)
							log_w("bBmsTask:BMS返回对象%d未定义,当前对象%d", 
								u_reply_param.tTaskParam.ucObj, u_param.tTaskParam.ucObj);
						cQueue_GotoStep(tp_task, STEP_FORWARD);
						break;
			}
		}
		break;
		
		//-------------------------------------完成---------------------------------------//
		case 2:
		{
			if(uPrint.tFlag.bBmsTask)
				sMyPrint("bBmsTask:对象%d----开关完成----\r\n",u_param.tTaskParam.ucObj);
			
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
        break;
			
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
	}
	
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt>(3000/bmsTASK_CTRL_SWITCH_CYCLE_TIME))  //等待超时
	{
		if(uPrint.tFlag.bBmsTask)
			sMyPrint("bBmsTask:对象%d开关任务等待超时退出,步骤%d", tp_task->ucStep);
		
		loop_end:
		if(u_param.tTaskParam.ucParam != 0)
		{
			u_param.tTaskParam.ucParam = 0;
			c_bms_cs_switch(u_param);

			if(u_param.tTaskParam.ucObj == 0)
				bBms_SetDevState(DS_SHUT_DOWN);
			else if(u_param.tTaskParam.ucObj == 2)
				tLight.eWordMode = LWM_OFF;
		}
		
		cQueue_GotoStep(tp_task, STEP_END);  //结束
	}
	
	#if(boardUSE_OS)
	vTaskDelay(bmsTASK_CTRL_SWITCH_CYCLE_TIME);
	#endif  //boardUSE_OS
}

#endif  //boardBMS_IFACE
