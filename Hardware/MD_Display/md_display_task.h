#ifndef MD_DISPLAY_TASK_H_
#define MD_DISPLAY_TASK_H_
#include "main.h"
#include "board_config.h"

#if(boardDISPLAY)

#if(boardENG_MODE_EN)
#include "md_lcd_eng_mode.h"
#endif

typedef enum : uint8_t
{
	ID_MAIN_0 = 0,	//循环切换屏幕
    ID_MAIN_1 = 1,
    ID_MAIN_2 = 2,
    ID_MAIN_3 = 3,
}ScreenId_E;

// 设备类型枚举
typedef enum : uint8_t
{
	DEV_TYPE_AC = 0,    // DCAC设备
	DEV_TYPE_PV,        // PV设备
	DEV_TYPE_LIGHT,     // Light设备
	DEV_TYPE_USB_A,     // USB A设备
	DEV_TYPE_USB_C1,    // USB C1设备
	DEV_TYPE_USB_C2,    // USB C2设备
	DEV_TYPE_USB_DC,    // DC设备
	DEV_TYPE_MAX
}DevType_E;

typedef struct
{
    bool             bLight;            //1:打开   0:关闭
	bool             bSleepShow;       //1:打开   0:关闭
	ScreenId_E 		 eTargetScreen;     //目标屏幕ID
	vu16             usAutoOffTime;     //息屏时间
	vu16             usAutoOffCnt;      //息屏倒计时
	
	// 设备状态
	bool             bDevAcState;      // DCAC设备状态(true=开, false=关)
	bool             bDevPvState;      // PV设备状态
	bool             bDevLightState;   // Light设备状态
	bool             bDevUsbAState;   // USB A设备状态
	bool             bDevUsbC1State;  // USB C1设备状态
	bool             bDevUsbC2State;  // USB C2设备状态
	bool             bDevDcState;     // DC设备状态
}Disp_T;  
extern Disp_T   tDisp; 

#pragma pack(1) //强制一个字节对齐
typedef struct
{
	u8           ucHighLightValue;
	u8           ucLowLightValue;
	vu16         usAutoOffTime;      //存储息屏的时间,大于0存在有息屏,0为常亮
}DispMenParam_T;
#pragma pack()  //取消一个字节对齐

bool bDisp_TaskInit(void);
bool bDisp_Switch(SwitchType_E type, bool fore_en);
void vDisp_RefreshDisplayParam(void);
void vDisp_AutoOffCnt(void);
void vDisp_TickTimer(void);
bool bDisp_MemParamInit(DispMenParam_T* p_lcd_mem);
u16 usDisp_ErrCodeDisplay(void);
bool bDisp_ChangeScreen(ScreenId_E screenId);

// 统一设备状态设置函数
void vDisp_SetDevStateIcon(DevType_E devType, bool bState);

#if(!boardUSE_OS)
void vDisp_Task(void *pvParameters);
#endif

#if(boardLOW_POWER)
void vLcd_EnterLowPower(void);
void vLcd_ExitLowPower(void);
#endif  //boardLOW_POWER

#endif  //boardDISPLAY

#endif  //MD_DISPLAY_TASK_H_


