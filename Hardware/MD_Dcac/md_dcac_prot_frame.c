

#include "MD_Dcac/md_dcac_prot_frame.h"

#if(boardDCAC_IFACE)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#include "MD_Dcac/md_dcac_iface.h"
#include "Print/print_task.h"

#include "check.h"
#include "function.h"
#include "app_info.h"



#define       	dcacDEV_ADRR                          	0x01
#define  		dcacWAIT_NOTIFY_OUTTIME              	1000     //任务通知超时时间 MS
#define       	dcacTX_PROTO_BUFF_LEN                   128
#define       	dcacRX_PROTO_BUFF_LEN                   128

//****************************************************参数初始化**************************************************//
__ALIGNED(4) 	BaikuProtoTx_t *tpDcacProtoTx = NULL;	//发送协议
__ALIGNED(4) 	BaikuProtoRx_t *tpDcacProtoRx = NULL;	//接收协议

#pragma pack (1)   //强制进行1字节对齐
struct
{
	vu16 usAcOutSwitch;
	vu16 usBatOV;//0.1V
	vu16 usBatUV;//0.1V
	vu16 usOutFreq;//0:50  1:60
	vu16 usOutVolt;//0:100 1:110 2:120 3:220 4:230 5:240
	vu16 usChgPwr;
	vu16 usPF;
	vu16 usChgVolt;
	vu16 usPvOV;
	vu16 usConstPwr;
	vu16 usSysPwr;
	vu16 ustemp;
	vu16 usParaSwitch;
}tDcacInit;
#pragma pack()   //取消进行1字节对齐

#pragma pack (1)   //强制进行1字节对齐
struct
{
	vu16 usParaOutEn;
	vu16 usParaPwrSet;
}tDcacParaInInit;
#pragma pack()   //取消进行1字节对齐

#if(boardUSE_OS)
/*创建互斥量*/
SemaphoreHandle_t dcacSemaphoreMutex = NULL;
#endif  //boardUSE_OS


//****************************************************函数声明****************************************************//
static s8 c_dcac_data_trans(u8 cmd, u8* data, u8 len);



/***********************************************************************************************************************
-----函数功能    通讯协议初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bDcac_SendProtInit(void)
{
	s8 c_result = 1;

	c_result = cBaiku_ProtoTransInit(&tpDcacProtoTx, dcacTX_PROTO_BUFF_LEN, dcacDEV_ADRR);
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
			log_e("bDcacTask:tpDcacProtoTx协议对象初始化失败,代码%d",c_result);
		
		return false;
	}
	
	#if(boardUSE_OS)
    /* 创建互斥信号量 */
    dcacSemaphoreMutex = xSemaphoreCreateMutex();
	#endif  //boardUSE_OS
	
	return true;
}

bool bDcac_RecProtInit(void)
{
	s8 c_result = cBaiku_ProtoRecInit(&tpDcacProtoRx, 	//协议指针
								256,			//协议缓存器大小
								dcacDEV_ADRR,	//协议设备ID
								boardREPET_TIMER_CYCLE_TMIE);			//计数器采样时间
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bDcacRecTask || uPrint.tFlag.bImportant)
			log_e("bDcacRecTask:tpDcacProtoRx协议对象初始化失败,代码%d",c_result);
		return false;
	}
	
	return true;
}

/*****************************************************************************************************************
-----函数功能    指令:开关
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_dcac_cs_switch(TaskInParam_U u_param)
{
	return c_dcac_data_trans(baikuCMD_SWITCH, (u8*)&u_param.usTaskInParam, 2);
}

/*****************************************************************************************************************
-----函数功能    指令:获取参数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_dcac_cs_get_param(u8 num)
{
	return c_dcac_data_trans(baikuCMD_GET_PARAM, &num, 1);
}


/*****************************************************************************************************************
-----函数功能    指令:设置充电功率
-----说明(备注)  none
-----传入参数    num设置的充电功率
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_dcac_cs_set_total_chg_pwr(u16 pwr)
{
	return false;
}

/*****************************************************************************************************************
-----函数功能    指令:开关BMS
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_dcac_cs_init(void)
{
	return true;
}

/*****************************************************************************************************************
-----函数功能    指令:并网功率设置
-----说明(备注)  none
-----传入参数    num:并网功率
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_dcac_cs_set_para_in_pwr(u16 pwr)
{
	return false;
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
static s8 c_dcac_data_trans(u8 cmd, u8* data, u8 len)
{
	s8 result = 0;
	
	if(dcacSemaphoreMutex == NULL)
		return 0;
	
	if(tpDcacProtoTx == NULL)
		return 0;
	
	//开始互斥
	#if(boardUSE_OS)
	if(xSemaphoreTake(dcacSemaphoreMutex, pdMS_TO_TICKS(1000)) == pdFAIL)
		return -99;
	#endif  //boardUSE_OS
	
	//开始发送
	#if(boardDCAC_IFACE)
	result = cBaiku_ProtoCreate(tpDcacProtoTx, cmd, data, len);
	if(result > 0)
	{
		bMpptUseFlag = 0;
		if(bDcac_DataSendStart(tpDcacProtoTx->ucaFrameData, tpDcacProtoTx->ucFrameLen) == true)
		{
			//等待任务通知,等待时间为1S
			#if(boardUSE_OS)
			if(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(dcacWAIT_NOTIFY_OUTTIME)) <= 0) 
			{
				if((uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant) && tDcac.eDevState != DS_LOST)
					log_w("bDcacTask:等待指令0x%x回复超时",cmd);
				
				result = -2;
			}
			#endif  //boardUSE_OS
		}
		else 
			result = -3;
	}
	#endif
	
	//释放互斥量
	#if(boardUSE_OS)
	xSemaphoreGive(dcacSemaphoreMutex);
	#endif  //boardUSE_OS
	
	return result;
}

#endif  //boardDCAC_IFACE
