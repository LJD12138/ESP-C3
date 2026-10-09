#include "MD_Mppt/md_mppt_rec_data_proc.h"

#if(boardMPPT_IFACE)
#include "MD_Mppt/md_mppt_rec_task.h"
#include "MD_Mppt/md_mppt_prot_frame.h"
#include "MD_Mppt/md_mppt_task.h"
#include "Print/print_task.h"

#include "app_info.h"
#include "function.h"

//****************************************************参数初始化**************************************************// 

//****************************************************函数声明****************************************************//
static s8 c_relay08_param(BaikuProtoRx_t* proto);


/***********************************************************************************************************************
-----函数功能    处理接收到的数据
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      0:没有错误  其他有错误
************************************************************************************************************************/
s8 c_mppt_rec_proc_data(BaikuProtoRx_t* proto)
{
	s8 c_ret = 0;

	if(uPrint.tFlag.bMpptRecTask)
	{
		sMyPrint("\r\n bMpptRecTask:指令:0x%x, 数据:",proto->ucCmd);
		for(int i = 0; i < proto->ucValidLen; i++)
			sMyPrint("%x ",proto->ucpValidData[i]);
		sMyPrint("\r\n");
	}
	
	switch (proto->ucCmd)
	{
		//回复开关
        case baikuCMD_REPLY_SWITCH:
        {
			if(proto->ucValidLen != 2 || proto->ucpValidData == NULL)
				return -1;

			lwrb_reset(&tMpptReplyBuff);
			lwrb_write(&tMpptReplyBuff, proto->ucpValidData, proto->ucValidLen);

			#if(boardUSE_OS)
			xTaskNotifyGive(tMpptTaskHandler);//通知发送任务
			#endif  //boardUSE_OS
        }
        break;

		//回复充电功率
		case baikuCMD_REPLY_SET_CHG_PWR:
        {
			if(proto->ucValidLen != 3 || proto->ucpValidData == NULL)
				return -1;

			lwrb_reset(&tMpptReplyBuff);
			lwrb_write(&tMpptReplyBuff, proto->ucpValidData, proto->ucValidLen);

			#if(boardUSE_OS)
			xTaskNotifyGive(tMpptTaskHandler);//通知发送任务
			#endif  //boardUSE_OS
        }
        break;
		
		//回复参数
		case baikuCMD_REPLY_PARAM:                
        {
			c_ret = c_relay08_param(proto);
			if(c_ret <= 0)
				return -2;

			#if(boardUSE_OS)
			xTaskNotifyGive(tMpptTaskHandler);//通知发送任务
			#endif  //boardUSE_OS
        }
        break;
		
		default:
			return -9;
	}
    return 1;
}


/***********************************************************************************************************************
-----函数功能    回复参数  0x08
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
static s8 c_relay08_param(BaikuProtoRx_t* proto)
{
	MpptParam_T t_param;
	u8 len = sizeof(t_param);
	u8 type = proto->ucpValidData[0];
	
	if(proto->ucValidLen != (len + 1) || proto->ucpValidData == NULL)
		return -1;
	
	if(type != 0x02)
		return -2;
	
	memcpy((u8*)&t_param,&proto->ucpValidData[1],len);
	tMppt.eDevState = t_param.eChgState;
	tMpptRx.usInVolt = t_param.usInVolt;
	tMpptRx.usInCurr = t_param.usInCurr * 10;
	tMpptRx.usInPwr = t_param.usInPwr;
	tMpptRx.usOutCurr = t_param.usOutCurr * 10;
	tMpptRx.usOutVolt = t_param.usOutVolt;
	tMpptRx.usOutPwr = t_param.usOutPwr;
	tMpptRx.sMaxTemp = t_param.cMaxTemp;
	tMpptRx.uErrCode.usCode = t_param.ucErrCode;

	if(t_param.eChgState >= IOS_STARTING)
		tMppt.eDevState = DS_WORK;
	else
		tMppt.eDevState = DS_SHUT_DOWN;
	return 1;
}
#endif  //boardMPPT_IFACE
