/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_prot_frame.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : Print通信协议帧组包、解析、分发及中继处理实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Print/print_prot_frame.h"
#include "lwrb.h"
#include "main.h"

#if (boardPRINT_IFACE)
#include "Print/print_iface.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"
#include "flash_allot_table.h"

#include "app_info.h"

#if(boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  //boardUPDATE

#if(boardADC_EN)
#include "Adc/adc_task.h"
#endif  //boardADC_EN

#if(boardKEY_EN)
#include "Key/key_task.h"
#endif  //boardKEY_EN

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  //boardDISPLAY_EN

#if(boardBMS_EN)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_EN

#if (boardUSE_OS)
#include "freertos/freertos.h"
#include "freertos/task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			printDEV_ADRR							printCONSOLE_MASTER_ADDR
#define			printWAIT_NOTIFY_OUTTIME				1000	/* 任务通知超时时间 MS */
#define			printTX_FRAME_SIZE						256
#define			printRX_FRAME_SIZE						256

/* 任务状态调试结构体（用于向上位机上报任务运行状态，共10字节） */
#pragma pack(1)
typedef struct
{
	u8					ucID;				/* 当前任务ID */
	u8					ucStep;				/* 当前步骤 */
	u16					usInParam;			/* 函数参数 */
	u16					usStepWaitCnt;		/* 步骤等待次数 */
	u16					usStepRepeatCnt;	/* 步骤重复次数 */
	u16					usTaskWaitCnt;		/* 任务等待次数 */
}TaskDebug_T;
#pragma pack()

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) BaikuProtoTx_t *tpPrintProtoTx = NULL;	/* 发送协议 */
__ALIGNED(4) BaikuProtoRx_t *tpPrintProtoRx = NULL;

vu8 uc_next_cmd = 0;

//****************************************************Function Declaration******************************************************//
static s8 c_print_data_trans(u8 cmd, u8 *data, u8 len);
#if (boardUPDATE)
static s8 c_print_data_trans_for_update(u8 cmd, u8* data, u8 len);
#endif  /* boardUPDATE */
static s8 c_get_console_ver_info(u8* data, u8* data_len);
static s8 c_relay_console_info(BaikuProtoRx_t* proto);
static s8 c_set_console_info(BaikuProtoRx_t* proto);


/***********************************************************************************************************************
 * 函数功能    : 发送通讯协议初始化
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bPrint_SendProtInit(void)
{
	s8 c_result = cBaiku_ProtoSendInit(&tpPrintProtoTx,		/* 协议指针 */
	                                   printTX_FRAME_SIZE,	/* 协议缓存器大小 */
	                                   printDEV_ADRR);		/* 协议设备ID */
	if (c_result <= 0)
		return false;
	
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 接收通讯协议初始化
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bPrint_RecProtInit(void)
{
	s8 c_result = cBaiku_ProtoRecInit(&tpPrintProtoRx,				/* 协议指针 */
	                                  printRX_FRAME_SIZE,			/* 协议缓存器大小 */
	                                  sysDEV_ADRR,					/* 协议设备ID */
	                                  boardREPET_TIMER_CYCLE_TMIE);	/* 计数器采样时间 */
	if (c_result <= 0)
		return false;
	
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 周期循环回复数据
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : 1: 发送成功, 0: 未触发
 ************************************************************************************************************************/
s8 c_cycle_relay_data(void)
{
	static vu16 us_delay_cnt = 0;
	
	if (uc_next_cmd != 0x0D)
	{
		us_delay_cnt = 0;
		return 0;
	}
	
	us_delay_cnt++;
	if (us_delay_cnt >= (100 / printTASK_CYCLE_TIME))
		us_delay_cnt = 0;
		/* c_relay0E_mppt_param(); */
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 回复模块开关结果  0x02
 * 说明(备注)  : none
 * 传入参数    : data: [0]开关对象, [1]开关动作与执行结果
 * 输出参数    : none
 * 返回值      : true:成功   false:失败
 ************************************************************************************************************************/
s8 c_relay02_switch_result(uint8_t data[])
{
    if(data[0] == 0x00)
    {
        if(data[1] == 0x00)
            cSys_Switch(SO_KEY, ST_OFF, false);
        else if (data[1] == 0x01)
            cSys_Switch(SO_KEY,ST_ON, false);
        c_print_data_trans(0x02, data, 2);  
    }

    return true;
}




/***********************************************************************************************************************
 * 函数功能    : 回复系统概览参数  0x08
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败
 ************************************************************************************************************************/
s8 c_relay08_param(void)
{
    #pragma pack(1)
    struct
    {
		u8				ucSysDevState;
		u16				usVolt;
		s16				sTotalCurr;
		u8				ucChgPerm;
		u8				ucDisChgPerm;
		u8				ucUsbDevState;
		u8				ucLightDevState;
		u8				ucDcacDisChgState;
		u8				ucDcDevState;
		u8				ucDcacChgState;
		u8				ucMpptDevState;
		u8				ucBmsSoc;
    } t_frame = {0};
    #pragma pack()

    t_frame.ucSysDevState = (u8)tSysInfo.eDevState;

    #if(boardBMS_EN)
    t_frame.usVolt     = tBmsRx.tDevInfo[0].usVolt;
    t_frame.sTotalCurr = tBmsRx.sTotalCurr;
    t_frame.ucBmsSoc   = ucBms_GetSoc();
    #endif  //boardBMS_EN

    t_frame.ucChgPerm    = tSysInfo.uPerm.tPerm.bChgPerm;
    t_frame.ucDisChgPerm = tSysInfo.uPerm.tPerm.bDisChgPerm;

    #if(boardUSB_EN)
    t_frame.ucUsbDevState = (u8)tUsb.eDevState;
    #endif  //boardUSB_EN

    #if(boardLIGHT_EN)
    t_frame.ucLightDevState = (u8)tLight.eDevState;
    #endif  //boardLIGHT_EN

    #if(boardDCAC_EN)
    t_frame.ucDcacDisChgState = (u8)tDcac.eDisChgState;
    t_frame.ucDcacChgState    = (u8)tDcac.eChgState;
    #endif  //boardDCAC_EN

    #if(boardDC_EN)
    t_frame.ucDcDevState = (u8)tDc.eDevState;
    #endif  //boardDC_EN

    #if(boardMPPT_EN)
    t_frame.ucMpptDevState = (u8)tMppt.eDevState;
    #endif  //boardMPPT_EN

    return c_print_data_trans(0x08, (u8*)&t_frame, sizeof(t_frame));
}

/***********************************************************************************************************************
 * 函数功能    : 回复系统任务参数  0x14
 * 说明(备注)  : 包含系统基础状态以及任务调试信息
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败   
 ************************************************************************************************************************/
s8 c_relay14_sysinfo_param(void)
{
    #pragma pack(1)
    struct
    {
        // 系统任务基础信息 (31 字节)
        struct
        {
			u8			ucDevState;			// 设备状态
			u16			usErrCode;			// 错误代码
			u8			ucPerm;				// 许可
			u16			usInit;				// 初始化完成标志
			u8			ucPowerType;		// 系统供电类型
            struct
            {
				u16		usMPPT;				// MPPT充电功率
				u16		usDCAC;				// DCAC充电功率
			}tSetChgPwr;
			u16			usAutoOffCnt;		// 自动关闭计时
			u16			usAutoOffTime;		// 自动关闭时间
			u16			usNeedSleepCnt;		// 需要休眠计时
			s16			sMaxTemp;			// 整机最高温
			s16			sMinTemp;			// 整机最低温
			s16			sBoardTempMax;		// 板载最高温
			u16			usVoltMax;			// 最高电压
			u16			usVoltMin;			// 最低电压
			u16			usOutPwr;			// 输出功率
			u16			usInPwr;			// 输入功率
		}tSysInfo;

        // 任务队列调试状态 (10 字节)
		TaskDebug_T		tTask;
    } t_frame = {0};
    #pragma pack()

    t_frame.tSysInfo.ucDevState        = (u8)tSysInfo.eDevState;
    t_frame.tSysInfo.usErrCode         = tSysInfo.uErrCode.usCode;
    t_frame.tSysInfo.ucPerm            = tSysInfo.uPerm.ucPerm;
    t_frame.tSysInfo.usInit            = tSysInfo.uInit.State;
    t_frame.tSysInfo.ucPowerType       = (u8)tSysInfo.ePowerType;
    t_frame.tSysInfo.tSetChgPwr.usMPPT = tSysInfo.tSetChgPwr.usMPPT;
    t_frame.tSysInfo.tSetChgPwr.usDCAC = tSysInfo.tSetChgPwr.usDCAC;
    t_frame.tSysInfo.usAutoOffCnt      = tSysInfo.usAutoOffCnt;
    t_frame.tSysInfo.usAutoOffTime     = tSysInfo.usAutoOffTime;
    t_frame.tSysInfo.usNeedSleepCnt    = tSysInfo.usNeedSleepCnt;
    t_frame.tSysInfo.sMaxTemp          = tSysInfo.sMaxTemp;
    t_frame.tSysInfo.sMinTemp          = tSysInfo.sMinTemp;
    t_frame.tSysInfo.sBoardTempMax     = tSysInfo.sBoardTempMax;
    t_frame.tSysInfo.usVoltMax         = tSysInfo.usVoltMax;
    t_frame.tSysInfo.usVoltMin         = tSysInfo.usVoltMin;
    t_frame.tSysInfo.usOutPwr          = tSysInfo.usOutPwr;
    t_frame.tSysInfo.usInPwr           = tSysInfo.usInPwr;

    if(tpSysTask != NULL)
        memcpy(&t_frame.tTask, (u8*)tpSysTask, sizeof(TaskDebug_T));

    return c_print_data_trans(0x14, (u8*)&t_frame, sizeof(t_frame));
}

/***********************************************************************************************************************
 * 函数功能    : 数据传输
 * 说明(备注)  : none
 * 传入参数    : cmd: 指令
 *               data: 指向数据指针
 *               len: 数据的长度
 * 输出参数    : none
 * 返回值      : -1: 写入的Len超出最大长度, -2: 等待回复超时, -3: 数据发送错误, 0: 无操作, 1: 操作成功
 ************************************************************************************************************************/
static s8 c_print_data_trans(u8 cmd, u8 *data, u8 len)
{
	s8 result = 0;
	
	if (tpPrintProtoTx == NULL)
		return 0;
	
	#if (boardPRINT_IFACE)
	result = cBaiku_ProtoCreate(tpPrintProtoTx, cmd, data, len);
	if (result > 0)
	{
		lwrb_write(&tPrintTxBuff, tpPrintProtoTx->ucaFrameData, tpPrintProtoTx->ucFrameLen);
		return bPrint_SendDataToUsart();
	}
	#endif  /* boardPRINT_IFACE */
	return result;
}

#endif  //boardPRINT_IFACE

