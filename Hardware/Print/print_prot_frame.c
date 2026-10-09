

#include "Print/print_prot_frame.h"

#if(boardPRINT_IFACE)
#include "Print/print_iface.h"
#include "Print/print_task.h"
// #include "Adc/adc_task.h"
#include "Key/key_task.h"
#include "Sys/sys_task.h"
// #include "Updata/updata_task.h"

#if(boardBMS_IFACE)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_IFACE

// #include "app_info.h"

#if(boardUSE_OS)
#include "freertos/freertos.h"
#include "freertos/task.h"
#endif  //boardUSE_OS

#define     	printDEV_ADRR							printCONSOLE_MASTER_ADDR
#define    		printWAIT_NOTIFY_OUTTIME				1000     //任务通知超时时间 MS
#define       	printTX_FRAME_SIZE                     	256
#define       	printRX_FRAME_SIZE                     	256
														
//****************************************************参数初始化**************************************************//
__ALIGNED(4) 	BaikuProtoTx_t *tpPrintProtoTx = NULL;	//发送协议
__ALIGNED(4) 	BaikuProtoRx_t *tpPrintProtoRx = NULL;

vu8 uc_next_cmd = 0;

#pragma pack(1) // 设置对齐为1字节
typedef struct {
    char a;    // 1 byte
    int b;     // 4 bytes (在32位系统上)
    short c;   // 2 bytes
} MyStruct_Packed;
#pragma pack() // 恢复默认对齐方式 (可选)


//****************************************************函数声明****************************************************//
static s8 c_print_data_trans(u8 cmd, u8* data, u8 len);
static bool b_get_console_ver_info(u8* data, u8* data_len);
static bool b_relay_console_info(BaikuProtoRx_t* proto);


/***********************************************************************************************************************
-----函数功能    通讯协议初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bPrint_SendProtInit(void)
{
	s8 c_result = cBaiku_ProtoSendInit(&tpPrintProtoTx,//协议指针
								printTX_FRAME_SIZE, 	//协议缓存器大小
								printDEV_ADRR);			//协议设备ID
	if(c_result <= 0)
		return false;
	
	return true;
}

bool bPrint_RecProtInit(void)
{
	s8 c_result = cBaiku_ProtoRecInit(&tpPrintProtoRx, 	//协议指针
								printRX_FRAME_SIZE,		//协议缓存器大小
								printDEV_ADRR,			//协议设备ID
								boardREPET_TIMER_CYCLE_TMIE);//计数器采样时间
	if(c_result <= 0)
		return false;
	
	return true;
}




/***********************************************************************************************************************
-----函数功能    持续回复
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
bool b_cycle_relay_data(void)
{
	static vu16 us_delay_cnt = 0;
	
	if(uc_next_cmd != 0x0D)
	{
		us_delay_cnt = 0;
		return false;
	}
	
	us_delay_cnt++;
	if(us_delay_cnt >= (100/printTASK_CYCLE_TIME))
	{
		us_delay_cnt = 0;
	}
	return true;
}

/***********************************************************************************************************************
-----函数功能    回复模块开关结果  0x02
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool b_relay02_switch_result(uint8_t data[])
{
    if(data[0] == 0x00)
    {
        if(data[1] == 0x00)
        {
            cSys_Switch(SO_KEY, ST_OFF, false);
        }  
        else if (data[1] == 0x01)
        {
            cSys_Switch(SO_KEY,ST_ON, false);
        }
        c_print_data_trans(0x02, data, 2);  
    }

    return true;
}




/***********************************************************************************************************************
-----函数功能    回复参数  0x08
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
bool b_relay08_param(void)
{
	uint8_t data[20] = {0};
    uint8_t len = 0;

	data[0] = tSysInfo.eDevState;

	#if(boardBMS_IFACE)
	memcpy((u8*)&data[1], (u8*)&tBmsRx.tParam.tDevInfo[0].usVolt,2);
	memcpy((u8*)&data[3], (u8*)&tBmsRx.tParam.sTotalCurr,2);
	#endif  //boardBMS_IFACE

	data[5] = tSysInfo.uPerm.tPerm.bChgPerm;
	data[6] = tSysInfo.uPerm.tPerm.bDisChgPerm;

	#if(boardBMS_IFACE)
	data[13] = ucBms_GetSoc();
	#endif  //boardBMS_IFACE

    len = 14;
    //len不可以超过data的最大长度
    return c_print_data_trans(0x08, data, len);
}


/***********************************************************************************************************************
-----函数功能    回复电池参数  0x0A
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
bool b_relay0A_bat_param(void)
{
	uint8_t data[255] = {0};
    uint8_t len = 0;
  
	#if(boardBMS_IFACE)
	tBmsRx.tParam.usSOC = 12;
	tBmsRx.tParam.sTotalCurr = 23;
	tBmsRx.tParam.usChgFullTime = 45;
	
    len = sizeof(tBms); 
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(data, (u8*)&tBms, sizeof(tBms));

    len += sizeof(tBmsRx.tParam); 
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(&data[len - sizeof(tBmsRx.tParam)], (u8*)&tBmsRx.tParam, sizeof(tBmsRx.tParam));
	
	len += 10; 
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(&data[len - 10], (u8*)tpBmsTask, 10);
    #endif  //boardBMS_IFACE

    //len不可以超过data的最大长度
    return c_print_data_trans(0x0A, data, len);
}


/***********************************************************************************************************************
-----函数功能    回复系统任务参数  0x14
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败   
************************************************************************************************************************/
bool b_relay14_sysinfo_param(void)
{
    uint8_t data[50] = {0};
    uint8_t len = 0;

    len = sizeof(tSysInfo); 
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(data, (u8*)&tSysInfo, sizeof(tSysInfo));
	
	len += 10; 
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(&data[len - 10], (u8*)tpSysTask, 10);
	
    //发送失败
    return c_print_data_trans(0x14, data, len);
}

/***********************************************************************************************************************
-----函数功能    回复系统任务参数  0x14
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败   
************************************************************************************************************************/
bool b_relay40_set_chg_pwr(BaikuProtoRx_t* proto)
{
    if(proto->ucValidLen != 3 || proto->ucpValidData == NULL)
		return false;
	
	#pragma pack(1)
	struct
	{
		u8 module;
		u16 pwr;
	}t_chg_pwr;
	#pragma pack()
	
	memcpy((u8*)&t_chg_pwr, proto->ucpValidData, proto->ucValidLen);
	
	if(t_chg_pwr.module != 0 && t_chg_pwr.module!= 1)
		return false;
	
	return c_print_data_trans(baikuCMD_REPLY_SET_CHG_PWR, (u8*)&t_chg_pwr, sizeof(t_chg_pwr));
}

/***********************************************************************************************************************
-----函数功能    回复系统任务参数  0x14
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败   
************************************************************************************************************************/
bool b_relay44_cali(BaikuProtoRx_t* proto)
{
	u16 us_temp = 0;
	
	if(proto->ucValidLen != 1 || proto->ucpValidData == NULL)
		return -1;
	
	us_temp = 0;
	// if(proto->ucpValidData[0] == 0)
	// 	cQueue_AddQueueTask(tpBmsTask, BTI_CALI, us_temp, false);
	
	return true;
}

/***********************************************************************************************************************
-----函数功能    回复系统任务参数  0x14
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败   
************************************************************************************************************************/
bool b_relay45_cali(u16 temp)
{
	c_print_data_trans(baikuCMD_REPLY_CALI, (u8*)&temp, sizeof(temp));
	
	return true;
}

/***********************************************************************************************************************
-----函数功能    回复记忆的参数  0x80
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败   
************************************************************************************************************************/
bool b_relay80_get_mem_param(BaikuProtoRx_t* proto)
{
	u8 uc_mode = proto->ucpValidData[0];
	
	if(proto->ucValidLen != 3 || proto->ucpValidData == NULL)
		return false;
	
	if(uc_mode != MO_BMS && uc_mode != MO_CONSOLE)
		return false;
	
	switch(uc_mode)
	{
		case MO_CONSOLE://主控
		{
			b_relay_console_info(proto);
		}
		break;
		
		case MO_BMS://电池包
		{
			TaskInParam_U u_in_param;
			memcpy(&u_in_param.usTaskInParam, &proto->ucpValidData[1], 2);
			// cQueue_AddQueueTask(tpBmsTask, BTI_GET_INFO, u_in_param.usTaskInParam, false);
		}
		break;
	}
	
	return true;
}

/***********************************************************************************************************************
-----函数功能    写入记忆参数信息  0x14
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败   
************************************************************************************************************************/
bool b_relay82_write_mem_info(BaikuProtoRx_t* proto)
{
	u8 temp = 0;
	
	//数据长度不对
// 	if(sizeof(tAppMemParam) != proto->ucValidLen)
// 	{
// 		temp = 0xff;
// 		c_print_data_trans(baikuCMD_REPLY_WRITE_MEM_PARAM, &temp, 1);
// 		return 1;
// 	}
// 	//写入参数
// //	bApp_MemParamUpdata(proto->ucpValidData,proto->ucValidLen, true);
// 	temp = 0x00;
// 	c_print_data_trans(baikuCMD_REPLY_WRITE_MEM_PARAM, &temp, 1);
	
	return true;
}

/***********************************************************************************************************************
-----函数功能    回复print状态  0x87
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
bool b_relay84_set_print_state(BaikuProtoRx_t* proto)
{
	u8 temp = 0;
	u8 len = sizeof(uPrint) + 1;
	u8 obj = proto->ucpValidData[0];
	
	if(proto->ucValidLen != len || obj != 0 || proto->ucpValidData == NULL)
	{
		temp = 0xFF;
		c_print_data_trans(baikuCMD_REPLY_SET_PRINT_STATE, &temp, 1);
		return false;
	}

	memcpy((u8*)&uPrint.ulFlag, &proto->ucpValidData[1], len -1);
	temp = 0x00;
	c_print_data_trans(baikuCMD_REPLY_SET_PRINT_STATE, &temp, 1);
	
	return true;
}

/***********************************************************************************************************************
-----函数功能    回复print状态  0x87
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
bool b_relay86_get_print_state(BaikuProtoRx_t* proto)
{
	uint8_t data[5] = {0};
    uint8_t len = 0;
	u8 obj = proto->ucpValidData[0];
	
	if(proto->ucValidLen != 1 || obj != 0 ||  proto->ucpValidData == NULL)
		return false;
	
	len = 1;
	data[0] = obj;
	
    len += sizeof(uPrint); 
    if(len > sizeof(data)) return false;//data长度不足    
    memcpy(&data[1], (u8*)&uPrint, sizeof(uPrint));

    return c_print_data_trans(baikuCMD_REPLY_PRINT_STATE, data, sizeof(data));
}

/***********************************************************************************************************************
-----函数功能    回复print状态  0x87
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
bool b_relay88_sys_set(BaikuProtoRx_t* proto)
{
// 	u8 temp = 0;
// 	Param_T tparam;
			
// 	if(proto->ucValidLen != sizeof(tparam))
// 	{
// 		temp = 0xFF;
// 		c_print_data_trans(baikuCMD_REPLY_SYS_SET, &temp, 1);
// 		return false;
// 	}
	
// 	memcpy(&tparam, proto->ucpValidData, proto->ucValidLen);
// 	//主控
// 	if(tparam.obj == UO_DEFAULT ||
// 		tparam.obj == UO_CONSOLE)
// 	{
// 		temp = 0x00;
// 		c_print_data_trans(baikuCMD_REPLY_SYS_SET, &temp, 1);
		
// 		// vApp_JumpToBoot(tparam.cmd);
// 	}
// 	//BMS
// 	else if(tparam.obj == UO_BMS)
// 	{
// 		TaskInParam_U u_in_param;
// 		u_in_param.tTaskParam.ucObj = UO_BMS;
		
// 		if(tparam.cmd == mainINIT_FINISH_FLAG)
// 			u_in_param.tTaskParam.ucParam = 0;
// //				else if(tparam.cmd == mainOTA_FLAG)
// //					u_in_param.tTaskParam.ucParam = 1;
// 		else if(tparam.cmd == mainUPDATA_FLAG)
// 			u_in_param.tTaskParam.ucParam = 2;
// 		else if(tparam.cmd == mainDISPLAY_FLAG)
// 			u_in_param.tTaskParam.ucParam = 3;
// 		else if(tparam.cmd == mainLOW_POWER_FLAG)
// 			u_in_param.tTaskParam.ucParam = 4;
// 		else if(tparam.cmd == mainINIT_APP_PARAM_FLAG)
// 			u_in_param.tTaskParam.ucParam = 5;
// 		else 
// 			return false;
		
// 		// cQueue_AddQueueTask(tpBmsTask, BTI_REQ_SET_CMD, u_in_param.usTaskInParam, false);//升级
// 	}
	
	return true;
}

/***********************************************************************************************************************
-----函数功能    回复APP信息
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
bool b_relay_bms_app_info(u16 len)
{
	if(len > 256)
		return false;
	
	return c_print_data_trans(baikuCMD_REPLY_MEM_PARAM, ucaPrintTaskInParam, len);
}






/***********************************************************************************************************************
-----函数功能    回复APP信息
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:成功   false:失败
************************************************************************************************************************/
static bool b_relay_console_info(BaikuProtoRx_t* proto)
{
	u8 uc_mode = 0;
	u8 uc_index = 0;
	u8 uc_obj = 0;
	u8 len = 0;
	s8 c_ret = 0;
	u8 buff[255] = {0};
	
	uc_mode = proto->ucpValidData[0];
	uc_index = proto->ucpValidData[1];
	uc_obj = proto->ucpValidData[2];
	
	if(proto->ucValidLen != 3 || proto->ucpValidData == NULL)
		return false;
	
	if(uc_mode != MO_CONSOLE)
		return false;
	
	memcpy(buff, proto->ucpValidData, 3);
	
	// switch(uc_index)
	// {
	// 	case 0://SYS
	// 	{
	// 		//获取版本信息
	// 		if(uc_obj == 1)
	// 		{
	// 			len = sizeof(tAppMemParam.tVerInfo);
	// 			if(len > 245)
	// 				return false;
				
	// 			if(b_get_console_ver_info(&buff[3], &len) == false)
	// 				goto get_mem_param_fail;
	// 		}
	// 		else
	// 			goto get_mem_param_fail;
	// 	}
	// 	break;
	// }
	
	// c_ret = c_print_data_trans(baikuCMD_REPLY_MEM_PARAM, buff, len + 3);
	// if(c_ret <= 0)
	// {
	// 	get_mem_param_fail:
	// 	return false;
	// }

	return true;
}

/***********************************************************************************************************************
-----函数功能    获取BMS的版本信息
-----说明(备注)  data指向的数组长度要足够,要不会溢出
-----传入参数    none
-----输出参数    none
-----返回值      true:成功   false:失败
************************************************************************************************************************/
static bool b_get_console_ver_info(u8* data, u8* data_len)
{
	u8 len = 0;
	u8 char_len = 0;
	char temp[] = "\r\n";
	char ver_temp[] = boardHARDWARE_VERSION;
	
// 	char_len = sizeof(tAppMemParam.tVerInfo.saVersion);
//     len += char_len; 
// //    if(len > sizeof(data)) return false;//data长度不足    
//     memcpy(&data[len - char_len], (u8*)&tAppMemParam.tVerInfo.saVersion, char_len);
	
// 	char_len = sizeof(temp);
// 	len += char_len;
// //    if(len > sizeof(data)) return false;//data长度不足
//     memcpy(&data[len - char_len], (u8*)temp, char_len);
	
// 	char_len = sizeof(ver_temp);
// 	len += char_len;
// //    if(len > sizeof(data)) return false;//data长度不足
//     memcpy(&data[len - char_len], (u8*)ver_temp, char_len);
	
// 	char_len = sizeof(temp);
// 	len += char_len;
// //    if(len > sizeof(data)) return false;//data长度不足
//     memcpy(&data[len - char_len], (u8*)temp, char_len);
	
// 	*data_len = len;
	return true;
}




/***********************************************************************************************************************
-----函数功能	数据传输
-----说明(备注) 
-----传入参数	cmd:指令
				data:指向数据指针
				len:数据的长度
-----输出参数	none
-----返回值		-1:写入的Len超出最大长度
				-2:等会回复超时
				-3:数据发送错误
				0:无操作
				1:操作成功
************************************************************************************************************************/
static s8 c_print_data_trans(u8 cmd, u8* data, u8 len)
{
	s8 result = 0;
	
	if(tpPrintProtoTx == NULL)
		return 0;
	
	#if(boardPRINT_IFACE)
	result = cBaiku_ProtoCreate(tpPrintProtoTx, cmd, data, len);
	if(result > 0)
	{
		lwrb_write(&tPrintTxBuff, tpPrintProtoTx->ucaFrameData, tpPrintProtoTx->ucFrameLen);
			return bPrint_SendDataToUsart();
	}
	#endif
	return result;
}

#endif  //boardPRINT_IFACE
