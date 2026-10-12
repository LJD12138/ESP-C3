/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\main
 * File    : board_config.c
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : 板级硬件配置及系统初始化任务实现文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "board_config.h"
#include "timer_task.h"
#include "gpio_init.h"
#include "filter.h"

#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Flash/flash_iface.h"

#if (boardKEY_EN)
#include "Key/key_task.h"
#endif  /* boardKEY_EN */

#if (boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif  /* boardUSE_OS */

#if (boardCM_BACKTRACE)
#include <cm_backtrace.h>
#endif  /* boardCM_BACKTRACE */

#if (boardEASY_LOGGER)
#include "elog_Init.h"
#endif  /* boardEASY_LOGGER */

#if (boardEASY_FLASH)
#include "easyflash.h"
#endif  /* boardEASY_FLASH */

#if (boardWDGT_EN)
#include "fwdgt.h"
#endif  /* boardWDGT_EN */

#if (boardPRINT_IFACE)
#include "Print/print_iface.h"
#endif  /* boardPRINT_IFACE */

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  /* boardDISPLAY_EN */

#if (boardHEALTH_MONITOR_EN)
#include "get_heapstack_task.h"
#endif  /* boardHEALTH_MONITOR_EN */

//****************************************************Parameter Initialization**************************************************//
#if (boardUSE_OS)
/* 全局临界区互斥量 (供 mainENTER_CRITICAL / mainEXIT_CRITICAL 宏使用) */
portMUX_TYPE mainCriticalMux = portMUX_INITIALIZER_UNLOCKED;
#endif  /* boardUSE_OS */

typedef struct
{
	const char			*p_name;			/* 任务名称字符串 */
	s8					(*p_fn_init)(void);	/* 任务初始化函数指针 (>0: 成功, <=0: 失败) */
	s16					s_base_code;		/* 任务基准错误码偏移 (0, -10, -20, ...) */
	bool				b_critical;			/* 关键任务标志 (true: 失败中断并断言, false: 容错继续) */
	void				(*p_fn_on_fail)(s16 s_err_code);	/* 专属失败回调 (可为 NULL) */
}TaskInitEntry_T;

/* 任务创建故障诊断监控变量 (供调试器 Watch 窗口实时排查) */
s8                 c_ret                = 0;    /* 任务初始化返回值 (保持向下兼容) */
static const char *s_p_failed_task_name = NULL; /* 首个故障任务名称 */
static s16         s_s_failed_err_code  = 0;    /* 首个故障综合错误码 (Base + 内部步骤码) */
static uint32_t    s_ul_free_heap_bytes = 0;    /* 故障发生时 FreeRTOS 剩余堆大小 (字节) */

//****************************************************Function Declaration******************************************************//
#if (boardPRINT_IFACE)
static void v_print_init_fail_cb(s16 s_err_code);
#endif  /* boardPRINT_IFACE */

/* 表驱动式任务注册表 (统一配置任务函数、基准错误码与关键级别) */
static const TaskInitEntry_T c_task_init_table[] = {
	/* 任务名称       初始化函数        基准错误码  是否关键任务  失败回调 */
	{"SysTask",      cSys_TaskInit,          0,         true,        NULL},
	
	#if (boardPRINT_IFACE)
	{"PrintTask",    cPrint_TaskInit,        -10,       false,       v_print_init_fail_cb},
	#endif  /* boardPRINT_IFACE */
	
	#if (boardDISPLAY_EN)
	{"DispTask",     cDisp_TaskInit,         -30,       true,        NULL},
	#endif  /* boardDISPLAY_EN */
	
	#if (boardKEY_EN)
	{"KeyTask",      cKey_TaskInit,          -50,       true,        NULL},
	#endif  /* boardKEY_EN */
	
	{"TimerTask",    cTimer_TaskInit,        -190,      false,       NULL},
	
	#if (boardUSE_OS && boardHEALTH_MONITOR_EN)
	{"HealthTask",   cHealth_TaskInit,       -200,      false,       NULL},
	#endif  /* boardUSE_OS && boardHEALTH_MONITOR_EN */
};


/***********************************************************************************************************************
 * 函数功能    : 系统参数初始化
 * 说明(备注)  : 初始化打印标志位及系统运行基础参数
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void SysParamInit(void)
{
	#if (boardPRINT_IFACE)
	uPrint.tFlag.bImportant  = 1;
	uPrint.tFlag.bAppInfo    = 1;
	uPrint.tFlag.bSysTask    = 1;
	uPrint.tFlag.bKeyTask    = 1;
	uPrint.tFlag.bDispTask   = 0;
	uPrint.tFlag.bAdcTask    = 0;
	uPrint.tFlag.bOperFlash  = 0;
	uPrint.tFlag.bBmsTask    = 0;
	uPrint.tFlag.bBmsRecTask = 0;

	#if (boardHEALTH_MONITOR_EN)
	uPrint.tFlag.bHealthTask = 1;	/* 健康巡检遥测(CPU/队列/栈/堆) */
	uPrint.tFlag.bFreeRTOS   = 1;	/* FreeRTOS剩余堆/任务表打印 */
	#endif  /* boardHEALTH_MONITOR_EN */
	
	/* 非调试模式下，关闭一般调试输出 */
	#if (!boardDEBUG)
	uPrint.ulFlag = 0;
	uPrint.tFlag.bImportant  = 1;
	#endif  /* !boardDEBUG */

	#endif  /* boardPRINT_IFACE */
}

/***********************************************************************************************************************
 * 函数功能    : 初始化任务的任务参数 (系统底层及硬件抽象层初始化)
 * 说明(备注)  : 在系统调度器启动前执行，完成 GPIO、Flash、看门狗、调试串口及中间件初始化
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void vBoard_SysInit(void)
{
	vGPIO_Init();
	
	bFlash_IfaceInit();
	
	#if (boardWDGT_EN)
	vFwdgt_Init();
	#endif  /* boardWDGT_EN */
	
	#if (boardPRINT_IFACE)
	vPrint_IfaceInit();
	#endif  /* boardPRINT_IFACE */
	
	#if (boardSEGGER)
	SEGGER_RTT_Init();      /* 初始化Seeger RTT 输出 */
	SEGGER_SYSVIEW_Conf();  /* SystemView 初始化 */
	#endif  /* boardSEGGER */
	
	#if (boardCM_BACKTRACE)
	cm_backtrace_init("APP", boardHARDWARE_VERSION, boardSOFTWARE_VERSION);
	#endif  /* boardCM_BACKTRACE */
	
	#if (boardEASY_LOGGER)
	vElog_Init();
	#endif  /* boardEASY_LOGGER */
	
	#if (boardEASY_FLASH)
	EfErrCode ret = easyflash_init();
	if (ret != EF_NO_ERR)
		printf("EasyFlash init fail, EfErrCode = %d.r\n", ret);
	#endif  /* boardEASY_FLASH */
	
	SysParamInit();        /* 初始化系统参数 */
}

/***********************************************************************************************************************
 * 函数功能    : 系统启动任务 (创建各模块业务子任务)
 * 说明(备注)  : 在调度器启动后进入临界区，依次拉起各外设控制任务、业务逻辑任务，随后删除自身
 * 传入参数    : p_v_parameters: 任务创建参数指针
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void vBoard_StartTask(void *p_v_parameters)
{
	(void)p_v_parameters;

	mainENTER_CRITICAL();

	for (size_t i = 0; i < sizeof(c_task_init_table) / sizeof(c_task_init_table[0]); i++)
	{
		const TaskInitEntry_T *p_entry = &c_task_init_table[i];
		s8 ret = p_entry->p_fn_init();

		if (ret <= 0)
		{
			s16 s_err_code = p_entry->s_base_code + ret;

			/* 记录首个故障任务信息至监控变量 (便于仿真器 Watch 窗口排查) */
			if (s_p_failed_task_name == NULL)
			{
				s_p_failed_task_name = p_entry->p_name;
				s_s_failed_err_code  = s_err_code;
				#if (boardUSE_OS)
				s_ul_free_heap_bytes = (uint32_t)xPortGetFreeHeapSize();
				#endif  /* boardUSE_OS */
			}
			c_ret = (s8)s_err_code;

			/* 标记系统任务故障错误 */
			bSys_SetErrCode(SEC_TASK_FAULT, true);

			#if (boardPRINT_IFACE)
			printf("\r\n[ERROR] Task init failed! Task: %s, Err: %d (ret: %d), FreeHeap: %u B\r\n",
			       p_entry->p_name, (int)s_err_code, (int)ret, (unsigned int)s_ul_free_heap_bytes);
			#endif  /* boardPRINT_IFACE */

			/* 执行任务专属失败回调 (如关闭打印标志位等) */
			if (p_entry->p_fn_on_fail != NULL)
				p_entry->p_fn_on_fail(s_err_code);

			/* 关键任务创建失败: 触发系统断言并挂起，等待看门狗复位 */
			if (p_entry->b_critical)
			{
				#if (boardPRINT_IFACE)
				printf("[FATAL] Critical task failed! System halt.\r\n");
				#endif  /* boardPRINT_IFACE */
				
				configASSERT(0);
				taskDISABLE_INTERRUPTS();
				for (;;)
				{}
			}
		}
	}

	mainEXIT_CRITICAL();

	#if (boardUSE_OS)
	vTaskDelete(NULL);
	#endif  /* boardUSE_OS */
}

#if (configUSE_IDLE_HOOK)
/***********************************************************************************************************************
 * 函数功能    : FreeRTOS 空闲任务钩子函数
 * 说明(备注)  : 当所有任务处于阻塞态且空闲任务被调度运行时触发
 *              1. 执行一些低优先级的、后台的、需要连续执行的操作
 *              2. 测量系统的空闲时间，换算处理器占用率
 *              3. 钩子函数不可调用可能引起阻塞的 API 函数
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void vApplicationIdleHook(void)
{
	#if (boardHEALTH_MONITOR_EN)
	ulHealthIdleLoopCnt++;
	#endif  /* boardHEALTH_MONITOR_EN */
}
#endif  /* configUSE_IDLE_HOOK */

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
/***********************************************************************************************************************
 * 函数功能    : FreeRTOS 任务栈溢出安全捕获钩子函数
 * 说明(备注)  : 当检测到任务栈被踩踏击穿时触发，打印故障任务并关中断死等硬件看门狗复位
 * 传入参数    : x_task: 溢出任务句柄, p_c_task_name: 溢出任务名称字符串
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void vApplicationStackOverflowHook(TaskHandle_t x_task, char *p_c_task_name)
{
	(void)x_task;
	
	#if (boardPRINT_IFACE)
	printf("\r\n[FATAL ERROR] Stack Overflow detected in task: %s!\r\n", p_c_task_name ? p_c_task_name : "Unknown");
	#endif  /* boardPRINT_IFACE */

	/* 关全局中断，等待硬件看门狗复位以恢复系统安全 */
	taskDISABLE_INTERRUPTS();
	for (;;)
	{}
}
#endif  /* configCHECK_FOR_STACK_OVERFLOW > 0 */

#if (configUSE_MALLOC_FAILED_HOOK > 0)
/***********************************************************************************************************************
 * 函数功能    : FreeRTOS 动态内存分配失败安全捕获钩子函数
 * 说明(备注)  : 当 pvPortMalloc 申请内存超出剩余堆空间时触发
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void vApplicationMallocFailedHook(void)
{
	#if (boardPRINT_IFACE)
	printf("\r\n[FATAL ERROR] FreeRTOS pvPortMalloc failed! Heap exhausted!\r\n");
	#endif  /* boardPRINT_IFACE */

	taskDISABLE_INTERRUPTS();
	for (;;)
	{}
}
#endif  /* configUSE_MALLOC_FAILED_HOOK > 0 */

#if (configUSE_TICK_HOOK > 0)
/***********************************************************************************************************************
 * 函数功能    : FreeRTOS 滴答定时器中断钩子函数
 * 说明(备注)  : 每个时钟节拍中断中调用
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void vApplicationTickHook(void)
{
}
#endif  /* configUSE_TICK_HOOK > 0 */


#if (boardPRINT_IFACE)
/***********************************************************************************************************************
 * 函数功能    : 打印任务初始化失败回调
 * 说明(备注)  : 打印任务初始化失败时关闭全部打印调试标志位，防止未就绪打印引发溢出或死锁
 * 传入参数    : s_err_code: 综合错误码
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
static void v_print_init_fail_cb(s16 s_err_code)
{
	(void)s_err_code;
	uPrint.ulFlag = 0;	/* 关闭所有调试 */
}
#endif  /* boardPRINT_IFACE */
