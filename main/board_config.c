/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统初始化任务                                                        *
*                                                                                                                *
******************************************************************************************************************/
#include "board_config.h"
#include "timer_task.h"
#include "gpio_init.h"
#include "app_info.h"

#include "Sys/sys_task.h"
// #include "Adc/adc_task.h"
#include "Key/key_task.h"
// #include "Led/led_task.h"
// #include "Dc/dc_task.h"
#include "UsbDevice/usb_task.h"
// #include "Buz/buz_task.h"
#include "Flash/flash_iface.h"
#include "MD_Light/md_light_task.h"
#include "MD_HeatManage/md_hm_task.h"
#include "flash_allot_table.h"


#if(boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif  //boardUSE_OS

#if(boardCM_BACKTRACE)
#include <cm_backtrace.h>
#endif  //boardCM_BACKTRACE

#if(boardEASY_LOGGER)
#include "elog.h"
#endif  //boardEASY_LOGGER

#if(boardEASY_FLASH)
#include "easyflash.h"
#endif  //boardEASY_FLASH

#if(boardWDGT_EN)
#include "fwdgt.h"
#endif  //boardWDGT_EN

#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#include "Print/print_iface.h"
#endif  //boardPRINT_IFACE

#if(boardBMS_IFACE)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_rec_task.h"
#endif  //boardBMS_IFACE

#if(boardMPPT_IFACE)
#include "MD_Mppt/md_mppt_task.h"
#include "MD_Mppt/md_mppt_rec_task.h"
#endif  //boardMPPT_IFACE

#if(boardDCAC_IFACE)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#endif  //boardDCAC_IFACE

#if(boardDISPLAY)
#include "MD_Display/md_display_task.h"
#endif  //boardDISPLAY

// #if(boardUPDATA)
// #include "Updata/updata_task.h"
// #endif  //boardUPDATA


void SysParamInit(void)
{
	#if boardWDGT_EN
	vFwdgt_Init();
	#endif
	
	#if(boardPRINT_IFACE)
	uPrint.tFlag.bImportant = 1;
	uPrint.tFlag.bAppInfo   = 1;
	uPrint.tFlag.bSysTask   = 1;
	uPrint.tFlag.bKeyTask   = 1;
	uPrint.tFlag.bAdcTask   = 0;
	uPrint.tFlag.bOperFlash = 0;

	uPrint.tFlag.bBmsRecTask= 0;
	uPrint.tFlag.bBmsTask   = 1;
	uPrint.tFlag.bDcacTask  = 1;
	uPrint.tFlag.bDcacRecTask= 1;
	uPrint.tFlag.bMpptTask  = 1;
	uPrint.tFlag.bMpptRecTask= 1;
	uPrint.tFlag.bUsbTask   = 1;
	uPrint.tFlag.bDcTask    = 0;
	uPrint.tFlag.bDispTask   = 0;
	
	#if(boardUSE_OS_DEBUG_OUT)
	uPrint.tFlag.bFreeRTOS = 1;
	#endif  //boardUSE_OS_DEBUG_OUT
	#endif
}


/***********************************************************************************************************************
-----函数功能    初始化任务的任务参数    第一步
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vBoard_SysInit(void)
{
	vGPIO_Init();
	
	bFlash_IfaceInit();
	
	#if(boardPRINT_IFACE)
	vPrint_Init();
	#endif  //boardPRINT_IFACE
	
	#if boardSEGGER
	SEGGER_RTT_Init();      //初始化Seeger RTT 输出 
	SEGGER_SYSVIEW_Conf();  //SystemView 初始化
	#endif  //boardSEGGER
	
	#if(boardCM_BACKTRACE)
	cm_backtrace_init("APP", boardHARDWARE_VERSION, boardSOFTWARE_VERSION);
	#endif  //boardCM_BACKTRACE
	
	#if(boardEASY_LOGGER)
	vElog_Init();
	#endif  //boardEASY_LOGGER
	
	#if(boardEASY_FLASH)
	EfErrCode ret = easyflash_init();
	if(ret != EF_NO_ERR)
		printf("EasyFlash init fail, EfErrCode = %d.r\n", ret);
	#endif  //boardEASY_FLASH
	
	SysParamInit();        //初始化系统参数
}





/***********************************************************************************************************************
-----函数功能    初始化任务的任务    第二步
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vBoard_StartTask(void *pvParameters)
{
	#if(boardUSE_OS)
    // taskENTER_CRITICAL();
	#endif  //boardUSE_OS

	bSys_TaskInit();            //系统总任务

	#if(boardPRINT_IFACE)
	if(bPrint_TaskInit() == false)
		uPrint.ulFlag = 0;	//关闭所有调试
	#endif

    vKey_TaskInit();			//按键任务

	vTimer_TaskInit();			//系统计时

	// vLight_TaskInit();           //照明灯任务

	// bUsb_TaskInit();			//USB任务

	// bHM_TaskInit();             //风扇散热任务

	#if(boardDISPLAY)
	bDisp_TaskInit();			//显示任务
	#endif  //boardDISPLAY

	#if(boardDCAC_IFACE)
	bDcac_TaskInit();           //逆变器发送任务
	bDcac_RecTaskInit();        //逆变器接收任务
	#endif
	
	#if(boardBMS_IFACE)
	bBms_TaskInit();            //BMS发送任务
	bBms_RecTaskInit();         //BMS接收任务
	#endif
	
	#if(boardMPPT_IFACE)
	bMppt_TaskInit();           //MPPT发送任务
	bMppt_RecTaskInit();        //MPPT接收任务
	#endif
	
	#if(boardUSE_OS)
    vTaskDelete(NULL);   
    // taskEXIT_CRITICAL();
	#endif  //boardUSE_OS
}






