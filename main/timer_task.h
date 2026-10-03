#ifndef TIMER_TASK_H_
#define TIMER_TASK_H_
#include "main.h"
#include "board_config.h"
#include "freertos/freertos.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#if(boardCONSOLE_485_IFACE_EN)
extern	TimerHandle_t 	tConsoleRxEnTimer; 	//单次定时器
#endif

#if(boardMPPT_485_IFACE_EN)
extern TimerHandle_t 	tMpptRxEnTimer; //单次定时器
#endif

#if(boardPRINT_485_IFACE_EN)
extern TimerHandle_t 	tPrintRxEnTimer; //单次定时器
#endif

extern TimerHandle_t 	tWakeUpBmsTimer;

#if(boardDCAC_485_IFACE_EN)
extern TimerHandle_t 	tDcacRxEnTimer;
#endif

extern TimerHandle_t 	tRepetTimer;   //重复定时器  repetition

void vTimer_TaskInit(void);
#if(boardLOW_POWER)
void vCount_EnterLowPower(void);
void vCount_ExitLowPower(void);
#endif
#endif
