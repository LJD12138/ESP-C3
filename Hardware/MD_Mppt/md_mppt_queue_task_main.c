/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Mppt/md_mppt_queue_task.h"

#if(boardMPPT_IFACE)
#include "MD_Mppt/md_mppt_task.h"
#include "MD_Mppt/md_mppt_prot_frame.h"
#include "MD_Mppt/md_mppt_rec_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "app_info.h"


#define       	mpptTASK_GET_PARAM_CYCLE_TIME               		1000


//****************************************************函数声明****************************************************//
static void v_check_chg_perm(void);
static void v_proc_rec_param(void);
static void v_set_total_chg_pwr(void);


/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_mppt_queue_task_main(Task_T *tp_task)
{
	s8 result = 0;
	
	//*****************************************非工作状态下,检查电池包是否开启中**************************************
	if((tSysInfo.uPerm.tPerm.bChgPerm == false || tMppt.bChgPerm == false) &&
		tMpptRx.usInPwr != 0)
	{
		cMppt_SetChgPwr(0);
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
			//主机
			result = c_mppt_cs_get_param(2);
			//发送成功
			if(result > 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);
        }break;
		
		case 1:
        {
			v_proc_rec_param();
			
			v_check_chg_perm();
			
			v_set_total_chg_pwr();
			
			cQueue_GotoStep(tp_task, 0);  //结束
        }break;
			
		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }
	
	//队列里面有任务
	if(lwrb_get_full(&tp_task->tQueueBuff))  
	{
		cQueue_GotoStep( tp_task, STEP_END );  //结束
		return;
	}
	
	#if(boardUSE_OS)
	vTaskDelay(mpptTASK_GET_PARAM_CYCLE_TIME);
	#endif  //boardUSE_OS
}

/*****************************************************************************************************************
-----函数功能    获取参数处理
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE void v_proc_rec_param(void)
{
	// //输入过流
	// if(tMpptRx.uErrCode.tCode.bOC)
	// {
	// 	if(tMppt.uErrCode.tCode.bMpptInOC == false)
	// 		bMppt_SetErrCode(MEC_MPPT_IN_OC, true);
	// }
	// else 
	// {
	// 	if(tMppt.uErrCode.tCode.bMpptInOC == true)
	// 		bMppt_SetErrCode(MEC_MPPT_IN_OC, false);
	// }

	//输入过压
	if(tMpptRx.uErrCode.tCode.bIOV)
	{
		if(tMppt.uErrCode.tCode.bMpptInOV == false)
			bMppt_SetErrCode(MEC_MPPT_IN_OV, true);
	}
	else 
	{
		if(tMppt.uErrCode.tCode.bMpptInOV == true)
			bMppt_SetErrCode(MEC_MPPT_IN_OV, false);
	}

	// //输入错误
	// if(tMpptRx.uErrCode.tCode.bIUV)
	// {
	// 	if(tMppt.uErrCode.tCode.bMpptInErr == false)
	// 		bMppt_SetErrCode(MEC_MPPT_IN_ERR, true);
	// }
	// else 
	// {
	// 	if(tMppt.uErrCode.tCode.bMpptInErr == true)
	// 		bMppt_SetErrCode(MEC_MPPT_IN_ERR, false);
	// }

	//输出过压
	if(tMpptRx.uErrCode.tCode.bOOV)
	{
		if(tMppt.uErrCode.tCode.bMpptOutOV == false)
			bMppt_SetErrCode(MEC_MPPT_OUT_OV, true);
	}
	else 
	{
		if(tMppt.uErrCode.tCode.bMpptOutOV == true)
			bMppt_SetErrCode(MEC_MPPT_OUT_OV, false);
	}

	// 输出错误
	if(tMpptRx.uErrCode.tCode.bOUV)
	{
		if(tMppt.uErrCode.tCode.bMpptOutErr == false)
			bMppt_SetErrCode(MEC_MPPT_OUT_ERR, true);
	}
	else 
	{
		if(tMppt.uErrCode.tCode.bMpptOutErr == true)
			bMppt_SetErrCode(MEC_MPPT_OUT_ERR, false);
	}

	//过温
	if(tMpptRx.uErrCode.tCode.bOT)
	{
		if(tMppt.uErrCode.tCode.bMpptOT == false)
			bMppt_SetErrCode(MEC_MPPT_IN_OV, true);
	}
	else 
	{
		if(tMppt.uErrCode.tCode.bMpptOT == true)
			bMppt_SetErrCode(MEC_MPPT_IN_OV, false);
	}

	//过载
	if(tMpptRx.uErrCode.tCode.bOL)
	{
		if(tMppt.uErrCode.tCode.bMpptOL == false)
			bMppt_SetErrCode(MEC_MPPT_OL, true);
	}
	else 
	{
		if(tMppt.uErrCode.tCode.bMpptOL == true)
			bMppt_SetErrCode(MEC_MPPT_OL, false);
	}

	tMppt.usInPwr = tMpptRx.usInPwr / 10;
}

/*****************************************************************************************************************
-----函数功能    检查充放电许可
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE void v_check_chg_perm(void)
{
	if(tMppt.uErrCode.tCode.bMpptInOV		== 1	||
		tMppt.uErrCode.tCode.bMpptInErr		== 1	||
		tMppt.uErrCode.tCode.bMpptInOC		== 1	||
		tMppt.uErrCode.tCode.bMpptOutOV		== 1	||
		tMppt.uErrCode.tCode.bMpptOutOC		== 1	||
		tMppt.uErrCode.tCode.bMpptOutErr	== 1	||
		tMppt.uErrCode.tCode.bMpptOT		== 1	||
		tMppt.uErrCode.tCode.bMpptOL		== 1	||
		tMppt.uErrCode.tCode.bSysOverLoad	== 1	||
		tSysInfo.uPerm.tPerm.bChgPerm == false)
	{
		if(tMppt.bChgPerm == true)
		{
			bMppt_SetChgPerm(false);
		}
	}
	else 
	{
		if(tMppt.bChgPerm == false)
		{
			bMppt_SetChgPerm(true);
		}
	}
}

/*****************************************************************************************************************
-----函数功能    充电控制
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE void v_set_total_chg_pwr(void)
{
	vu16 us_chg_pwr = 0;
	static vu16 us_last_chg_pwr = 100;
	static vu16 us_chg_pwr_err = 0;
	
	if(tSysInfo.eDevState != DS_WORK)
		return;
	
	if(tMppt.bChgPerm == false)
		tSysInfo.tSetChgPwr.usMPPT = 0;
	
	us_chg_pwr = tSysInfo.tSetChgPwr.usMPPT;
	
	if(abs(tMpptRx.usInPwr - us_chg_pwr) > 100)
		us_chg_pwr_err++;
	else 
		us_chg_pwr_err = 0;
	
	if(us_last_chg_pwr == us_chg_pwr && us_chg_pwr_err < (5000 / mpptTASK_GET_PARAM_CYCLE_TIME))
		return;

	if(c_mppt_cs_set_pwr(us_chg_pwr) == true)
	{
		us_last_chg_pwr = us_chg_pwr;
		us_chg_pwr_err = 0;
	}
}

#endif  //boardMPPT_IFACE
