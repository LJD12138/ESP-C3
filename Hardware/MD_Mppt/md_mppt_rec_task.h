#ifndef MD_MPPT_REC_TASK_H
#define MD_MPPT_REC_TASK_H

#include "board_config.h"
#if(boardMPPT_IFACE)
#include "lwrb.h"

#if(boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif  //boardUSE_OS

extern lwrb_t tMpptReplyBuff;

#if(boardUSE_OS)
extern TaskHandle_t tMpptRecTaskHandle;
#endif  //boardUSE_OS

//*****************************返回参数*************************************
#pragma pack (1)   //强制进行1字节对齐
typedef struct
{
	InOutState_E		eChgState;			//充电状态
	vu16 				usInCurr;   		//输入电流 0.1A
	vu16 				usInVolt;   		//输入电压 0.1V
	vu16 				usInPwr;   			//输入电流 0.1W
	vu16    			usOutVolt;    		//0.1V
	vu16    			usOutCurr;    		//0.1A
	vu16    			usOutPwr;     		//0.1W
	vu16    			usChgPwr;     		//充电功率 W
	s8             		cMaxTemp;           //1摄氏度
	u8 					ucErrCode;   		//故障状态
}MpptParam_T;  
#pragma pack()   //取消进行1字节对齐

//*********************************输入类型**********************************
typedef enum  : uint8_t
{
    MIT_NULL = 0,
    MIT_DC,       //适配器      
    MIT_PV,       //太阳能
}MpptInType_U;

//*****************************错误状态*************************************
typedef union
{
	struct
	{
		vu16 bIOV:1;		//过压
		vu16 bIUV:1;		//欠压
		vu16 bOOV:1;		//过压
		vu16 bOUV:1;		//欠压

		vu16 bOL:1;			//过载
		vu16 bOT:1;			//过温
		vu16 bEnErr:1;		//使能故障
		
		vu16 :9;
	}tCode;
	vu16 usCode;   
}MpptRecErrState_U;

//*********************************任务对象**********************************
#pragma pack (1)   //强制进行1字节对齐
typedef struct
{											
	MpptInType_U		uInType;			//输入类型
	MpptRecErrState_U	uErrCode;           //错误状态
	u16           		usInVolt;      		//输入电压 0.1V
    u16           		usInCurr;      		//输入电流 0.01A
	u16 				usInPwr;			//输入功率 0.1W
    u16           		usOutVolt;      	//输出电压 0.1V
    u16           		usOutCurr;      	//输出电流 0.01A
	u16 				usOutPwr;			//输入功率 0.1W
	u16			  		usMaxInPwr;			//最大输入功率 0.1W
	s16					sMaxTemp;			//最大温度 摄氏度
}MpptRx_T;              
extern MpptRx_T     	tMpptRx; 
#pragma pack()   //取消进行1字节对齐


bool bMppt_RecTaskInit(void);
void vMppt_RecTickTimer(void);

#if(!boardUSE_OS)
void vMppt_RecTask(void *pvParameters);
#endif  //boardUSE_OS

#endif  //boardMPPT_IFACE

#endif  //MD_MPPT_REC_TASK_H


