/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Dcac/md_dcac_task.h"
// #include "Adc/adc_task.h"
#include "UsbDevice/usb_task.h"
#include "MD_Light/md_light_task.h"
#include "MD_Mppt/md_mppt_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"

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
		// vDcac_RefreshOffTime();
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
			if(tUsb.eDevState == DS_SHUT_DOWN && tSysInfo.uPerm.tPerm.bDisChgPerm)
			{
				cUsb_Switch(ST_ON, false);
			}
			
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
	// static u8 index = 0;
	// char cStr[8];
	// static u8 uc_light_pwr = 0;

	// tSysInfo.usOutPwr = tUsb.usOutPwr + uc_light_pwr;
	// #if(boardDCAC_IFACE)
	// tSysInfo.usOutPwr = tDcacRx.usOutPwr;
	// #endif  //boardDCAC_IFACE

	// tSysInfo.usInPwr = tUsb.usInPwr;
	// #if(boardMPPT_IFACE)
	// tSysInfo.usInPwr += tMppt.usInPwr;
	// #endif  //boardMPPT_IFACE

	// #if(boardDISPLAY)
	// switch(index)
	// {
	// 	case 0:
	// 	{
	// 		set_var_uc_bat_soc_value1(tBmsRx.tParam.usSOC);

	// 		snprintf(cStr, sizeof(cStr), "%d", tBmsRx.tParam.usSOC);
	// 		set_var_uc_bat_soc_value(cStr);
	// 		index++;
	// 	}
	// 	break;

	// 	case 1:
	// 	{
	// 		snprintf(cStr, sizeof(cStr), "%d", tSysInfo.usOutPwr);
	// 		set_var_uc_out_pwr_value(cStr);
	// 		index++;
	// 	}
	// 	break;

	// 	case 2:
	// 	{
	// 	   	snprintf(cStr, sizeof(cStr), "%d", tSysInfo.usInPwr);
	// 		set_var_uc_in_pwr_value(cStr);
	// 		index++;
	// 	}
	// 	break;

	// 	case 3:
	// 	{
	// 		snprintf(cStr, sizeof(cStr), "%d", tUsbDev.usC1Pwr);
	// 		set_var_uc_type_c1_pwr_value(cStr);

	// 		if(tUsbDev.eC1DevState == DS_WORK)
	// 			vDisp_SetDevStateIcon(DEV_TYPE_USB_C1,true);
	// 		else
	// 			vDisp_SetDevStateIcon(DEV_TYPE_USB_C1,false);
	// 		index++;
	// 	}
	// 	break;

	// 	case 4:
	// 	{
	// 		snprintf(cStr, sizeof(cStr), "%d", tUsbDev.usC2Pwr);
	// 		set_var_uc_type_c2_pwr_value(cStr);

	// 		if(tUsbDev.eC2DevState == DS_WORK)
	// 			vDisp_SetDevStateIcon(DEV_TYPE_USB_C2,true);
	// 		else
	// 			vDisp_SetDevStateIcon(DEV_TYPE_USB_C2,false);
	// 		index++;
	// 	}
	// 	break;

	// 	case 5:
	// 	{
	// 		snprintf(cStr, sizeof(cStr), "%d", tUsbDev.usUsbAPwr);
	// 		set_var_uc_usb_a_pwr_value(cStr);

	// 		if(tUsbDev.eUsbADevState == DS_WORK)
	// 			vDisp_SetDevStateIcon(DEV_TYPE_USB_A, true);
	// 		else
	// 			vDisp_SetDevStateIcon(DEV_TYPE_USB_A, false);
	// 		index++;
	// 	}
	// 	break;

	// 	case 6:
	// 	{
	// 		snprintf(cStr, sizeof(cStr), "%d", tUsbDev.usDcPwr);
	// 		set_var_uc_dc_pwr_value(cStr);

	// 		if(tUsbDev.eDcDevState == DS_WORK)
	// 			vDisp_SetDevStateIcon(DEV_TYPE_USB_DC, true);
	// 		else
	// 			vDisp_SetDevStateIcon(DEV_TYPE_USB_DC, false);
	// 		index++;
	// 	}
	// 	break;

	// 	case 7:
	// 	{
	// 		#if(boardMPPT_IFACE)
	// 		snprintf(cStr, sizeof(cStr), "%d", tMppt.usInPwr);
	// 		set_var_uc_pv_in_pwr_value(cStr);
			
	// 		if(tMppt.eDevState >= DS_BOOTING)
	// 			vDisp_SetDevStateIcon(DEV_TYPE_PV, true);
	// 		else
	// 			vDisp_SetDevStateIcon(DEV_TYPE_PV, false);
	// 		#endif  //boardMPPT_IFACE
	// 		index++;
	// 	}
	// 	break;

	// 	case 8:
	// 	{
	// 		#if(boardDCAC_IFACE)
	// 		snprintf(cStr, sizeof(cStr), "%d", tDcacRx.usOutPwr);
	// 		set_var_uc_ac_out_pwr_value(cStr);
			
	// 		if(tDcac.eDisChgState >= IOS_STARTING)
	// 			vDisp_SetDevStateIcon(DEV_TYPE_AC, true);
	// 		else
	// 			vDisp_SetDevStateIcon(DEV_TYPE_AC, false);
	// 		#endif  //boardDCAC_IFACE
	// 		index++;
	// 	}
	// 	break;

	// 	case 9:
	// 	{
	// 		if(tLight.eWordMode == LWM_FULL)
	// 			uc_light_pwr = 4;
	// 		else if(tLight.eWordMode == LWM_HALF)
	// 			uc_light_pwr = 2;
	// 		else if(tLight.eWordMode == LWM_SOS)
	// 			uc_light_pwr = 2;
	// 		else if(tLight.eWordMode == LWM_TWINKLE)
	// 			uc_light_pwr = 2;
	// 		else 
	// 			uc_light_pwr = 0;
			
	// 		tSysInfo.usOutPwr += uc_light_pwr;
	// 		snprintf(cStr, sizeof(cStr), "%d", uc_light_pwr);
	// 		set_var_uc_light_out_pwr_value(cStr);

	// 		if(tLight.eDevState == DS_WORK)
	// 			vDisp_SetDevStateIcon(DEV_TYPE_LIGHT, true);
	// 		else
	// 			vDisp_SetDevStateIcon(DEV_TYPE_LIGHT, false);
	// 		index++;
	// 	}
	// 	break;
	
	// 	default:
	// 	index = 0;
	// 			break;
	// }
	// #endif  //boardPRINT_IFACE
}

