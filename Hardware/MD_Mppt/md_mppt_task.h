#ifndef MD_MPPT_TASK_H
#define MD_MPPT_TASK_H

#include "board_config.h"

#if(boardMPPT_IFACE)
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif  //boardUSE_OS


#define       	mpptTASK_CYCLE_TIME               		100

extern     		Task_T									*tpMpptTask;

#if(boardUSE_OS)
extern     		TaskHandle_t                			tMpptTaskHandler;
#endif  //boardUSE_OS


//*********************************任务ID***********************************
typedef enum : uint8_t
{
	MTI_NULL = 0,       //空任务函数
    MTI_INIT,           //初始化电池包
    MTI_MAIN,      		//循环获取电池包的数据
    MTI_SET_CHG_PWR,   	//控制MPPT开启
	MTI_ERR_PROCESS,    //错误处理
}MpptTaskId_E;

//*********************************工作状态***********************************
typedef enum : uint8_t
{ 
	MWM_NULL = 0, 		//清除所有
	MWM_PV,
	MWM_DC,
}MpptWorkMode_E;

//*****************************错误状态*************************************
typedef enum 
{
    MEC_CLEAR_ALL = 0,   	//清所有错误	
	
	MEC_MPPT_IN_OC = 1,		//MPPT上报错误
	MEC_MPPT_IN_OV,			//MPPT上报错误
	MEC_MPPT_IN_ERR,		//MPPT上报错误
	MEC_MPPT_OUT_OC,		//MPPT上报错误
	MEC_MPPT_OUT_OV,		//MPPT上报错误
	MEC_MPPT_OUT_ERR,		//MPPT上报错误
	MEC_MPPT_OT,			//MPPT上报错误
	MEC_MPPT_OL,			//MPPT上报错误
	
	MEC_SYS_DEV_LOST= 17,   //设备丢失
	MEC_SYS_OVER_TEMP,      //过温
	MEC_SYS_LOW_TEMP,       //低温
	MEC_SYS_OVER_VOLT,      //过压
	MEC_SYS_OVER_LOAD,      //过载
	
}MpptErrCode_E;

typedef union
{
	struct
	{
		//MPPT上报错误(开始)
		vu32 bMpptInOC:1;
		vu32 bMpptInOV:1;
		vu32 bMpptInErr:1;  //短路
		vu32 bMpptOutOV:1;
		vu32 bMpptOutOC:1;
		vu32 bMpptOutErr:1;  //短路
		vu32 bMpptOT:1;
		vu32 bMpptOL:1;
		
		vu32 temp:8;
		//上报错误(结束)
		
		//系统判断的错误
		vu32 bSysDevLost:1;
		vu32 bSysOT:1;
		vu32 bSysUT:1;
		vu32 bSysOV:1;
		vu32 bSysOverLoad:1;

	}tCode;
	vu32 ulCode;  
}MpptErrState_U;

//*********************************任务对象**********************************
#pragma pack(1)
typedef struct
{
	DevState_E  		eDevState;          //设备状态
    MpptWorkMode_E  	eWorkMode;          //工作模式
	MpptErrState_U 		uErrCode;           //tMppt错误状态
	vu16             	usAutoOffTime;   	//关闭逆变器的时间,0为不开启  
    vu16             	usAutoOffCnt;    	//时间戳大于这个值就关闭逆变器
	vu16             	usInPwr;         	//输入的功率 W
	bool             	bChgPerm;         	//充电许可
	s16             	sMaxTemp;    
}
Mppt_T;
#pragma pack()
extern Mppt_T 			tMppt;

//*********************************记忆参数**********************************
#pragma pack(1)//强制一个字节对齐
typedef struct
{
	s8               	cAllowMaxTemp;      //允许的最高温度 1摄氏度
	vu16             	usAutoOffTime;   	//自动关闭时间  0为关闭此功能
	vu16             	usMaxInVolt;		//最大输入电压 0.1V
	vu16             	usMinInVolt;		//最小输入电压 0.1V
	vu16             	usMaxInCurr;		//最大输入电压 0.01A
	vu16             	usInPwrRating;     	//最大输入功率 0.1W
}MpptMemParam_T;
#pragma pack() //取消一个字节对齐


bool bMppt_TaskInit(void);
bool bMppt_SetDevState(DevState_E state);
bool bMppt_SetErrCode(MpptErrCode_E code, bool set);
s8 cMppt_SetChgPwr(u16 pwr);
bool bMppt_SetChgPerm(bool en);
bool bMppt_MemParamInit(MpptMemParam_T* p_mppt_mem);
void vMppt_MemParamSet(u8 item, bool add);

#if(!boardUSE_OS)
void vMppt_Task(void *pvParameters);
#endif  //boardUSE_OS

#endif  //boardMPPT_IFACE

#endif
