

#include "MD_Bms/md_bms_prot_frame.h"

#if(boardBMS_IFACE)
#include "MD_Bms/md_bms_iface.h"
#include "Print/print_task.h"
// #include "Updata/updata_task.h"

#if(boardUSE_OS)
#include "freertos/freertos.h"
#include "freertos/task.h"
#endif  //boardUSE_OS

#define     	bmsDEV_ADRR								0x01
#define    		bmsWAIT_NOTIFY_OUTTIME     				1000     //任务通知超时时间 MS


//****************************************************参数初始化**************************************************//
__ALIGNED(4) BaikuProtoTx_t *tpBmsProtoTx = NULL;	//发送协议
__ALIGNED(4) BaikuProtoRx_t *tpBmsProtoRx = NULL;

//升级对象
typedef enum : uint8_t
{
    UO_DEFAULT = 0,		//当前连接设备
    UO_CONSOLE,			//主控
	UO_BMS,				//BMS
}UpdataObj_E;

#pragma pack(1)
typedef struct 
{
	u8 	obj;
	u32	cmd;
}Param_T;
#pragma pack()

//****************************************************函数声明****************************************************//
static s8 c_bms_data_trans(u8 cmd, u8* data, u8 len);



/***********************************************************************************************************************
-----函数功能    通讯协议初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bBms_SendProtInit(void)
{
	s8 c_result = cBaiku_ProtoTransInit(&tpBmsProtoTx, 128, bmsDEV_ADRR);
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
			log_e("bBmsTask:tpBmsProtoTx协议对象初始化失败,代码%d",c_result);
		
		return false;
	}
	
	return true;
}

bool bBms_RecProtInit(void)
{
	s8 c_result = cBaiku_ProtoRecInit(&tpBmsProtoRx, 	//协议指针
								256,			//协议缓存器大小
								bmsDEV_ADRR,	//协议设备ID
								boardREPET_TIMER_CYCLE_TMIE);			//计数器采样时间
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bBmsRecTask || uPrint.tFlag.bImportant)
			log_e("bBmsRecTask:tpBmsProtoRx协议对象初始化失败,代码%d",c_result);
		return false;
	}
	
	return true;
}







/*****************************************************************************************************************
-----函数功能    指令:获取参数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_bms_cs_get_param(u8 num)
{
	return c_bms_data_trans(baikuCMD_GET_PARAM, &num, 1);
}

/*****************************************************************************************************************
-----函数功能    指令:开关BMS
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_bms_cs_switch(TaskInParam_U u_in_param)
{
	return c_bms_data_trans(baikuCMD_SWITCH,(u8*)&u_in_param.usTaskInParam,2);
}

/*****************************************************************************************************************
-----函数功能    指令:校准
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_bms_cs_set_cali(u8 num)
{
	return c_bms_data_trans(baikuCMD_CALI, &num, 1);
}

/*****************************************************************************************************************
-----函数功能    指令:获取APP信息
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_bms_cs_get_app_info(u16 num)
{
	u8 buff[3] = {0};
	
	buff[0] = MO_BMS;
	memcpy(&buff[1], (u8*)&num, 2);
	
	return c_bms_data_trans(baikuCMD_GET_MEM_PARAM, buff, sizeof(buff));
}

/*****************************************************************************************************************
-----函数功能    指令:获取参数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_bms_cs_req_updata(u8 num)
{
	Param_T tparam;
	tparam.obj = UO_BMS;
	
	if(num == 0)
		tparam.cmd = mainINIT_FINISH_FLAG;
//	else if(num == 1)
//		tparam.cmd = mainOTA_FLAG;
	else if(num == 2)
		tparam.cmd = mainUPDATA_FLAG;
	else if(num == 3)
		tparam.cmd = mainDISPLAY_FLAG;
	else if(num == 4)
		tparam.cmd = mainLOW_POWER_FLAG;
	else if(num == 5)
		tparam.cmd = mainINIT_APP_PARAM_FLAG;
	
	return c_bms_data_trans(baikuCMD_SYS_SET, (u8*)&tparam, sizeof(tparam));
}

/*****************************************************************************************************************
-----函数功能    指令:请求充电
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_bms_cs_req_chg(void)
{
	u8 obj = 0;
	return c_bms_data_trans(baikuCMD_REQ_CHG, &obj, 1);
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
static s8 c_bms_data_trans(u8 cmd, u8* data, u8 len)
{
	s8 result = 0;
	
	if(tpBmsProtoTx == NULL)
		return 0;
	
	#if(boardBMS_IFACE)
	result = cBaiku_ProtoCreate(tpBmsProtoTx, cmd, data, len);
	if(result > 0)
	{
		if(bBms_DataSendStart(tpBmsProtoTx->ucaFrameData, tpBmsProtoTx->ucFrameLen) == true)
		{
			//等待任务通知,等待时间为1S
			#if(boardUSE_OS)
			if(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(bmsWAIT_NOTIFY_OUTTIME)) <= 0)
			{
				if(uPrint.tFlag.bBmsTask)
					log_w("bBmsTask:等待指令0x%x回复超时",cmd);
				
				return result = -2;
			}
			#endif  //boardUSE_OS
		}
		else 
			result = -3;
	}
	#endif
	return result;
}

#endif  //boardBMS_IFACE
