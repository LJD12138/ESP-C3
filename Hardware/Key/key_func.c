/*****************************************************************************************************************
*                                                                                                                *
 *                                         按键功能                                                             *
*                                                                                                                *
******************************************************************************************************************/
#include "Key/key_func.h"
#include "Key/key_task.h"
#include "Sys/sys_task.h"
// #include "UsbDevice/usb_task.h"
// #include "Dc/dc_task.h"
// #include "Buz/buz_task.h"
#include "Print/print_task.h"
#include "MD_Light/md_light_task.h"
#include "MD_Display/md_display_task.h"
// #include "MD_Dcac/md_dcac_task.h"
#include "MD_Bms/md_bms_task.h"
// #include "MD_Mppt/md_mppt_task.h"

#include "app_info.h"
#include "timer_task.h"
#include "gpio_init.h"
#include "function.h"


#if(boardENG_MODE_EN)
#include "eng_mode.h"
#endif

//****************************************************参数初始化**************************************************//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////按键功能数组要求:不满十个触发类型的要加KTE_FUN_NULL作为结束符///////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if(boardENG_MODE_EN)
//工程模式按键功能
enum
{			
	EMKF_NULL = 0, 		//空按键
	EMKF_CHARGE,   		//改变
	EMKF_ADD,      		//增加
	EMKF_REDUCE,   		//减少
	EMKF_NEXT_OPTION,  	//下一项
	EMKF_COMFIRM,  		//确认
}eEngModeKeyFunc;
//工程模式按键类型
u8 const     KeyTriType_SetBuff[ 3 ]    = { KTE_AC_LONG, KTE_POWER_LONG, KTE_FUN_NULL};  //进入
u8 const     KeyTriType_Set1Buff[ 3 ]   = { KTE_POWER_LONG, KTE_AC_LONG, KTE_FUN_NULL};
u8 const     KeyTriType_ChargeBuff[ 2 ] = { KTE_POWER_SHORT, KTE_FUN_NULL};  //当前项循环
u8 const     KeyTriType_AddBuff[ 2 ]    = { KTE_AC_SHORT, KTE_FUN_NULL};     //增加
u8 const     KeyTriType_ReduceBuff[ 2 ] = { KTE_USB_SHORT, KTE_FUN_NULL};    //减少
u8 const     KeyTriType_NextOptionBuff[ 2 ]= { KTE_DC_SHORT, KTE_FUN_NULL};  //下一选项
u8 const     KeyTriType_ComfirmBuff[ 2 ]= { KTE_DC_LONG, KTE_FUN_NULL};      //确认
#endif

//长按        开关机
u8 const     KeyTriType_SysOnOffBuff[ 2 ] = { KTE_POWER_LONG, KTE_FUN_NULL};
//单击        切换屏幕
u8 const     KeyTriType_ScreenBuff[ 2 ] = { KTE_POWER_SHORT, KTE_FUN_NULL};
// //单击        开关USB
// u8 const     KeyTriType_USBOnOffBuff[ 2 ] = { KTE_USB_SHORT, KTE_FUN_NULL}; 
// //长击        开关USB
// u8 const     KeyTriType_USBOnOffBuff1[ 2 ] = { KTE_USB_LONG, KTE_FUN_NULL};
//长按        开关灯
u8 const     KeyTriType_LightOnOffBuff[ 2 ] = { KTE_LIGHT_LONG, KTE_FUN_NULL};  
//单击        切换灯
u8 const     KeyTriType_LightChargeBuff[ 2 ] = { KTE_LIGHT_SHORT, KTE_FUN_NULL};
//单击        开关AC
u8 const     KeyTriType_AcOnOffBuff[ 2 ] = { KTE_AC_SHORT, KTE_FUN_NULL}; 
//长击        开关AC
u8 const     KeyTriType_AcOnOffBuff1[ 2 ] = { KTE_AC_LONG, KTE_FUN_NULL};


//****************************************************函数声明**************************************************//
#if(boardENG_MODE_EN)
    static bool eng_mode_key_deal(u8 func);
#endif



/***********************************************************************************************************************
-----函数功能    按键功能处理函数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vKey_ProcKeyFunc(u8* pKeyTriTypeBuff)
{
	//******************************************开关机************************************************
	if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_SysOnOffBuff, sizeof(KeyTriType_SysOnOffBuff)))  
	{
        cSys_Switch(SO_KEY, ST_NULL, false);

        if (uPrint.tFlag.bKeyTask)
            sMyPrint("Key_Task:开关机");
	}
	#if(boardENG_MODE_EN)
	//******************************************工程模式************************************************************
	else if(tSysInfo.eDevState == DS_ENG_MODE)
	{
		if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_ChargeBuff, sizeof(KeyTriType_ChargeBuff)) )   //单击电源键
		{
			eEngModeKeyFunc = EMKF_CHARGE;
			bBuz_Tweet(SHORT_1);
			if(uPrint.tFlag.bKeyTask)
				sMyPrint("循环选择\r\n");
			
		}
		else if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_AddBuff, sizeof(KeyTriType_AddBuff)) )  //AC键短按  +
		{
			eEngModeKeyFunc = EMKF_ADD;
			bBuz_Tweet(SHORT_1);
			if(uPrint.tFlag.bKeyTask)
				sMyPrint("增加\r\n");
			
		}
		else if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_ReduceBuff, sizeof(KeyTriType_ReduceBuff))) //USB键短按  -
		{
			eEngModeKeyFunc = EMKF_REDUCE;
			bBuz_Tweet(SHORT_1);
			if(uPrint.tFlag.bKeyTask)
				sMyPrint("减少\r\n");
		}
		else if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_NextOptionBuff, sizeof(KeyTriType_NextOptionBuff))) //LIGHT键短按
		{
			eEngModeKeyFunc = EMKF_NEXT_OPTION;
			bBuz_Tweet(SHORT_1);
			if(uPrint.tFlag.bKeyTask)
				sMyPrint("下一项\r\n");
		}
		else if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_ComfirmBuff, sizeof(KeyTriType_ComfirmBuff))) //LIGHT键长按
		{
			eEngModeKeyFunc = EMKF_COMFIRM;
			bBuz_Tweet(SHORT_1);
			if(uPrint.tFlag.bKeyTask)
				sMyPrint("确认\r\n");
		}
		else 
		{
			eEngModeKeyFunc = EMKF_NULL;
			if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_NullBuff, sizeof(KeyTriType_NullBuff)))
			{
				bBuz_Tweet(SHORT_1);
			}
		}
		
		if(eEngModeKeyFunc != EMKF_NULL)
			eng_mode_key_deal(eEngModeKeyFunc);
	}
	//*****************************************待机或关机状态下**********************************************************
	else if (bSys_IsShutDownState() ==  true) //待机
	{
		//Power&AC键长按  设置时间---------------------------------------------------------------------------------------
		if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_SetBuff, sizeof(KeyTriType_SetBuff)) ||   
			bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_Set1Buff, sizeof(KeyTriType_Set1Buff)))
		{
			bSys_SetDevState(DS_ENG_MODE, false);  //进入工程模式
			
			if(uPrint.tFlag.bKeyTask)
				sMyPrint("进入工程模式\r\n");	
		}
	}
	#endif
    //*****************************************************工作状态下**************************************************//
    else if (tSysInfo.eDevState == DS_WORK)
    {
		if(bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_LightChargeBuff, sizeof(KeyTriType_LightChargeBuff)))    //单击LIGHT按键
		{
			vLight_CircSelectMode();
			if(uPrint.tFlag.bKeyTask)
				sMyPrint("Key_Task:切换照明模式\r\n");
		}
		else if(bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_LightOnOffBuff, sizeof(KeyTriType_LightOnOffBuff))) //长按LIGHT按键　
		{
			bLight_Switch(ST_NULL);
			if(uPrint.tFlag.bKeyTask)
				sMyPrint("Key_Task:开关照明\r\n");
		}
		else if(bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_AcOnOffBuff, sizeof(KeyTriType_AcOnOffBuff)))  //单击AC按键 
		{
			#if(boardDCAC_IFACE)
			if(cDCAC_Switch(DSO_AC_OUT, ST_NULL, true) == true)
				bDcac_SetAutoOffTime(boardDCAC_OFF_TIME);
			#endif  //boardDCAC_IFACE

			if(uPrint.tFlag.bKeyTask)
				sMyPrint("Key_Task:开关逆变\r\n");
		}
		else if(bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_AcOnOffBuff1, sizeof(KeyTriType_AcOnOffBuff1)))  //长按AC按键 
		{
			#if(boardDCAC_IFACE)
			if(cDCAC_Switch(DSO_AC_OUT, ST_NULL, true) == true)
				bDcac_SetAutoOffTime(0);
			#endif  //boardDCAC_IFACE

			if(uPrint.tFlag.bKeyTask)
				sMyPrint("Key_Task:开关逆变1\r\n");
		}
		else if(bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_ScreenBuff, sizeof(KeyTriType_ScreenBuff)))
		{
			#if(boardDISPLAY)
			bDisp_ChangeScreen(ID_MAIN_0);
			#endif  //boardDISPLAY

			if(uPrint.tFlag.bKeyTask)
				sMyPrint("Key_Task:切换屏幕\r\n");
		}
    }

    vKey_ParamInit();
}


#if(boardENG_MODE_EN)
/***********************************************************************************************************************
-----函数功能    工程模式按键功能
-----说明(备注)  none
-----传入参数    按键功能
				 EMKF_NULL = 0, //空按键
				 EMKF_CHARGE,   //改变
				 EMKF_ADD,      //增加
				 EMKF_REDUCE,   //减少
				 EMKF_COMFIRM,  //确认
-----输出参数    none
-----返回值      true:允许  false:不允许
************************************************************************************************************************/
static bool eng_mode_key_deal(u8 func)
{
	EngModeStep_E step = (EngModeStep_E)tSysInfo.ucRunStep;
	//累加设置
//	if(step ==EMS_TIME)
//	{
//		tKeyAC.bEnLongPressAdd = true;
//		tKeyLight.bEnLongPressAdd = true ;
//	}
//	else 
//	{
//		tKeyAC.bEnLongPressAdd = false;
//		tKeyLight.bEnLongPressAdd = false ;
//	}
	
	vEng_RefreshEngModeTime();
	
	switch(step)
	{
		case EMS_LCD:
		{
			if(func == EMKF_CHARGE)
				vLCD_TypeSelect();
			else if(func == EMKF_ADD)
				vLCD_LightSelect(true);
			else if(func == EMKF_REDUCE)
				vLCD_LightSelect(false);
			else if(func == EMKF_NEXT_OPTION)
			{
				bEng_SetEngStep(EMS_SYS);
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_系统设置-------\r\n");
			}
			else if(func == EMKF_COMFIRM)
			{
				bLCD_ExitSetLightness();
			}
		}break;
		
		case EMS_SYS:
		{
			if(func == EMKF_CHARGE)
				;
			else if(func == EMKF_ADD)
				;
			else if(func == EMKF_REDUCE)
				;
			else if(func == EMKF_NEXT_OPTION)
			{
				bEng_SetEngStep(EMS_BAT);
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_电池设置-------\r\n");
			}
			else if(func == EMKF_COMFIRM)
			{
				
			}
		}break;
		
		case EMS_BAT:
		{
			if(func == EMKF_CHARGE)
				;
			else if(func == EMKF_ADD)
				;
			else if(func == EMKF_REDUCE)
				;
			else if(func == EMKF_NEXT_OPTION)
			{
				bEng_SetEngStep(EMS_ADC);
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_ADC采样-------\r\n");
			}
			else if(func == EMKF_COMFIRM)
			{
				
			}
			
		}break;
		
		case EMS_ADC:
		{
			if(func == EMKF_CHARGE)
				;
			else if(func == EMKF_ADD)
				;
			else if(func == EMKF_REDUCE)
				;
			else if(func == EMKF_NEXT_OPTION)
			{
				bEng_SetEngStep(EMS_USB);
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_USB-------\r\n");
			}
			else if(func == EMKF_COMFIRM)
			{
				
			}
		}break;
		
		case EMS_USB:
		{
			if(func == EMKF_CHARGE)
				;
			else if(func == EMKF_ADD)
				;
			else if(func == EMKF_REDUCE)
				;
			else if(func == EMKF_NEXT_OPTION)
			{
				bEng_SetEngStep(EMS_DC);
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_AD-------\r\n");
			}
			else if(func == EMKF_COMFIRM)
			{
				
			}
		}break;
		
		case EMS_DC:
		{
			if(func == EMKF_CHARGE)
				;
			else if(func == EMKF_ADD)
				;
			else if(func == EMKF_REDUCE)
				;
			else if(func == EMKF_NEXT_OPTION)
			{
				bEng_SetEngStep(EMS_LIGHT);
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_照明-------\r\n");
			}
			else if(func == EMKF_COMFIRM)
			{
				
			}
		}break;
		
		case EMS_LIGHT:
		{
			if(func == EMKF_CHARGE)
				;
			else if(func == EMKF_ADD)
				;
			else if(func == EMKF_REDUCE)
				;
			else if(func == EMKF_NEXT_OPTION)
			{
				bEng_SetEngStep(EMS_UPDATA);
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_升级-------\r\n");
			}
			else if(func == EMKF_COMFIRM)
			{
				
			}
		}break;
		
		case EMS_UPDATA:
		{
			if(func == EMKF_CHARGE)
				;
			else if(func == EMKF_ADD)
				;
			else if(func == EMKF_REDUCE)
				;
			else if(func == EMKF_NEXT_OPTION)
			{
				bEng_SetEngStep(EMS_RESET);
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_重置-------\r\n");
			}
			else if(func == EMKF_COMFIRM)
			{
				
			}
		}break;
		
		case EMS_RESET:
		{
			if(func == EMKF_COMFIRM)   //确认
			{
				bApp_SysInfoInit(true);
			}
			else if(func == EMKF_NEXT_OPTION)  //下一项
			{
				bEng_SetEngStep(EMS_FINISH);
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_完成-------\r\n");
			}
			
		}break;
		
		default:
		case EMS_FINISH:
		{
			if(func == EMKF_NEXT_OPTION)  //下一项
			{
				bEng_SetEngStep(EMS_LCD);
			}
			else if(func == EMKF_COMFIRM)   //确认
			{
				cSys_Switch(SST_ON);      //开机
			}
			if(uPrint.tFlag.bKeyTask)
				sMyPrint("Key_Task:-------工程模式_显示屏设置-------\r\n");
		}break;
	}
	
	return true;
}
#endif
