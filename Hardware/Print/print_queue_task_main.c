/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Print/print_queue_task.h"

#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"

#define       	printTASK_PARAM_CYCLE_TIME               		50


//****************************************************函数声明****************************************************//
static s8 c_print_rec_proc_data(BaikuProtoRx_t* proto);

/*****************************************************************************************************************
-----函数功能    任务函数:错误处理任务
-----说明(备注)  none
-----传入参数    PRINT_ErrState_N:
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_print_queue_task_main(Task_T *p_task)
{
	s8 c_ret = 0;

	//队列里面有任务
	if (lwrb_get_full(&p_task->tQueueBuff))  
	{
		cQueue_GotoStep(p_task, STEP_END);  //结束
		return;
	}
	
	c_cycle_relay_data();
		
	//******************************************处理发送的数据****************************************************
	if (lwrb_get_full(&tPrintTxBuff))
		bPrint_SendDataToUsart();
	
	//******************************************处理接收的数据****************************************************
	c_ret = cBaiku_ProtoCheck(tpPrintProtoRx);
	if (c_ret > 0)
		c_print_rec_proc_data(tpPrintProtoRx);
	
	switch (p_task->ucStep)
    {
        case 0:
        {
			
        }
		break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);  //结束
		}
		break;
    }
	
	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, printTASK_PARAM_CYCLE_TIME);
	#endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 处理接收到的协议数据
 * 说明(备注)  : 解析协议命令码并调用对应的回复打包函数
 * 传入参数    : proto: 协议接收结构体指针
 * 输出参数    : 无
 * 返回值      : 0: 正常, 其它: 错误码
 ************************************************************************************************************************/
__STATIC_INLINE s8 c_print_rec_proc_data(BaikuProtoRx_t* proto)
{
	if (proto == NULL)
		return 99;
		
	uc_next_cmd = proto->ucCmd;
    switch (proto->ucCmd)
    {
        case baikuCMD_SWITCH:                  //开关回复
        {
            if (proto->ucpValidData != NULL && proto->ucValidLen >= 2)
                c_relay02_switch_result(proto->ucpValidData);
        }
		break;

        case baikuCMD_GET_PARAM:
            c_relay08_param();
			break;

        default:
            break;
    }
    return 0;
}
#endif  //boardPRINT_IFACE

