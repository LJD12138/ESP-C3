/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统总任务                                                           *
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task.h"
#include "Print/print_task.h"
// #include "Adc/adc_task.h"
// #include "Buz/buz_task.h"
#include "Key/key_task.h"
// #include "Updata/updata_task.h"
#include "MD_Display/md_display_task.h"
// #include "flash_allot_table.h"

#include "gpio_init.h"
#include "timer_task.h"
#include "app_info.h"
#include "function.h"

#if(boardBMS_IFACE)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_rec_task.h"
#endif  //boardBMS_IFACE

#if(boardENG_MODE_EN)
#include "Sys/sys_queue_task_eng.h"
#endif


//****************************************************任务初始化**************************************************//
#if(boardUSE_OS)
#define     	SYS_TASK_PRIO                  			3     //任务优先级 
#define      	SYS_TASK_STK_SIZE              			4096   //任务堆栈  实际字节数 *4
TaskHandle_t  	tSysTaskHandler = NULL; 
void         	vSys_Task(void *pvParameters);
#endif  //boardUSE_OS


//****************************************************参数初始化**************************************************// 
SysInfo_T tSysInfo; 
static Task_T *tp_task = NULL;
bool G_TestMode = false;

//****************************************************函数声明****************************************************//
static bool b_task_param_init(void);
static void v_sys_check_prote(void);
static void v_sys_get_perm(void);


/*****************************************************************************************************************
-----函数功能    系统任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool bSys_TaskInit(void)
{
	if(bSys_QueueInit() == false)
		return false;
	
	if(b_task_param_init() == false)
		return false;
	
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vSys_Task,				//任务函数
                (const char* )"bSysTask",				//任务名称
                (uint16_t ) SYS_TASK_STK_SIZE,          //任务堆栈大小
                (void* )NULL,                           //传递给任务函数的参数
                (UBaseType_t ) SYS_TASK_PRIO,           //任务优先级
                (TaskHandle_t*)&tSysTaskHandler);      	//任务句柄
	#endif  //boardUSE_OS

	return true;
}


/***********************************************************************************************************************
-----函数功能    任务参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static bool b_task_param_init(void)
{
	if(tpSysTask == NULL)
		return false;
	
	//系统任务参数
	memset(&tSysInfo, 0, sizeof(tSysInfo));
	
	tSysInfo.sMaxTemp = 25;				//设置默认最高温度
	tSysInfo.sMinTemp = 25;				//设置默认最低温度
	// tSysInfo.uInit.tFinish.bIF_Gpio = 1;
	// bSys_SetAutoOffTime(tAppMemParam.tSYS.usAutoOffTime);
	bSys_SetDevState(DS_INIT, false);	//进入初始化
	
	tp_task = tpSysTask;
	
	return true;
}

/***********************************************************************************************************************
-----函数功能    系统循环任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vSys_Task(void *pvParameters)
{
	#if(boardUSE_OS)
    for(;;)
	#endif
    {
		if(tp_task == NULL)
		{
			b_task_param_init();
			
			#if(boardUSE_OS)
			vTaskDelay(500);
			continue;
			#else
			return;
			#endif
		}

		if(tSysInfo.eDevState == DS_UPDATA_MODE)
		{
			vTaskDelay(1000);
			continue;
		}
		
		v_sys_check_prote();
		v_sys_get_perm();
		
		if(tp_task->vp_func != NULL && tp_task ->bNowRun == false)
			tp_task->vp_func(tp_task);
		else if(tp_task->vp_func == NULL || tp_task ->bNowRun == true)
		{
			#if(boardUSE_OS)
			if(lwrb_get_full(&tp_task->tQueueBuff) == 0)
				ulTaskNotifyTake(pdFALSE, sysTASK_CYCLE_TIME);//pdFALSE:任务通知多少次就执行多少次
			#endif  //boardUSE_OS
			
			if(tp_task->bp_task_manage_func != NULL)
				tp_task->bp_task_manage_func(tp_task);
		}
    }
}

/************************************************************************************************************************
*************************************************************************************************************************
                                                  局部函数
*************************************************************************************************************************
*************************************************************************************************************************/
/***********************************************************************************************************************
-----函数功能    检查系统保护
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_sys_check_prote(void)
{
	vu16 us_delay_time = 0;
	
	if(tpSysTask->ucID == STI_INIT || 
		tpSysTask->ucID == STI_CLOSING || 
		tpSysTask->ucID == STI_SHUT_DOWN)
		return;
	else if(tpSysTask->ucID == STI_BOOTING ||
			tpSysTask->ucID == STI_WORK)
	{
		us_delay_time = 10;
	}
	else 
		us_delay_time = sysTASK_CYCLE_TIME;
	
	//------------------------------------------------温度获取-----------------------------------------------
	s16 s_max_temp = 0;
	s16 s_min_temp = 0;
	
	tSysInfo.sBoardTempMax = 25;

	#if(boardBMS_IFACE)
	s_max_temp = MAX2(s_max_temp, tBms.sMaxTemp);
	s_min_temp = tBms.sMinTemp;
	#endif  //boardBMS_IFACE
	
	tSysInfo.sMaxTemp = MAX2(tSysInfo.sBoardTempMax, s_max_temp);
	
	tSysInfo.sMinTemp = s_min_temp;
	
	//------------------------------------------------系统输入过压-----------------------------------------------
	// static u16 us_over_volt_cnt = 0; 
	// if(bAdc_CheckSysInVolt() == VS_HIGH)
	// {
	// 	if(tSysInfo.uErrCode.tCode.bOV == 0)
	// 	{
	// 		us_over_volt_cnt++;
	// 		if(us_over_volt_cnt > (1000/us_delay_time))
	// 		{
	// 			us_over_volt_cnt = 0;
	// 			bSys_SetErrCode(SEC_OV, true);
	// 		}
	// 	}
	// }
	// else 
	// {
	// 	us_over_volt_cnt =0;
	// 	if(tSysInfo.uErrCode.tCode.bOV == 1)
	// 	{
	// 		tSysInfo.uErrCode.tCode.bOV = 0;
	// 		bSys_SetErrCode(SEC_OV, false);
	// 	}
		
	// }
	
	// //-----------------------------------------------系统输入欠压--------------------------------------------------
	// static u16 us_low_volt_cnt = 0; 
	// if((bAdc_CheckSysInVolt() == VS_LOW && bSys_ExistInVolt() == false) || tBms.uErrCode.tCode.uBmsCode.tCode.bCellUV)
	// {
	// 	if(tSysInfo.uErrCode.tCode.bUV == 0)
	// 	{
	// 		us_low_volt_cnt++;
	// 		if(us_low_volt_cnt > (1000/us_delay_time))
	// 		{
	// 			us_low_volt_cnt = 0;
	// 			bSys_SetErrCode(SEC_UV, true);
	// 		}
	// 	}
	// }
	// else 
	// {
	// 	us_low_volt_cnt =0;
	// 	if(tSysInfo.uErrCode.tCode.bUV == 1)
	// 	{
	// 		tSysInfo.uErrCode.tCode.bUV = 0;
	// 		bSys_SetErrCode(SEC_UV, false);
	// 	}
	// }
	
	//------------------------------------------------系统输入过温-----------------------------------------------------
	if(
	   #if(boardBMS_IFACE)
	   tBms.uErrCode.tCode.bSysChgOT == 1     ||
	   tBms.uErrCode.tCode.bSysDisChgOT == 1
	   #else
	   0
	   #endif  //boardBMS_IFACE
	  )
	{
		if(tSysInfo.uErrCode.tCode.bOT == 0)
			bSys_SetErrCode(SEC_OT, true);
	}
	else 
	{
		if(tSysInfo.uErrCode.tCode.bOT == 1)
			bSys_SetErrCode(SEC_OT, false);
	}
	
	
	//---------------------------------------------系统输入低温 ---------------------------------------------------
	if(
		#if(boardBMS_IFACE)
		tBms.uErrCode.tCode.bSysChgUT == 1     || 
	   	tBms.uErrCode.tCode.bSysDisChgUT == 1
		#else
		0
	   	#endif  //boardBMS_IFACE
	  )
	{
		if(tSysInfo.uErrCode.tCode.bUT == 0)
			bSys_SetErrCode(SEC_UT, true);
	}
	else 
	{
		if(tSysInfo.uErrCode.tCode.bUT == 1)
			bSys_SetErrCode(SEC_UT, false);
	}
	
	//-----------------------------------------系统过载保护---------------------------------------------------
	//过载信号原由DCAC模块提供,DCAC模块已移除,暂判定为无过载
	if(0)
	{
		if(tSysInfo.uErrCode.tCode.bOL == 0)
			bSys_SetErrCode(SEC_OL, true);
	}
	else
	{
		if(tSysInfo.uErrCode.tCode.bOL == 1)
			bSys_SetErrCode(SEC_OL, false);
	}
	
	//--------------------------------------低SOC自动关机--------------------------------------------------------
	// static vu16 us_soc_low_cnt = 0;
	// if(	
	// 	#if(boardBMS_IFACE)
	// 	ucBms_GetSoc() == 0 && 			//SOC = 0%
	// 	tBms.eDevState != DS_LOST &&	//BMS非离线
	// 	#endif  //boardBMS_IFACE
	// 	bSys_ExistInVolt() == false &&	//非充电状态
	// 	G_TestMode == false)			//非测试模式
	// {
	// 	if(tSysInfo.uErrCode.tCode.b0SOC == false)
	// 		us_soc_low_cnt++;
	// 	if(us_soc_low_cnt >= (2000/us_delay_time))
	// 	{
	// 		us_soc_low_cnt = 0;
	// 		bSys_SetErrCode(SEC_0_SOC, true);
	// 	}
	// }
	// else if(tSysInfo.uErrCode.tCode.b0SOC == 1)
	// 	bSys_SetErrCode(SEC_0_SOC, false);
}

/*****************************************************************************************************************
-----函数功能    获取系统的许可
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
static void v_sys_get_perm(void)
{
	vu16 us_delay_time = 0;
	
	//测试模式
	if(G_TestMode == true)
	{
		bSys_SetPerm(SPO_CHG,true);
		bSys_SetPerm(SPO_DISCHG,true);
		return;
	}
	//初始化状态或关机
	else if(tpSysTask->ucID == STI_INIT)
	{
		bSys_SetPerm(SPO_ALL,false);
		return;
	}
	
	//更新任务时间
	if(tpSysTask->ucID == STI_CLOSING ||
			tpSysTask->ucID == STI_BOOTING ||
			tpSysTask->ucID == STI_WORK)
	{
		us_delay_time = 10;
	}
	else 
		us_delay_time = sysTASK_CYCLE_TIME;
	
	//------------------------------------------充电许可------------------------------------------------------
	if(	
		#if(boardBMS_IFACE)
		tBms.uPerm.tPerm.bChgPerm == 0			||	//BMS不许可充电
		#endif  //boardBMS_IFACE
		tSysInfo.uPerm.tPerm.bForceClose == 1	||	//强制关闭
		tSysInfo.eDevState == DS_CLOSING		||	//开始关闭
		tSysInfo.uErrCode.tCode.bOV == 1)			//系统过压
	{
		if(tSysInfo.uPerm.tPerm.bChgPerm == true)
			bSys_SetPerm(SPO_CHG,false);
	}
	else
	{
		if(tSysInfo.uPerm.tPerm.bChgPerm == false)
			bSys_SetPerm(SPO_CHG,true);
	}
	
	//------------------------------------------放电许可------------------------------------------------------
	if(
		#if(boardBMS_IFACE)
		tBms.uPerm.tPerm.bDisChgPerm == 0		||	//BMS不许可放电
		#endif  //boardBMS_IFACE
		tSysInfo.uPerm.tPerm.bForceClose == 1	||	//强制关闭
		tSysInfo.eDevState == DS_CLOSING		||	//开始关闭
		tSysInfo.uErrCode.tCode.b0SOC == 1		||	//低SOC
		tSysInfo.uErrCode.tCode.bUV == 1)    		//欠压
	{
		if(tSysInfo.uPerm.tPerm.bDisChgPerm == true)
			bSys_SetPerm(SPO_DISCHG,false);
	}
	else
	{
		if(tSysInfo.uPerm.tPerm.bDisChgPerm == false)
			bSys_SetPerm(SPO_DISCHG,true);
	}
}






































/************************************************************************************************************************
*************************************************************************************************************************
                                                  全局函数
*************************************************************************************************************************
*************************************************************************************************************************/
/***********************************************************************************************************************
-----函数功能    工作过程中检测是否进入关机或重启,并进入倒计时
-----说明(备注)  没有此功能
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vSys_TickTimer(void) 
{
	static  u8 uc_cnt = 0;
	
	uc_cnt++;
	if(uc_cnt >= 2)
	{
		uc_cnt = 0;
		switch(tSysInfo.eDevState)
		{
			case DS_INIT:  
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = DS_INIT !\r\n");
			}
			break;
			
			case DS_CLOSING:  
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = DS_CLOSING !\r\n");
			}
			break;
			
			case DS_SHUT_DOWN:
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = DS_SHUT_DOWN !\r\n");
			}
			break;
			
			case DS_ERR:  
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = DS_ERR !\r\n");
			}
			break;

			case DS_BOOTING:
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = DS_BOOTING !\r\n");
			}
			break;
			
			case DS_WORK:
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask = DS_WORK !\r\n");
			}
			break;
			
			#if(boardENG_MODE_EN)
			case DS_ENG_MODE:
				if(uPrint.tFlag.bSysTask)
				{
					sMyPrint("bSysTask = DS_ENG_MODE !\r\n");
					#if printSEGGER
					SEGGER_RTT_printf(0,"bSysTask = DS_INIT !\r\n");
					#endif
				}
				break;
			#endif
			
			default:
				break;
		}
	}
//	log_e("这个是错误");
//	log_w("这个是警告");
//	log_i("这是一条提示");

	if(bSys_IsWorkState() == false) //非工作状态不检测 
		return;
	
	//***************************************************关机倒计时*****************************************************
	if(tSysInfo.usAutoOffTime) 
	{
		if(tSysInfo.usAutoOffCnt)
		{
			tSysInfo.usAutoOffCnt--;
			if(tSysInfo.usAutoOffCnt == 0)//倒计时为0进入
			{
				// bDisp_Switch(ST_ON, false);
				cSys_Switch(SO_KEY,ST_OFF, false);
				if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
					sMyPrint("Sys_Task:====倒计时结束,进入关机  时间=%dS====\r\n",tSysInfo.usAutoOffTime);
			}
		}	
	}
}

/***********************************************************************************************************************
-----函数功能    更新自动关机倒计时
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void  vSys_RefreshOffTime(void)
{
    if(tSysInfo.usAutoOffTime)
    {
        tSysInfo.usAutoOffCnt =  tSysInfo.usAutoOffTime;         //更新倒计时
    }
}

/***********************************************************************************************************************
-----函数功能    更新系统全部的关机倒计时
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vSys_RefreshAllOffTime(bool BLON)  //更新系统的自动关闭时间
{
    vSys_RefreshOffTime();        //系统关机倒计时      
}

/***********************************************************************************************************************
-----函数功能    自动关闭功能开关
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:操作成功  false:操作不成功
************************************************************************************************************************/
bool bSys_SetAutoOffTime(u16 time)
{
	tSysInfo.usAutoOffTime = time;
	vSys_RefreshOffTime();
	return true;
}

/***********************************************************************************************************************
-----函数功能    设置系统运行状态
-----说明(备注)  none
-----传入参数    step:
					  DS_INIT = 0,          // 初始化
					  DS_CLOSING ,          // 关闭中
					  DS_SHUT_DOWN,         // 关机状态
					  DS_ERR,               // 错误状态
					  DS_BOOTING,           // 装载中
					  DS_WORK,              // 工作状态
					  DS_ENG_MODE,          // 工程模式
				 bz:
					  true 打开 蜂鸣器
-----输出参数    none
-----返回值      true:操作成功   false:操作失败
************************************************************************************************************************/
bool bSys_SetDevState(DevState_E state, bool bz)
{
	if(tSysInfo.eDevState != state)
	{
		tSysInfo.eDevState = state;
		if(tSysInfo.eDevState == DS_INIT)  //初始化
		{
			gpioASSIST_OPEN_ON();
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----更新系统任务状态为初始化----\r\n");
		}
		else if(tSysInfo.eDevState == DS_CLOSING)  //关闭中
		{
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----更新系统任务状态为关闭中----\r\n");
		}
		else if(tSysInfo.eDevState == DS_SHUT_DOWN)  //关闭
		{
			gpioASSIST_OPEN_OFF();
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----更新系统任务状态为关闭----\r\n");
		}
		else if(tSysInfo.eDevState == DS_ERR)  //错误
		{
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----更新系统任务状态为错误----\r\n");
		}
		else if(tSysInfo.eDevState == DS_BOOTING)    //启动中
		{
			gpioASSIST_OPEN_ON();
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----更新系统任务状态为启动中----\r\n");
		}
		else if(tSysInfo.eDevState == DS_WORK)    //工作
		{
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----更新系统任务状态为工作----\r\n");
		}
		#if(boardENG_MODE_EN)
		else if(tSysInfo.eDevState == DS_ENG_MODE)  //工程模式
		{
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----更新系统任务状态为工程模式----\r\n");
		}
		#endif //boardENG_MODE_EN
		else if(tSysInfo.eDevState == DS_UPDATA_MODE)    //工作
		{
			if(uPrint.tFlag.bSysTask)
				sMyPrint("bSysTask:----更新系统任务状态为升级模式----\r\n");
		}
	}	
	
	// if(state != DS_UPDATA_MODE && tUpdata.eWorkState != UWS_NULL)
	// 	bUpdata_Init();

	// if(bz)
    //     bBuz_Tweet(LONG_1);
	
	return true;
}


/***********************************************************************************************************************
-----函数功能    检查系统活跃的状态
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:活跃  false:不活跃
************************************************************************************************************************/
bool bSys_CheckActState(void)
{
	if( bSys_IsChgState()	== true			||	//充电状态
		tSysInfo.usOutPwr	>  0			||	//处于放电
		bSys_ExistInVolt()	== true				//还插着电
	   )
	{
		return true;
	}
	else 
	{
		return false;
	}
}

/***********************************************************************************************************************
-----函数功能    存在输入电源
-----说明(备注)  还插着电
-----传入参数    none
-----输出参数    none
-----返回值      true:插着充电  false:断开充电
************************************************************************************************************************/
bool bSys_ExistInVolt(void)
{
	//输入电压检测原由DCAC/MPPT模块提供,模块已移除,暂判定为无输入电压
	if(0)
	{
		return true;
	}
	else 
	{
		return false;
	}
}

/***********************************************************************************************************************
-----函数功能    处于充电状态
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:充电  false:不充电
************************************************************************************************************************/
bool bSys_IsChgState(void)
{
	//充电状态原由DCAC/MPPT模块提供,模块已移除,暂判定为未充电
	if(0)
	{
		return true;
	}
	else 
	{
		return false;
	}
}


/***********************************************************************************************************************
-----函数功能    处于工作状态
-----说明(备注)  包含工作和启动中
-----传入参数    none
-----输出参数    none
-----返回值      true:工作  false:不工作
************************************************************************************************************************/
bool bSys_IsWorkState(void)
{
	if(tSysInfo.eDevState == DS_BOOTING || 
	   tSysInfo.eDevState == DS_WORK  
	  )
	{
		return true;
	}
	else 
	{
		return false;
	}
}

/***********************************************************************************************************************
-----函数功能    处于关机状态
-----说明(备注)  包含关机和关机中
-----传入参数    none
-----输出参数    none
-----返回值      true:充电  false:不充电
************************************************************************************************************************/
bool bSys_IsShutDownState(void)
{
	if(tSysInfo.eDevState == DS_CLOSING || 
	   tSysInfo.eDevState == DS_SHUT_DOWN
	  )
	{
		return true;
	}
	else 
	{
		return false;
	}
}

/***********************************************************************************************************************
-----函数功能    充电唤醒
-----说明(备注)  包含关机和关机中
-----传入参数    none
-----输出参数    none
-----返回值      true:有  false:无
************************************************************************************************************************/
bool bSys_ChgWakeUp(SwitchObject_E obj)
{
	//关机状态下 && 非强制关机
	if((bSys_IsShutDownState() ==true || 
		tSysInfo.eDevState == DS_INIT) && 
		tSysInfo.uPerm.tPerm.bForceClose == false)
	{
		cSys_Switch(obj, ST_ON, false); //开机
		if(uPrint.tFlag.bSysTask)
			sMyPrint("bSysTask:开启充电唤醒\r\n");
		return true;
	}
	else 
	{
		return false;
	}
}

/***********************************************************************************************************************
-----函数功能    系统开关机函数
-----说明(备注)  none
-----传入参数    开关类型
				 SST_NULL=0,//进行取反
				 SST_ON,
				 SST_OFF,
-----输出参数    none
-----返回值      小于0:有错误   等于0:没操作    大于0:操作成功
************************************************************************************************************************/
s8 cSys_Switch(SwitchObject_E obj,SwitchType_E type, bool fore_en)
{
	TaskInParam_U u_param;
	
	u_param.tTaskParam.ucObj = obj;
	
	switch(type)
	{
		case ST_ON:
		{
			if(bSys_IsWorkState() == true && 
				fore_en == false)
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:当前状态为工作,不允许开机.对象:%d\r\n",u_param.tTaskParam.ucObj);
				 
				return 0;
			}
			 
			goto LoopOn;
		}
		 
		case ST_OFF:
		{
			if(bSys_IsShutDownState() == true && 
				 fore_en == false)
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:当前状态为关机,不允许关机.对象:%d\r\n",u_param.tTaskParam.ucObj);
			 
				return 0;
			}
			
			goto LoopOff;	 
		 }
		 
		default:
		{
			if(bSys_IsShutDownState() == true)                                    
			{
				LoopOn:
				u_param.tTaskParam.ucParam = ST_ON;
				bSys_SetDevState(DS_BOOTING,true);
				// vGPIO_AssistBmsOpen(true);
				if(tSysInfo.eDevState == DS_INIT)
					cQueue_AddQueueTask(tpSysTask, STI_BOOTING, u_param.usTaskInParam, false);
				else 
					cQueue_AddQueueTask(tpSysTask, STI_BOOTING, u_param.usTaskInParam, fore_en);
			
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:开机\r\n");
			}
			else                                                                  
			{
				LoopOff:
				//插着充电线,不关机
				if(bSys_ExistInVolt() == true && fore_en == false)
				{
					// bBuz_Tweet(LONG_2);
				
					if(uPrint.tFlag.bSysTask)
						log_w("bSysTask:插着充电线,不允许关机");
					return -2;
				}
			
				// if(tDisp.bLight == true || fore_en == true)                              
				{ 
					u_param.tTaskParam.ucParam = ST_OFF;
					cQueue_AddQueueTask(tpSysTask, STI_CLOSING, u_param.usTaskInParam, fore_en);
				
					if(uPrint.tFlag.bSysTask)
						sMyPrint("bSysTask:关机\r\n");
				}
				// else  
				// {
				// 	if(uPrint.tFlag.bSysTask)
				// 		sMyPrint("bSysTask:屏幕休眠,开始唤醒屏幕\r\n");
				// }
			}
			// bDisp_Switch(ST_ON, false);
		}
		break;
	 }
	 
	 return 1;
}

/*****************************************************************************************************************
-----函数功能    初始化参数
-----说明(备注)  none
-----传入参数    p_sys_mem : 记忆参数结构体
-----输出参数    none
-----返回值      true:设置成功  反之失败
*****************************************************************************************************************/
bool bSys_MemParamInit(SysMemParam_T* p_sys_mem)
{
	p_sys_mem->usAutoOffTime = boardSYS_OFF_TIME;
	p_sys_mem->sMaxTemp = boardSYS_MAX_TEMP;
	p_sys_mem->sMinTemp = boardSYS_MIN_TEMP;
	p_sys_mem->usMinOpenVolt = boardSYS_OPEN_MIN_VOLT;
	p_sys_mem->bBuzSwitchOff = 0;
	return true;
}

/*****************************************************************************************************************
-----函数功能    设置记忆参数
-----说明(备注)  none
-----传入参数    add:true 增加   false:减少
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vSys_MemParamSet(u8 item, bool add)
{
	if(add == true)
	{
		if(item == 2)
		{
			if(tAppMemParam.tSYS.usAutoOffTime < 3600)
				tAppMemParam.tSYS.usAutoOffTime++;
		}
		else if(item == 3)
		{
			if(tAppMemParam.tSYS.sMaxTemp < 127)
				tAppMemParam.tSYS.sMaxTemp++;
		}
		else if(item == 4)
		{
			if(tAppMemParam.tSYS.sMinTemp < 127)
				tAppMemParam.tSYS.sMinTemp++;
		}
		else if(item == 5)
		{
			tAppMemParam.tSYS.usMinOpenVolt++;
		}
		
	}
	else 
	{
		if(item == 2)
		{
			if(tAppMemParam.tSYS.usAutoOffTime > 0)
				tAppMemParam.tSYS.usAutoOffTime--;
		}
		else if(item == 3)
		{
			if(tAppMemParam.tSYS.sMaxTemp > -127)
				tAppMemParam.tSYS.sMaxTemp--;
		}
		else if(item == 4)
		{
			if(tAppMemParam.tSYS.sMinTemp > -127)
				tAppMemParam.tSYS.sMinTemp--;
		}
		else if(item == 5)
		{
			tAppMemParam.tSYS.usMinOpenVolt--;
		}
	}
}

/*****************************************************************************************************************
-----函数功能    设置充放电许可
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:设置成功  反之失败
******************************************************************************************************************/
bool bSys_SetPerm(SysPermObject_E obj, bool en)
{
	switch(obj)
	{
		case SPO_CHG:
		{
			//状态变化
			if(en != tSysInfo.uPerm.tPerm.bChgPerm)
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:设置充电许可: 设置=%d 当前状态=%d \r\n",en,tSysInfo.uPerm.tPerm.bChgPerm);
				
				tSysInfo.uPerm.tPerm.bChgPerm = en;
			}
		}
		break;
		
		case SPO_DISCHG:
		{
			//状态变化
			if(en != tSysInfo.uPerm.tPerm.bDisChgPerm)
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:设置放电许可: 设置=%d 当前状态=%d \r\n",en,tSysInfo.uPerm.tPerm.bDisChgPerm);
				
				tSysInfo.uPerm.tPerm.bDisChgPerm = en;
			}
		}
		break;
		
		case SPO_FORCE_CLOSE:
		{
			//状态变化
			if(en != tSysInfo.uPerm.tPerm.bForceClose)
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:设置强制关机: 设置=%d 当前状态=%d \r\n",en,tSysInfo.uPerm.tPerm.bForceClose);
				
				tSysInfo.uPerm.tPerm.bForceClose = en;
			}
		}
		break;
		
		case SPO_ALL:
		{
			//状态变化
			if(en != tSysInfo.uPerm.tPerm.bChgPerm)
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:设置充电许可: 设置=%d 当前状态=%d \r\n",en,tSysInfo.uPerm.tPerm.bChgPerm);
				
				tSysInfo.uPerm.tPerm.bChgPerm = en;
			}
			
			//状态变化
			if(en != tSysInfo.uPerm.tPerm.bDisChgPerm)
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:设置放电许可: 设置=%d 当前状态=%d \r\n",en,tSysInfo.uPerm.tPerm.bDisChgPerm);
				
				tSysInfo.uPerm.tPerm.bDisChgPerm = en;
			}
			
			//状态变化
			if(en != tSysInfo.uPerm.tPerm.bForceClose)
			{
				if(uPrint.tFlag.bSysTask)
					sMyPrint("bSysTask:设置强制关机: 设置=%d 当前状态=%d \r\n",en,tSysInfo.uPerm.tPerm.bForceClose);
				
				tSysInfo.uPerm.tPerm.bForceClose = en;
			}
		}
		break;
		
		default:
			return false;
	}
	return true;
}


/***********************************************************************************************************************
-----函数功能    设置设备错误代码
-----说明(备注)  none
-----传入参数    ERR_CODE
-----输出参数    none
-----返回值      true:添加了任务,并立即执行  false:没有添加任务,或添加了任务不执行
************************************************************************************************************************/
bool bSys_SetErrCode(SysErrCode_E code, bool set)
{
	static SysErrCode_E e_next_code;
	static bool b_next_set;

	if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
	{
		if(e_next_code != code || b_next_set != set)
		{
			// if(tSysInfo.uErrCode.tCode.bOV)
			// 	log_e("bSysTask:输入过压 电压=%dV",tAdcSamp.usSysInVolt/10);
			// else if(tSysInfo.uErrCode.tCode.bUV)
			// 	log_e("bSysTask:输入欠压 电压=%dV",tAdcSamp.usSysInVolt/10);
			// else if(tSysInfo.uErrCode.tCode.bOT)
			// 	log_e("bSysTask:系统过温 %d摄氏度",tSysInfo.sMaxTemp);
			// else if(tSysInfo.uErrCode.tCode.bUT)
			// 	log_e("bSysTask:系统低温,%d摄氏度",tSysInfo.sMinTemp);
			// else if(tSysInfo.uErrCode.tCode.bOL)
			// 	log_e("bSysTask:系统过载");
			// else if(tSysInfo.uErrCode.tCode.b0SOC)
			// 	log_e("bSysTask:SOC = 0%");
			// else if(tSysInfo.uErrCode.tCode.bBootFault)
			// 	log_w("bSysTask:启动任务等待超时,开始关闭系统");
			// else if(tSysInfo.uErrCode.tCode.bCloseFault)
			// 	log_w("bSysTask:关闭系统任务等待超时,退出");
			// else
				log_e("bSysTask:系统错误 代码%d 类型%d",code,set);
			e_next_code = code;
			b_next_set = set;
		}
	}
	
	//有错误
	if(code > SEC_CLEAR_ALL)
	{
		if(set)
			ERR_SET(tSysInfo.uErrCode.usCode, (code - 1));
		else
			ERR_CLR(tSysInfo.uErrCode.usCode, (code - 1));
	}
	else
		tSysInfo.uErrCode.usCode = 0;
	
	if(tSysInfo.uErrCode.usCode)
	{
		if(tSysInfo.uErrCode.tCode.bUV ||
			tSysInfo.uErrCode.tCode.b0SOC)
			cQueue_AddQueueTask(tpSysTask, STI_ERR, code ,false);
	}
	else 
	{
		//清除错误,重新启动
		if(tSysInfo.eDevState == DS_ERR)
		{
			bSys_SetDevState(DS_WORK,true);
			
			if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
				log_i("bSysTask:清除错误,重新进入工作状态");
		}
	}
	
	return false;
}

/***********************************************************************************************************************
-----函数功能    欠压请求充电
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:请求欠压充电  false:没有
************************************************************************************************************************/
bool bSys_LowVoltReqChg(void)
{
	#if(boardBMS_IFACE)
	if(tBms.uErrCode.tCode.uBmsCode.tCode.bCellUV == false)
		return false;

	if(tBms.uPerm.tPerm.bChgPerm == false)
		return false;
	#endif  //boardBMS_IFACE

	if(bSys_ExistInVolt() == false)
		return false;

	return true;
}

