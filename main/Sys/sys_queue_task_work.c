/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application\Sys
 * File    : sys_queue_task_work.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统正常工作状态队列任务实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_EN

#if (boardADC_EN)
#include "Adc/adc_task.h"
#endif  //boardADC_EN

#include "app_info.h"
#include "gpio_init.h"

//****************************************************Macros********************************************************************//
#define			sysTASK_WORK_CYCLE_TIME					10		//任务时间

//****************************************************Function Declaration******************************************************//


/***********************************************************************************************************************
 * 函数功能    : 系统工作状态队列任务执行函数
 * 说明(备注)  : 定周期管理充放电许可与各通道充电功率配比
 * 传入参数    : p_task: 队列任务指针
 * 输出参数    : p_task: 更新任务状态
 * 返回值      : void
 ************************************************************************************************************************/
void v_sys_queue_task_work(Task_T *p_task)
{
	if (tSysInfo.eDevState != DS_WORK)
		bSys_SetDevState(DS_WORK, false);

	if (tSysInfo.uErrCode.tCode.bBootFault)
		bSys_SetErrCode(SEC_BOOT_FAULT, false);

	//检查系统活跃状态
	if (bSys_CheckActState() == true)
		bSys_SetAutoOffTime(tAppMemParam.tSYS.usAutoOffTime);

	//队列里面有任务
	if (lwrb_get_full(&p_task->tQueueBuff))
	{
		cQueue_GotoStep(p_task, STEP_END);  //结束
		return;
	}

	switch (p_task->ucStep)
	{
		case 0:
		{
			cQueue_GotoStep(p_task, STEP_NEXT);  //下一步
		}
		break;

		case 1:
		{
			
		}
		break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);  //结束
		}
		break;
	}

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, sysTASK_WORK_CYCLE_TIME);  /* 周期节拍；新任务投递立即唤醒抢占 */
	#endif  //boardUSE_OS
}
