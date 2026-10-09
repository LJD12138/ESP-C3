#include "MD_Bms/md_bms_rec_data_proc.h"

#if(boardBMS_IFACE)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_task.h"
#include "Print/print_task.h"
// #include "Updata/updata_task.h"

#include "Baiku/baiku_proto.h"

//****************************************************函数声明****************************************************//
static s8 c_relay08_param(BaikuProtoRx_t* proto);


/***********************************************************************************************************************
-----函数功能    处理接收到的数据
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      0:没有错误  其他有错误
************************************************************************************************************************/
s8 c_bms_rec_proc_data(BaikuProtoRx_t* proto)
{
	s8 c_ret = 1;
	vu16 us_temp = 0;
    
	if(uPrint.tFlag.bBmsRecTask)
	{
		sMyPrint("bBmsRecTask:指令:0x%x, 数据:",proto->ucCmd);
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

			lwrb_reset(&tBmsReplyBuff);
			lwrb_write(&tBmsReplyBuff, proto->ucpValidData, proto->ucValidLen);

			#if(boardUSE_OS)
			xTaskNotifyGive(tBmsTaskHandler);//通知发送任务
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
			xTaskNotifyGive(tBmsTaskHandler);//通知发送任务
			#endif  //boardUSE_OS
        }
        break;
		
//		//回复MPPT充电设置
//		case baikuCMD_MPPT_REPLY_CHG_PWR:
//		{
//			if(proto->ucValidLen != 2 || proto->ucpValidData == NULL)
//				return -3;
//			
//			memcpy((u8*)&us_temp, proto->ucpValidData, proto->ucValidLen);
//			
//			if(uPrint.tFlag.bBmsRecTask)
//				sMyPrint("bBmsRecTask:设置MPPT功率返回的数据%d \r\n",us_temp);
//			
//			if(us_temp > tAppMemParam.tMPPT.usInPwrRating)
//				return -4;
//			
//			tMpptRx.tParam.usOutPwr = us_temp;

			// #if(boardUSE_OS)
			// xTaskNotifyGive(tBmsTaskHandler);//通知发送任务
			// #endif  //boardUSE_OS
//		}
//		break;
		
		//回复校准结果
		case baikuCMD_REPLY_CALI://45            
        {
			if(proto->ucValidLen != 2 || proto->ucpValidData == NULL)
				return -3;
			
			memcpy((u8*)&us_temp, proto->ucpValidData, proto->ucValidLen);

			#if(boardPRINT_IFACE)
			cQueue_AddQueueTask(tpPrintTask, PTI_REPLY_CALI, us_temp, false);
			#endif  //boardPRINT_IFACE

			#if(boardUSE_OS)
			xTaskNotifyGive(tBmsTaskHandler);//通知发送任务
			#endif  //boardUSE_OS
        }
        break;
		
		//回复设置结果
		case baikuCMD_REPLY_SYS_SET://89            
        {
            #if(boardUSE_OS)
			xTaskNotifyGive(tBmsTaskHandler);//通知发送任务
			#endif  //boardUSE_OS
        }
        break;
		
		//回复APP信息
		case baikuCMD_REPLY_MEM_PARAM://81
        {
            if(proto->ucValidLen == 0 || proto->ucpValidData == NULL)
				return -3;
			
			#if(boardPRINT_IFACE)
			memcpy(&ucaPrintTaskInParam, proto->ucpValidData, proto->ucValidLen);
			
			cQueue_AddQueueTask(tpPrintTask, PTI_REPLY_APP_INFO, proto->ucValidLen, false);
			#endif  //boardPRINT_IFACE

			#if(boardUSE_OS)
			xTaskNotifyGive(tBmsTaskHandler);//通知发送任务
			#endif  //boardUSE_OS
        }
        break;
		
		//请求开始发送
		case baikuCMD_RRQ_START_SEND://C4               
        {
			// cBaiku_ResetRxBuff(proto);
            // cUpdata_ChSelect(CT_PRINT, UWS_CHECK, PT_BAIKU);

			#if(boardUSE_OS)
			xTaskNotifyGive(tBmsTaskHandler);//通知发送任务
			#endif  //boardUSE_OS
        }
        break;
		
		//BMS正在升级
		case baikuCMD_BMS_UPDATA:                
        {
			if(tpBmsTask->ucID == BTI_REQ_SET_CMD)
				return 0;
			
			if(proto->ucValidLen != 4 || proto->ucpValidData == NULL)
				return -5;
			
			// memcpy((u8*)&tUpdata.usRecFrameCnt, proto->ucpValidData, proto->ucValidLen);
			
            // cUpdata_ChSelect(CT_NULL, UWS_NULL, PT_NULL);

			#if(boardUSE_OS)
			xTaskNotifyGive(tBmsTaskHandler);//通知发送任务
			#endif  //boardUSE_OS
        }
        break;
		
		default:
			c_ret = -2;
            break;
	}
	
   return c_ret; 

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
	u8 len = sizeof(tBmsRx.tParam);
	u8 cmd = proto->ucpValidData[0];
	
	if(proto->ucValidLen != (len + 1) || proto->ucpValidData == NULL)
		return -1;
	
	if(cmd != 0x01)
		return -2;
	
	memcpy((u8*)&tBmsRx.tParam,&proto->ucpValidData[1],len);

	static vu16  last_err_state=0;
	
	tBmsRx.usErrCode = 0;
	for(int i = 0; i < bmsDEV_NUM; i++)
		tBmsRx.usErrCode |= tBmsRx.tParam.tDevInfo[i].uErrCode.usCode;
	
	//----------------------------获取故障位-------------------------------------------------
	if(last_err_state != tBmsRx.usErrCode) 
	{
		last_err_state = tBmsRx.usErrCode;
		if(tBmsRx.usErrCode)
			bBms_SetErrCode(BEC_BMS_ERR,true);
		else 
			bBms_SetErrCode(BEC_BMS_ERR,false);
	}
	
	//----------------------------获取充放电状态-----------------------------------------------
	if(tBmsRx.tParam.sTotalCurr > 0)  //充电状态
		tBms.eWorkState = BWS_CHG;
	else 
		tBms.eWorkState = BWS_DISCHG;
	//sMyPrint("BMS电流%d  充电状态%d  温度 = %d\r\n ",tBmsRx.tParam.sTotalCurr,bBms_GetBmsChgState(),tSysInfo.sMaxTemp);
	
	//----------------------------获取温度-----------------------------------------------
	vs16 s_temp_max = tBmsRx.tParam.tDevInfo[0].sMaxTemp;
	vs16 s_temp_min = tBmsRx.tParam.tDevInfo[0].sMinTemp;
	if(tBmsRx.tParam.tDevNum.ucOnlineNum > 0)
	{
		for(int i = 1; i < (tBmsRx.tParam.tDevNum.ucOnlineNum + 1); i++)
		{
			s_temp_max = MAX2(s_temp_max, tBmsRx.tParam.tDevInfo[i].sMaxTemp);
			s_temp_min = MIN2(s_temp_min, tBmsRx.tParam.tDevInfo[i].sMinTemp);
		}
	}
	tBms.sMaxTemp = s_temp_max;
	tBms.sMinTemp = s_temp_min;
	
	return 1;
}

#endif  //boardBMS_IFACE
