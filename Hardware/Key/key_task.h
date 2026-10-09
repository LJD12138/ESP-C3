#ifndef KEY_TASK_H_
#define KEY_TASK_H_

#include "main.h"
#include "board_config.h"
#include "driver/gpio.h"

//#define		//4Tab									//13Tab
#define			keyTASK_CYCLE_TIME						10		//按键任务更新时间

#define			keyNUM									3		//按键的数量
#define			keyGROUP_NUM							5		//组合按键种类   
#define			keySHORT_PRESS_TIME						2		//短按按键的最小时间*10ms
#define			keyLONG_PRESS_TIME						120		//长按按键的最小时间*10ms
#define     	keySUPER_LONG_PRESS_TIME           		200	    //超长按按键的最小时间*10ms
#define			keyNUPRESS_MAX_TIME						35		//组合按键最大的等待时间*10ms
#define			keyADD_SPACE_TIME						20		//长按累加间隔

// #define     	keyGPIO_POWER_RCU       				RCU_GPIOC
// #define     	keyGPIO_POWER_PORT      				GPIOC
#define     	keyGPIO_POWER_PIN       				GPIO_NUM_9

// #define     	keyGPIO_AC_RCU          				RCU_GPIOB
// #define     	keyGPIO_AC_PORT         				GPIOB
// #define     	keyGPIO_AC_PIN          				GPIO_NUM_0

// #define     	keyGPIO_USB_RCU        					RCU_GPIOB
// #define     	keyGPIO_USB_PORT        				GPIOB
// #define     	keyGPIO_USB_PIN        					GPIO_NUM_23


// #define     	keyGPIO_LIGHT_PIN        				GPIO_NUM_0


__STATIC_INLINE bool bKey_PowerIsPress(void)          
{    
    if(gpio_get_level(keyGPIO_POWER_PIN) == 0)//读取按键
        return true;
    else
        return false;
}

__STATIC_INLINE bool bKey_AcIsPress(void)
{
    // if(gpio_get_level(keyGPIO_AC_PIN) == 0)//读取按键
    //     return true;
    // else
        return false;
}

__STATIC_INLINE bool bKey_LightIsPress(void)
{
    // if(gpio_get_level(keyGPIO_LIGHT_PIN) == 0)//读取按键
    //     return true;
    // else
        return false;
}

__STATIC_INLINE bool bKey_UsbIsPress(void)
{
    // if(gpio_get_level(keyGPIO_USB_PIN) == 0)//读取按键
    //     return true;
    // else
        return false;
}

__STATIC_INLINE bool bKey_DcIsPress(void)
{
    // if((GPIO_ISTAT(keyGPIO_DC_PORT)&(keyGPIO_DC_PIN)) == 0)//读取按键
    //     return true;
    // else
        return false;
}


typedef enum : uint8_t
{
    KTE_FUN_NULL = 0,
    KTE_POWER_LONG,
    KTE_POWER_SHORT,
	KTE_POWER_SUPER_LONG,
    KTE_AC_LONG,
	KTE_AC_SHORT,
	KTE_AC_SUPER_LONG,
	KTE_LIGHT_LONG,
	KTE_LIGHT_SHORT,
	KTE_LIGHT_SUPER_LONG,
	KTE_USB_LONG,
	KTE_USB_SHORT,
	KTE_USB_SUPER_LONG,
	KTE_DC_LONG,
	KTE_DC_SHORT,
	KTE_DC_SUPER_LONG,
} KeyTriEvent_e; //触发事件

typedef struct
{
    bool    			(*IsPress)(void);        
    vs16    			sOnPressCnt;	  	//按键按下计时  0:表示按键没触发   -1:表示按键功能已经被记录
	bool        		bEnMulitFunKey;     //使能多功能按键
	bool        		bEnLongPressAdd;    //使能长按累加      
}KeyHandler_t;


void vKey_TaskInit(void);
void vKey_PowerIsTri(void);
void vKey_ParamInit(void);

#if(!boardUSE_OS)
void vKey_Task(void *pvParameters);
#endif

#if(boardLOW_POWER)
void vKey_EnterLowPower(void);
void vKey_ExitLowPower(void);
#endif

#endif






















