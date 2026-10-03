
#include "MD_Mppt/md_mppt_prot_frame.h"

#if(boardMPPT_IFACE)
#include "MD_Mppt/md_mppt_rec_task.h"
#include "MD_Mppt/md_mppt_queue_task.h"
#include "MD_Mppt/md_mppt_task.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "MD_Dcac/md_dcac_iface.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#include "Print/print_task.h"

#include "check.h"


#define       	mpptDEV_ADRR                          	0x02
#define  		mpptWAIT_NOTIFY_OUTTIME              	1000     //任务通知超时时间 MS
#define       	mpptTX_PROTO_BUFF_LEN                   128
#define       	mpptRX_PROTO_BUFF_LEN                   256


//****************************************************参数初始化**************************************************//
__ALIGNED(4) 	BaikuProtoTx_t *tpMpptProtoTx = NULL;	//发送协议
__ALIGNED(4) 	BaikuProtoRx_t *tpMpptProtoRx = NULL;	//接受协议

//****************************************************函数声明****************************************************//
static s8 c_mppt_data_trans(u8 cmd, u8* data, u8 len);



/***********************************************************************************************************************
-----函数功能    通讯协议初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bMppt_SendProtInit(void)
{
	s8 c_result = 1;

	c_result = cBaiku_ProtoTransInit(&tpMpptProtoTx, mpptTX_PROTO_BUFF_LEN, mpptDEV_ADRR);
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bMpptTask || uPrint.tFlag.bImportant)
			log_e("bMpptTask:tpMpptProtoTx协议对象初始化失败,代码%d",c_result);
		
		return false;
	}
	
	return true;
}

bool bMppt_RecProtInit(void)
{
	s8 c_result = cBaiku_ProtoRecInit(&tpMpptProtoRx, 	//协议指针
								mpptRX_PROTO_BUFF_LEN,	//协议缓存器大小
								mpptDEV_ADRR,			//协议设备ID
								boardREPET_TIMER_CYCLE_TMIE);			//计数器采样时间
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bMpptRecTask || uPrint.tFlag.bImportant)
			log_e("bMpptRecTask:tpMpptProtoRx协议对象初始化失败,代码%d",c_result);
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
s8 c_mppt_cs_switch(TaskInParam_U u_param)
{
	return c_mppt_data_trans(baikuCMD_SWITCH, (u8*)&u_param.usTaskInParam, 2);
}

/*****************************************************************************************************************
-----函数功能    指令:获取参数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_mppt_cs_get_param(u8 num)
{
	return c_mppt_data_trans(baikuCMD_GET_PARAM, &num, 1);
}

/*****************************************************************************************************************
-----函数功能    指令:开关MPPT
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_mppt_cs_set_pwr(u16 pwr)
{
	u8 buff[3] = {0};

	buff[0] = 0;
	buff[1] = pwr;
	buff[2] = pwr >> 8;
	return c_mppt_data_trans(baikuCMD_GET_PARAM, buff, sizeof(buff));
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
static s8 c_mppt_data_trans(u8 cmd, u8* data, u8 len)
{
	s8 result = 0;
	
	if(dcacSemaphoreMutex == NULL)
		return 0;
	
	if(tpMpptProtoTx == NULL)
		return 0;
	
	//开始互斥
	#if(boardUSE_OS)
	if(xSemaphoreTake(dcacSemaphoreMutex, pdMS_TO_TICKS(1000)) == pdFAIL)
		return -99;
	#endif  //boardUSE_OS
	
	//开始发送
	#if(boardDCAC_IFACE)
	result = cBaiku_ProtoCreate(tpMpptProtoTx, cmd, data, len);
	if(result > 0)
	{
		bMpptUseFlag = 1;
		if(bDcac_DataSendStart(tpMpptProtoTx->ucaFrameData, tpMpptProtoTx->ucFrameLen) == true)
		{
			//等待任务通知,等待时间为1S
			#if(boardUSE_OS)
			if(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(mpptWAIT_NOTIFY_OUTTIME)) <= 0) 
			{
				if(uPrint.tFlag.bMpptTask)
					log_w("bMpptTask:等待指令0x%x回复超时", cmd);
				
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

#endif  //boardMPPT_IFACE
