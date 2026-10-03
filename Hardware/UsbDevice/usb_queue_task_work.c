/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "UsbDevice/usb_queue_task.h"
#include "UsbDevice/usb_task.h"
#include "UsbDevice/usb_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "app_info.h"
#include "filtration.h"

#define       	usbTASK_WORK_CYCLE_TIME               		100

s32 us_usb_total_out_pwr = 0;
vu16 s_total_temp = 0;

//PD100W温度滤波器
// #define 		usbPD_TEMP_FILTER_BUFF_SIZE     		10 
// static s32 usa_pd_temp_buff[usbPD_TEMP_FILTER_BUFF_SIZE];
// FilterHandler_T    tAdc_PDTempFilterMadAvg = {usa_pd_temp_buff, usbPD_TEMP_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

// //无线充温度滤波器
// #define 		usbCHG_TEMP_FILTER_BUFF_SIZE     		10 
// static s32 usa_chg_temp_buff[usbCHG_TEMP_FILTER_BUFF_SIZE];
// FilterHandler_T    tAdc_ChgTempFilterMadAvg = {usa_chg_temp_buff, usbCHG_TEMP_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//USB电压滤波器
#define 		adcUSB_PWR_FILTER_BUFF_SIZE     		4 
static s32 	usa_adc_usb_pwr_buff[adcUSB_PWR_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_UsbPwrFilterMadAvg = {usa_adc_usb_pwr_buff, adcUSB_PWR_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

/*****************************************************************************************************************
-----函数功能    任务函数:初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_usb_queue_task_work(Task_T *tp_task)
{
	//有任务,退出
	if(lwrb_get_full(&tp_task->tQueueBuff))
	{
		cQueue_GotoStep(tp_task, STEP_END);  //结束
		return;
	}
	
	switch (tp_task->ucStep)
    {
		case 0:
        {
			us_usb_total_out_pwr = 0;
			s_total_temp = 0;
			if(c_usb_cs_get_ic1_param() != 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
			else
				break;
        }

		case 1:
        {
			if(c_usb_cs_get_ic2_param() != 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);  	//下一步
			else
				break;
        }

		case 2:
		{
			c_usb_cs_get_ic3_param();

			// if(tBmsRx.tParam.usSOC < 25)
			// 	c_usb_cs_set_chg_pwr(20);
			// else
			// 	c_usb_cs_set_chg_pwr(0);

			// c_usb_cs_ic3_test();

			// vu16 pwr_twmp  = (tUsb.usInCurr * tUsb.usInVolt)  / 100;
			// if(pwr_twmp < 2)
			// 	pwr_twmp = 0;
			// us_usb_total_out_pwr += pwr_twmp;

			tUsb.usOutPwr = lFilter_MadianAverage(&tAdc_UsbPwrFilterMadAvg, &us_usb_total_out_pwr);
			tUsb.sMaxTemp = s_total_temp;
			vTaskDelay(400);
			cQueue_GotoStep(tp_task, 0);
		}
		break;

		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }

	#if(boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, usbTASK_WORK_CYCLE_TIME);
	#endif  //boardUSE_OS
}
