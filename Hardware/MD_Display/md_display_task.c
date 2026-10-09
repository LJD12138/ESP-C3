/*****************************************************************************************************************
*                                                                                                                *
 *                                         Disp显示任务                                                          *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Display/md_display_task.h"

#if(boardDISPLAY)
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_iface.h"
#include "MD_Display/lvgl_ui.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"

#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
// #include "lv_port_tft_touch.h"
// #FF7F27
// #325f9e
#include "ui.h"
#include "ui/images.h"
#include "ui/screens.h"

#if(boardUSE_OS)
#include "freertos/freertos.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#endif  //boardUSE_OS

//****************************************************任务参数初始化**********************************************//
#if(boardUSE_OS)
#define			dispTASK_PRIO                   4       //任务优先级 
#define			dispTASK_STK_SIZE               1024*8    //任务堆栈  实际字节数 *4 (增大堆栈以支持复杂渲染)
TaskHandle_t	tDispTaskHandler = NULL; 
void vDisp_Task(void *pvParameters);
#endif  //boardUSE_OS


//****************************************************参数初始化**************************************************//
Disp_T   tDisp; 


//****************************************************局部函数定义************************************************//
static void v_disp_param_init(void);
static void v_disp_init(void);
static void v_disp_close(void);
static void v_disp_shutdown(void);
static void v_disp_err(void);
static void v_disp_boot(void);
static void v_disp_work(void);


/***********************************************************************************************************************
-----函数功能    Disp显示任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bDisp_TaskInit(void)
{
	v_disp_param_init();

	vDisp_IfaceInit();
	
	#if(boardUSE_OS)
	xTaskCreate((TaskFunction_t )vDisp_Task,			//任务函数
                (const char* )"DispTask",				//任务名称
                (uint16_t ) dispTASK_STK_SIZE,			//任务堆栈大小
                (void* )NULL,							//传递给任务函数的参数
                (UBaseType_t ) dispTASK_PRIO,           //任务优先级
                (TaskHandle_t*)&tDispTaskHandler);      //任务句柄
	#endif  //boardUSE_OS
	
	return true;
}


/***********************************************************************************************************************
-----函数功能    参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_disp_param_init(void)
{
	tDisp.usAutoOffTime = boardDISP_OFF_TIME;
}



/***********************************************************************************************************************
-----函数功能    tDisp显示任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_Task(void *pvParameters)
{
	// 将显示任务从看门狗监控中移除，因为LVGL渲染可能耗时较长
	esp_task_wdt_delete(NULL);
	
	for(;;)
	{
		// LVGL事务处理 - 必须在任务中调用，不能在定时器回调中调用
		lv_task_handler();
		
		tDisp.bSleepShow =true;//待机强制打开亮屏
		
		switch(tSysInfo.eDevState)
		{
			//--------------------------初始化----------------------------------
			case DS_INIT:
			{	
				v_disp_init();
			}
			break;
			
			//---------------------------关闭中---------------------------------
			case DS_CLOSING:
			{
				v_disp_close();
			}
			break;
			
			//---------------------------关闭----------------------------------
			case DS_SHUT_DOWN:
			{
				v_disp_shutdown();
			}
			break;
			
			//---------------------------错误------------------------------------
			case DS_ERR: 
			{
				v_disp_err();
			}
			break;
			
			//----------------------------装载中---------------------------------
			case DS_BOOTING:
			{
				v_disp_boot();
			}
			break;
			
			//----------------------------工作中---------------------------------
			case DS_WORK:
			{
				v_disp_work();
			}
			break;

			#if(boardENG_MODE_EN)
			//----------------------------工程模式---------------------------------
			case DS_ENG_MODE:
			{
				vDisp_EnginModeDis();
				vTaskDelay(400);
				break;
			}
			#endif	
				
			default:
				break;
		}
		vTaskDelay(boardDISP_REFRESH_TMIE);
	}
}

/***********************************************************************************************************************
-----函数功能    显示开关
-----说明(备注)  none
-----传入参数    type:类型   fore_en:强制打开
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bDisp_Switch(SwitchType_E type, bool fore_en)
{
	switch(type)
	{
		case ST_ON:
			goto LoopOn;
		
		case ST_OFF:
			goto LoopOff;
		
		default:
		{
			if(tDisp.bLight == false)
			{
				LoopOn:
				// dispPOWER_OPEN_ON();
				tDisp.bLight = true;
				
				//关闭息屏
				if(fore_en == true)
					tDisp.usAutoOffTime = 0;
				
				//更新显示时间
				if(tDisp.usAutoOffTime)
					tDisp.usAutoOffCnt =  tDisp.usAutoOffTime;
			}
			else 
			{
				LoopOff:
				// dispPOWER_OPEN_OFF();
				v_disp_param_init();
				tDisp.bLight = false;
			}
		}
		break;
	}
	
	return true;
}

/***********************************************************************************************************************
-----函数功能    更新显示参数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_RefreshDisplayParam(void) 
{
	if(tDisp.usAutoOffTime)
	{
		tDisp.usAutoOffCnt =  tDisp.usAutoOffTime;
	}
	bDisp_Switch(ST_ON, false);
}


/***********************************************************************************************************************
-----函数功能    背光自动关闭计时
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_AutoOffCnt(void) 
{
	//非工作状态下退出
	if(tSysInfo.eDevState != DS_WORK) 
		return;
	
	//非亮屏幕状态
	if(tDisp.bLight == false)   
		return;
	
	//-----自动关闭背光--------------------------------------   
	if(tDisp.usAutoOffTime)
	{
		if(tDisp.usAutoOffCnt)
		{
			tDisp.usAutoOffCnt--;
			if(tDisp.usAutoOffCnt == 0)
			{
//				vDisp_ColseDisplay();
				if(uPrint.tFlag.bDispTask|| uPrint.tFlag.bImportant)
					sMyPrint("================Lcd_Task:倒计时结束,进入息屏 时间 = %dS==========\r\n",tDisp.usAutoOffTime);
			}
		}
	}
}

/***********************************************************************************************************************
-----函数功能    初始化显示
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
__STATIC_INLINE void v_disp_init(void)
{
	static vu8 index = 0;
	
	switch(index)
	{
		case 0:
		{
			lv_init();
			lv_port_disp_init();  // lvgl显示接口初始化,放在lv_init()的后面
			lv_port_indev_init(); // lvgl输入接口初始化,放在lv_init()的后面

			ui_init();            // 创建并加载UI屏幕
			lvgl_ui_init();
			index++;
		}
		break;
		
		case 1:
		{
			tSysInfo.uInit.tFinish.bIF_DispTask = true;
		}
		break;
		
		default:
			index = 0;
			break;
	}
}

/***********************************************************************************************************************
-----函数功能    关闭中显示
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
__STATIC_INLINE void v_disp_close(void)
{
	
}

/***********************************************************************************************************************
-----函数功能    关闭显示
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
__STATIC_INLINE void v_disp_shutdown(void)
{
	
}

/***********************************************************************************************************************
-----函数功能    错误显示
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
__STATIC_INLINE void v_disp_err(void)
{
	
}

/***********************************************************************************************************************
-----函数功能    开启显示
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
__STATIC_INLINE void v_disp_boot(void)
{
	vDisp_SetBacklight(false);
}


lv_obj_t* lv_obj;  //屏幕对象
static lv_obj_t* s_gif_obj = NULL;  // 保存GIF对象指针
/***********************************************************************************************************************
-----函数功能    工作显示
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
__STATIC_INLINE void v_disp_work(void)
{
	static vu8 index = 0;
	static vu16  us_delay_cnt = 0;
	static ScreenId_E eCurrScreen;

	switch(index)
	{
		case 0:
		{
			// 直接设置屏幕背景色
			// lv_obj_t* screen = lv_screen_active();
			// lv_obj_set_style_bg_color(screen, lv_palette_main(LV_PALETTE_RED), 0);
			// lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
			// lv_obj_set_style_border_width(screen, 0, 0);
			// lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

			// 创建GIF对象
			// LV_IMAGE_DECLARE(img_gif);
			// s_gif_obj = lv_gif_create(lv_screen_active());
			// lv_obj_set_size(s_gif_obj, 100, 100);
			// lv_gif_set_color_format(s_gif_obj, LV_COLOR_FORMAT_RGB565A8);
			// lv_gif_set_src(s_gif_obj, &img_gif);
			// lv_obj_align(s_gif_obj, LV_ALIGN_CENTER, 0, 30);

			// lv_obj_t *obj = lv_button_create(screen);
            // lv_obj_set_pos(obj, 114, 21);
            // lv_obj_set_size(obj, 100, 50);

			if(us_delay_cnt < 10)
				vDisp_SetBacklight(false);
			else 
				vDisp_SetBacklight(true);

			us_delay_cnt++;
			if(us_delay_cnt < (3000 / boardDISP_REFRESH_TMIE))
				break;
			us_delay_cnt = 0;

			index++;
		}
		break;
		
		case 1:
		{
			// loadScreen(SCREEN_ID_MAIN_2);
			index++;
		}
		break;

		case 2:
		{
			// 初始化并更新LVGL显示界面
			lvgl_ui_update();

			ui_tick();
		}
		break;
		
		default:
			index = 0;
			break;
	}
}


/***********************************************************************************************************************
-----函数功能    显示TICK
-----说明(备注)  在定时器中断中调用，只更新LVGL时基
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDisp_TickTimer(void) 
{
	// 注意：lv_task_handler() 不能在定时器回调中调用（栈空间不足）
	// 它应该在 vDisp_Task 任务中调用
	lv_tick_inc(boardREPET_TIMER_CYCLE_TMIE);
}



/*****************************************************************************************************************
-----函数功能    初始化参数
-----说明(备注)  none
-----传入参数    p_disp_mem : disp记忆参数结构体
-----输出参数    none
-----返回值      true:设置成功  反之失败
*****************************************************************************************************************/
bool bDisp_MemParamInit(DispMenParam_T* p_disp_mem)
{
	p_disp_mem->ucHighLightValue = boardDISP_HIGH_LIGHT_VALUE;
	p_disp_mem->ucLowLightValue = boardDISP_LOW_LIGHT_VALUE;
	p_disp_mem->usAutoOffTime = boardDISP_OFF_TIME;
	return true;
}

/*****************************************************************************************************************
-----函数功能    设置需要切换的屏幕
-----说明(备注)  该函数设置标志位，实际的屏幕切换由显示任务在下一个循环中处理
-----传入参数    screenId: 目标屏幕ID
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
bool bDisp_ChangeScreen(ScreenId_E screenId)
{
	if(screenId == ID_MAIN_0)
	{
		if(tDisp.eTargetScreen >= ID_MAIN_3)
			tDisp.eTargetScreen = ID_MAIN_2;
		else
			tDisp.eTargetScreen = ID_MAIN_3;
		return true;
	}
	
	tDisp.eTargetScreen = screenId;
	return true;
}

/*****************************************************************************************************************
-----函数功能    统一设置设备状态
-----说明(备注)  设置设备状态标志，实际的UI更新在displaytask中处理
-----传入参数    devType: 设备类型, bState: true=开启状态, false=关闭状态
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vDisp_SetDevStateIcon(DevType_E devType, bool bState)
{
	switch(devType)
	{
		case DEV_TYPE_AC:
			tDisp.bDevAcState = bState;
			break;
		
		case DEV_TYPE_PV:
			tDisp.bDevPvState = bState;
			break;
		
		case DEV_TYPE_LIGHT:
			tDisp.bDevLightState = bState;
			break;
		
		case DEV_TYPE_USB_A:
			tDisp.bDevUsbAState = bState;
			break;
		
		case DEV_TYPE_USB_C1:
			tDisp.bDevUsbC1State = bState;
			break;
		
		case DEV_TYPE_USB_C2:
			tDisp.bDevUsbC2State = bState;
			break;
		
		default:
			break;
	}
}

#if(boardLOW_POWER)
/*****************************************************************************************************************
-----函数功能    检查系统的输入电源:外接电池
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void v_dis_power_select( void )
{
	if(tDisp.bLight)
	{
		/*************************************电源有输入*********************************************************/
		if( tAdc_SysSamp.usBMS_Vin >= boardBMS_MIN_VOLT )   
		{
			Disp_EN_OFF();          //关闭显示屏的电池供电
		}
		/*************************************没有电源输入*******************************************************/
		else
		{
			Disp_EN_ON();          //打开显示屏的电池供电
		}	
	}
	else 
	{
		Disp_EN_OFF();          //关闭显示屏的电池供电
	}
}


/*****************************************************************************************************************
-----函数功能    进入低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vLcd_EnterLowPower(void)
{
//	vLcd_IoEnterLowPower();
//	bAtti_EnterLowPower();
//	vExRTC_EnterLowPower();
	vTaskSuspend(tDisp_Task_Handler);
}

/*****************************************************************************************************************
-----函数功能    退出低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vLcd_ExitLowPower(void)
{
//	vExRTC_ExitLowPower();
	vTaskResume(tDisp_Task_Handler);
}
#endif //boardLOW_POWER

#endif //boardDISPLAY



