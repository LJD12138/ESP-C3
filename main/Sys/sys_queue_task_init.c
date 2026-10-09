/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Key/Key_task.h"
// #include "Buz/buz_task.h"
#include "Flash/flash_iface.h"

#include "gpio_init.h"
#include "app_info.h"


#define     	sysTASK_INIT_CYCLE_TIME					10 //任务时间

/***********************************************************************************************************************
-----函数功能    系统初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/  
void v_sys_queue_task_init(Task_T *tp_task)
{
	s8 c_ret = 0;
	static bool b_ret = true;
	static vu8 uc_tri_init_cnt = 0;
	static vu8 uc_tri_type = 0;
	
	//记录长按时间
	if(bKey_PowerIsPress() == true)
	{
		if(uc_tri_init_cnt < 0xff)
			uc_tri_init_cnt++;
	}
	else 
		uc_tri_init_cnt = 0;
	
    switch (tp_task->ucStep)
    {
		case 0:
		{
			// vFlash_ReadWriteTest();

			// tSysInfo.uInit.tFinish.bIF_AppInfo = false;
			
			// c_ret = cApp_BootInfoInit();
			// if(c_ret > 0)
			// {
			// 	if((uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) && b_ret == false)
			// 		log_w("bSysTask:Boot记忆消息获取错误清除");
				
			// 	b_ret = true;
				cQueue_GotoStep(tp_task, STEP_NEXT);
			// }
			// else
			// {
			// 	if((uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) && b_ret == true)
			// 	{
			// 		log_w("bSysTask:Boot记忆消息初始化失败 代码%d",c_ret);
			// 		b_ret = false;
			// 	}
			// 	// vFlash_ReadWriteTest();
			// 	vTaskDelay(100);
			// 	break;
			// }
		}
		
		case 1:
		{
			c_ret = cApp_AppInfoInit();
			if(c_ret > 0)
			{
				if((uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) && b_ret == false)
					log_w("bSysTask:App记忆消息获取错误清除");
				
				b_ret = true;
				tSysInfo.uInit.tFinish.bIF_AppInfo = true;
				
				cQueue_GotoStep(tp_task, STEP_NEXT);
			}
			else
			{
				if((uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant) && b_ret == true)
				{
					log_w("bSysTask:App记忆消息初始化失败 代码%d",c_ret);
					b_ret = false;
				}
		
				vTaskDelay(100);
				break;
			}
			break;
		}
		
		case 2:
		{
			// if(tSysInfo.uInit.tFinish.bIF_AdcTask)
				cQueue_GotoStep(tp_task, STEP_NEXT);
			// else
			// {
			// 	vTaskDelay(500);
			// 	break;
			// } 
		}
		
		case 3:
		{
			// if(bKey_PowerIsPress() == true)
				cQueue_GotoStep(tp_task, STEP_NEXT);
			// else if(bSys_ExistInVolt() == true)
			// {
			// 	cQueue_GotoStep(tp_task, STEP_NEXT);
			// }
			// else if(tAdcSamp.usSysInVolt > 200 && tBms.eDevState == DS_LOST)  //10V 0.1V
			// {
			// 	//充电激活
			// 	tp_task->usStepWaitCnt++;
			// 	if(tp_task->usStepWaitCnt > (5000 / sysTASK_INIT_CYCLE_TIME))
			// 	{
			// 		cQueue_GotoStep(tp_task, STEP_NEXT);
			// 	}
			// 	else 
			// 		break;
			// }
			// else
			// {
			// 	tp_task->usStepWaitCnt = 0;
			// 	break;
			// }
		}
		
		case 4:
		{
			if(bKey_AcIsPress() == false &&  //工厂模式
				bKey_PowerIsPress() == true &&
				bKey_LightIsPress() == false)
			{
				if(uc_tri_type != 2)
				{
					uc_tri_type = 2;
					tp_task->usStepWaitCnt = 0;
				}
				
				tp_task->usStepWaitCnt++;
			}
			else if(tSysInfo.uInit.tFinish.bIF_BmsTask ||
					tSysInfo.uInit.tFinish.bIF_DispTask)
			{
				uc_tri_type = 0;
				tp_task->usStepWaitCnt = 0;
				cQueue_GotoStep(tp_task, STEP_NEXT);
			}
			else 
			{
				uc_tri_type = 0;
				tp_task->usStepWaitCnt = 0;
			}

			if(tp_task->usStepWaitCnt > (3000 / sysTASK_INIT_CYCLE_TIME))
			{
				
				if(uc_tri_type == 2)  //工厂模式
				{
					G_TestMode = true;
					cSys_Switch(SO_KEY, ST_ON, false);
				}
				#if(boardENG_MODE_EN)  //工程模式
				else if(uc_tri_type == 1)
				{
					bBuz_Tweet(SHORT_2);
					cQueue_AddQueueTask(tpSysTask, STI_ENG, NULL, false);
					cBms_Switch(SO_KEY, ST_ON, false);
					cQueue_GotoStep(tp_task, STEP_NEXT);
				}
				#endif
			}
		}break;
		
		case 5:
		{
			tSysInfo.uInit.tFinish.bIF_SysTask = 1;
			tSysInfo.uInit.tFinish.bIF_SysInit = 1;
			tSysInfo.uInit.tFinish.bIF_Gpio = 1;

			//没有任务就调度关闭任务
			if(tSysInfo.eDevState != DS_BOOTING)
				bSys_SetDevState(DS_SHUT_DOWN,false);

			cSys_Switch(SO_KEY, ST_ON, false);
			cQueue_GotoStep(tp_task, STEP_END);
		}break;
		
        default:
			cQueue_GotoStep(tp_task, STEP_END);
			break;
    }
	
	//初始化等待5S,超时退出
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (5000 / sysTASK_INIT_CYCLE_TIME))
	{
		// gpioASSIST_OPEN_OFF();
	}
	
	vTaskDelay(sysTASK_INIT_CYCLE_TIME);
}
