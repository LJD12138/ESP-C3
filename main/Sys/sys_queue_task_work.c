/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#if(boardBMS_IFACE)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_rec_task.h"
#endif  //boardBMS_IFACE

// #include "app_info.h"
#include "gpio_init.h"

#if(boardDISPLAY)
#include "MD_Display/md_display_task.h"
#include "MD_Display/ui/vars.h"
#endif

#define     	sysTASK_WORK_CYCLE_TIME					100 //任务时间


//****************************************************参数初始化**************************************************//
Perm_U uPerm;

//****************************************************函数声明****************************************************//
static void v_updata_param(void);

/***********************************************************************************************************************
-----函数功能    工作
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void v_sys_queue_task_work(Task_T *tp_task)
{
	if(tSysInfo.eDevState != DS_WORK)
		bSys_SetDevState(DS_WORK, false);
	
	//检查系统活跃状态
	if(bSys_CheckActState() == true)   
	{
		// bSys_SetAutoOffTime(tAppMemParam.tSYS.usAutoOffTime);
	}
	
	//队列里面有任务
	if(lwrb_get_full(&tp_task->tQueueBuff))  
	{
		cQueue_GotoStep( tp_task, STEP_END );  //结束
		return;
	}
	
    switch (tp_task->ucStep)
    {
		case 0:
		{
			cQueue_GotoStep( tp_task, STEP_NEXT );  //下一步
			vTaskDelay(2000);
		}
		break;
		
		case 1:
		{
			v_updata_param();
		}
		break;
		
        default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }
	
	vTaskDelay(sysTASK_WORK_CYCLE_TIME);
}

__STATIC_INLINE void v_updata_param(void)
{

}
