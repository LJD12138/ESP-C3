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
// #include "Buz/buz_task.h"
#include "Flash/flash_iface.h"
#include "flash_allot_table.h"


#if(boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif  //boardUSE_OS

//全局临界区互斥量(供 mainENTER_CRITICAL/mainEXIT_CRITICAL 使用)
#if(boardUSE_OS)
portMUX_TYPE mainCriticalMux = portMUX_INITIALIZER_UNLOCKED;
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

#if(boardUSE_OS && boardHEALTH_MONITOR_EN)
#include "get_heapstack_task.h"
#endif  //boardUSE_OS && boardHEALTH_MONITOR_EN

#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#include "Print/print_iface.h"
#endif  //boardPRINT_IFACE

#if(boardBMS_IFACE)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_rec_task.h"
#endif  //boardBMS_IFACE

#if(boardDISPLAY)
#include "MD_Display/md_display_task.h"
#endif  //boardDISPLAY

// #if(boardUPDATA)
// #include "Updata/updata_task.h"
// #endif  //boardUPDATA


void SysParamInit(void)
{
	#if(boardPRINT_IFACE)
	uPrint.tFlag.bImportant = 1;
	uPrint.tFlag.bAppInfo   = 1;
	uPrint.tFlag.bSysTask   = 1;
	uPrint.tFlag.bKeyTask   = 1;
	uPrint.tFlag.bAdcTask   = 0;
	uPrint.tFlag.bOperFlash = 0;

	uPrint.tFlag.bBmsRecTask= 0;
	uPrint.tFlag.bBmsTask   = 1;
	uPrint.tFlag.bDispTask   = 0;
	
	#if(boardHEALTH_MONITOR_EN)
	uPrint.tFlag.bHealthTask = 1;	//健康巡检遥测(CPU/队列/栈/堆)
	uPrint.tFlag.bFreeRTOS   = 1;	//FreeRTOS剩余堆/任务表打印
	#endif  //boardHEALTH_MONITOR_EN
	
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
	
	#if boardWDGT_EN
	vFwdgt_Init();
	#endif  //boardWDGT_EN
	
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
	(void)pvParameters;

	#if(boardUSE_OS)
    mainENTER_CRITICAL();
	#endif  //boardUSE_OS

	bSys_TaskInit();            //系统总任务

	#if(boardPRINT_IFACE)
	if(bPrint_TaskInit() == false)
		uPrint.ulFlag = 0;	//关闭所有调试
	#endif

    vKey_TaskInit();			//按键任务

	vTimer_TaskInit();			//系统计时

	#if(boardDISPLAY)
	bDisp_TaskInit();			//显示任务
	#endif  //boardDISPLAY

	#if(boardBMS_IFACE)
	bBms_TaskInit();            //BMS发送任务
	bBms_RecTaskInit();         //BMS接收任务
	#endif

	#if(boardUSE_OS && boardHEALTH_MONITOR_EN)
	cHealth_TaskInit();			//健康巡检任务(CPU/队列/栈/堆遥测)
	#endif  //boardUSE_OS && boardHEALTH_MONITOR_EN

	#if(boardUSE_OS)
	mainEXIT_CRITICAL();
    vTaskDelete(NULL);
	#endif  //boardUSE_OS
}

#if (configUSE_IDLE_HOOK)
/***********************************************************************************************************************
-----函数功能    FreeRTOS 空闲任务钩子函数
-----说明(备注)  所有任务阻塞且空闲任务被调度时触发; 仅累加空闲循环计数,
                  供健康巡检任务换算 CPU 占用率(不可调用阻塞型 API)
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vApplicationIdleHook(void)
{
	#if(boardHEALTH_MONITOR_EN)
	ulHealthIdleLoopCnt++;
	#endif  //boardHEALTH_MONITOR_EN
}
#endif  //configUSE_IDLE_HOOK

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
/***********************************************************************************************************************
-----函数功能    FreeRTOS 任务栈溢出安全捕获钩子
-----说明(备注)  检测到任务栈被踩踏时打印故障任务并关中断, 交由看门狗复位
-----传入参数    xTask: 溢出任务句柄; pcTaskName: 溢出任务名称
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
	(void)xTask;

	#if(boardPRINT_IFACE)
	printf("\r\n[FATAL ERROR] Stack Overflow detected in task: %s!\r\n", pcTaskName ? pcTaskName : "Unknown");
	#endif  //boardPRINT_IFACE

	taskDISABLE_INTERRUPTS();
	for (;;)
	{
	}
}
#endif  //configCHECK_FOR_STACK_OVERFLOW > 0






