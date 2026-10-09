#ifndef MD_DCAC_REC_TASK_H
#define MD_DCAC_REC_TASK_H

#include "board_config.h"

#if(boardDCAC_IFACE)
#include "lwrb.h"

#if(boardUSE_OS)
#include "freertos/freertos.h"
#include "freertos/task.h"
#endif  //boardUSE_OS

#define       	dcacREC_TASK_CYCLE_TIME               	30

extern lwrb_t tDcacReplyBuff;

#if(boardUSE_OS)
extern TaskHandle_t tDcacRecTaskHandle;
#endif  //boardUSE_OS

//*****************************返回参数*************************************
#pragma pack (1)   //强制进行1字节对齐
typedef struct
{
	vu8					ucState;
	vu8					ucErrCode;
	vu16    			usOutCurr;    		//0.1A
	vu16    			usOutVolt;			//0.1V
	vu16    			usHighVolt;			//0.1V
	vu16    			usOutPwr;     		//0.1W
	vs8     			cTemp1;          	//1℃ 逆变温度
}DCAC_Param1_t;  
#pragma pack()   //取消进行1字节对


//*****************************工作状态*************************************
typedef union
{
	struct
	{								
		vu16 bInit:1;			//初始化中
		vu16 bDcMode:1;			//适配器模式
		vu16 bPvMode:1;			//太阳能模式
		vu16 bPvOff:1;			//PV关闭
		vu16 bPvErr:1;			//PV错误
		vu16 :3;
		
		vu16 bAcDisChg:1;		//初始化中
		vu16 bAcChg:1;			//适配器模式
		vu16 bAcUpf:1;			//太阳能模式
		vu16 bAcErr:1;			//太阳能模式
		vu16 :4;
	}tState;
	vu16 usState;   
}DcacWorkState_U;

//*****************************错误状态*************************************
typedef union
{
	struct
	{
		struct
		{
			vu16 bIOV:1;		//过压
			vu16 bIUV:1;		//欠压
			vu16 bOOV:1;		//过压
			vu16 bOUV:1;		//欠压
			vu16 bOL:1;			//过载
			vu16 bOT:1;			//过温
			vu16 bSC:1;			//短路
			vu16 bEnErr:1;		//使能故障
			vu16 :8;
		}tSys;

		vu16 usTemp[3];			//
		
	}tCode;
	
	vu16 usCode[4];   
}DcacErrState_U;

//*********************************任务对象**********************************
typedef struct
{
	//输入
	vu16				usInVolt;     		//0.1V
	vu16    			usInCurr;			//0.1A
	vu16    			usInPwr;			//W
	vu16    			usInChgPwr;			//W
	vu16    			usInFreq;     		//0.1HZ
	//输出
	vu16    			usOutVolt;    		//0.1V
	vu16    			usOutCurr;    		//0.1A
	vu16    			usOutPwr;     		//W
	vu16    			usOutFreq;    		//0.1HZ
	//并网
	vu16    			usParaInMaxPwr;     //并网最大功率 W
	vu16    			usParaInPwr;        //并网功率 W
	vu16    			usParaInMode;       //并网模式
	//状态
	vs16    			usChgPwr;     		//充电功率 W
	vu16    			usMaxInPwr;        	//最大输入功率 W
	vs16             	sMaxTemp;           //1摄氏度
	vs16             	sMinTemp;           //1摄氏度
	DcacWorkState_U    	uState;        		//状态
	DcacErrState_U		uErrCode;			//错误代码
}DcacRx_T;
extern DcacRx_T   		tDcacRx; 

bool bDcac_RecTaskInit(void);
void vDcac_RecTickTimer(void);

#if(!boardUSE_OS)
void vDcac_RecTask(void *pvParameters);
#endif  //boardUSE_OS

#if(boardLOW_POWER)
void vDcac_EnterLowPower(void);
void vDcac_ExitLowPower(void);
#endif  //boardLOW_POWER

#endif  //boardDCAC_IFACE

#endif  //MD_DCAC_PROCESS_H


