
#ifndef __MAIN_H
#define __MAIN_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "board_config.h"

//#define		//4Tab									//10Tab
#define     	mainINIT_BOOT_PARAM_FLAG     			0x0000000	//初始化BOOT参数
#define     	mainINIT_FINISH_FLAG        			0x88888888	//系统初始化完成
#define     	mainUPDATA_FLAG             			0xAAAAAAAA	//上位机
#define     	mainDISPLAY_FLAG            			0xAAAABBBB	//显示
#define     	mainLOW_POWER_FLAG          			0xBBBBCCCC	//进入低功耗
#define     	mainINIT_APP_PARAM_FLAG     			0xEFEFFEFE	//初始化APP参数

#define     	RANGE(val, min, max)     				((val < min) ? false : ((val > max) ? false : true))  //在范围内为true,包含min和max   
#define     	RANGE_NO(val, min, max)     			((val < min) ? true  : ((val > max) ? true  : false)) //在范围外为true
#define     	LIMIT(X, min, MAX)          			((X) <= (min) ? (min) : ((X) >= (MAX) ? (MAX) : (X)))
#define     	LIMIT_MAX(X, MAX)           			((X) < (MAX) ? (X) : (MAX))
#define     	LIMIT_MIN(X, MIN)           			((X) > (MIN) ? (X) : (MIN))

#define     	MSEC(TIME)                  			((TIME) / tickTime)                         //ms转成实际的 tick时间
#define     	MAX2(a, b)                  			((a > b) ? a : b)
#define     	MAX3(a, b, c)               			(a) > (b) ? ((a) > (c) ? (a) : (c)) : ((b) > (c) ? (b) : (c))
#define     	MIN2(a, b)                  			((a < b) ? a : b)
#define    		MIN3(a, b, c)               			(a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c))
#define     	DIFFER(val1, val2)          			((val1 <= val2) ? (val2 - val1) : (val1 - val2))

#define     	wb(addr, value)             			(*((u8  volatile *) (addr)) = value)
#define     	rb(addr)                    			(*((u8  volatile *) (addr)))
#define     	whw(addr, value)            			(*((u16 volatile *) (addr)) = value)
#define     	rhw(addr)                   			(*((u16 volatile *) (addr)))
#define     	ww(addr, value)             			(*((u32 volatile *) (addr)) = value)
#define     	ARRAYNUM(arr_name)          			(uint32_t)(sizeof(arr_name)/sizeof(*(arr_name)))
	

#ifndef   		__ALIGNED
#define 		__ALIGNED(x)                           	__attribute__((aligned(x)))
#endif

#define     	BIT_SET(obj, bitMask)         			(obj |= bitMask)
#define     	BIT_CLR(obj, bitMask)         			(obj &= (~bitMask))

#define     	BIT_GET(obj, XX_CODE)        			((obj >> XX_CODE) & (0x01))

#define     	ERR_SET(obj, ERR_CODE)         			BIT_SET(obj, ((u16)0x1 << ERR_CODE))
#define     	ERR_CLR(obj, ERR_CODE)         			BIT_CLR(obj, ((u16)0x1 << ERR_CODE))

#define     	STAT_SET(obj, STAT_CODE)        		BIT_SET(obj, ((u16)0x1 << STAT_CODE))
#define     	STAT_CLR(obj, STAT_CODE)        		BIT_CLR(obj, ((u16)0x1 << STAT_CODE))


#define         __STATIC_INLINE                        static inline

//**********************************有符号*********************************************************************
typedef       	int64_t  								s64;
typedef       	int32_t  								s32;
typedef       	int16_t  								s16;
typedef       	int8_t   								s8;

typedef volatile  	int64_t  								vs64;
typedef volatile  	int32_t  								vs32;
typedef volatile  	int16_t  								vs16;
typedef volatile  	int8_t   								vs8;

//**********************************无符号*********************************************************************
typedef       	uint64_t 								u64;
typedef       	uint32_t 								u32;
typedef       	uint16_t 								u16;
typedef       	uint8_t  								u8;

typedef volatile  	uint64_t 								vu64;
typedef volatile  	uint32_t 								vu32;
typedef volatile  	uint16_t 								vu16;
typedef volatile  	uint8_t  								vu8;

//系统运行状态
typedef enum : uint8_t
{
	DS_LOST = 0,		// 丢失
	DS_INIT,	       	// 初始化
	DS_CLOSING ,       	// 关闭中
	DS_SHUT_DOWN,      	// 关机状态 3
	DS_ERR,            	// 错误状态
    DS_BOOTING,        	// 装载中  5
    DS_WORK,           	// 工作状态
	#if(boardENG_MODE_EN)
    DS_ENG_MODE,       // 工程模式 engineering mode
    #endif
	DS_UPDATA_MODE,		// 升级模式 
}DevState_E;


//开关类型
typedef enum : uint8_t
{
	ST_OFF = 0,//进行取反
	ST_ON,
	ST_NULL,
}SwitchType_E;

//开关的对象
typedef enum : uint8_t
{
	SO_KEY=0,  		//按键
	SO_CONSOLE,    	//面板
	SO_PARA,     	//并机
	SO_DCAC,    	//逆变充电激活
	SO_MPPT,    	//MPPT充电激活
}SwitchObject_E;

//许可
#pragma pack(1)
typedef union
{
	struct 
	{
		u8 bChgPerm		:1;//充电许可
		u8 bDisChgPerm	:1;//放电许可
		u8 bForceClose	:1;//强制关闭
		u8 temp			:5;
	}tPerm;
	u8 ucPerm;
}Perm_U;
#pragma pack()

//操作的对象
typedef enum : uint8_t
{
	OO_CHG=0,  		//充电
	OO_DISCHG,    	//放电
	OO_ALL,    		//充放电
	OO_PARA_IN,    	//并网
}OperaObject_E;

//输入输出状态
typedef enum : uint8_t
{
	IOS_CLOSING=0,  	//关闭中
	IOS_SHUT_DOWN,  	//关闭
	IOS_PROTE,      	//保护  需要关机清除
	IOS_ERR,        	//错误  可以移除输入清除
	IOS_STARTING,   	//启动中
	IOS_WORK,       	//工作
}InOutState_E;

//步骤
typedef enum : uint8_t
{
	STEP_FORWARD = (u8)0xfd,//上一步
	STEP_NEXT = (u8)0xfe,//下一步
	STEP_END = (u8)0xff,//结束
}Step_E;

//模块对象
typedef enum : uint8_t
{
	MO_CONSOLE=0,
	MO_BMS,
	MO_MPPT,
}ModuleObject_E;

typedef union
{
	struct
	{
		u8 ucObj;
		u8 ucParam;
	}tTaskParam;
	u16 usTaskInParam;
}TaskInParam_U;


#endif /* __MAIN_H */
