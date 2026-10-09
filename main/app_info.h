#ifndef APP_INFO_H_
#define APP_INFO_H_

#include "main.h"
#include "board_config.h"

#include "Sys/sys_task.h"
// #include "Dc/dc_task.h"
#include "UsbDevice/usb_task.h"

#if(boardBMS_IFACE)
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_IFACE

#if(boardMPPT_IFACE)
#include "MD_Mppt/md_mppt_task.h"
#endif  //boardMPPT_IFACE

#if(boardDCAC_IFACE)
#include "MD_Dcac/md_dcac_task.h"
#endif  //boardDCAC_IFACE

#if(boardDISPLAY)
#include "MD_Display/md_display_task.h"
#endif  //boardDISPLAY

#if(boardEASY_FLASH)
#include "easyflash.h"
#endif

extern const char tAppMemParamStr[];
extern const char tAppVerInfoStr[];
extern const char tAppParamStr[];
extern const char tDispMemParamStr[];
extern const char tDcMemParamStr[];
extern const char tUsbMemParamStr[];
extern const char tBmsMemParamStr[];
extern const char tMpptMemParamStr[];
extern const char tDcacMemParamStr[];
extern const char tSysMemParamStr[];

///********************************存在BOOT：IAP_FLASH_INFO**********************************************
//APP状态
typedef enum : uint8_t
{
	AS_NULL = 0,		//未选择
    AS_FINISH,			//刚升级完成
	AS_OK,				//当前是完整的
	AS_ERASE,			//已经擦除
}AppState_E;

#pragma pack(1)  //一个字节对齐
typedef struct
{
	char        		saVersion[32];    // 软件版本
    char        		saBuildDate[32];  // 程序编译日期
    char        		saBuildTime[32];  // 程序编译时间
}VerInfo_T;
#pragma pack()

#pragma pack(4)  //一个字节对齐
typedef struct
{
	vu32        		ulCmd;             	// 0xAAAA_AAAA需要升级,其他不需要升级
	AppState_E			eAppState;			// APP状态
	vu8					ucAppFaultCnt;		// APP启动失败计数 
}BootParam_T;
#pragma pack()

#pragma pack(1)  //一个字节对齐
typedef struct
{
	uint16_t         	usInitFinish;     	// APP参数初始化完成标志
	uint16_t         	usUniqueID[6];    	// ID
}AppParam_T;
#pragma pack()

#pragma pack(2)
typedef struct
{
	VerInfo_T        	tVerInfo;
	BootParam_T			tParam;
}BootMemParam_T;
#pragma pack()
extern BootMemParam_T  	tBootMemParam;

#pragma pack(2)
typedef struct
{
	VerInfo_T        	tVerInfo;
	AppParam_T			tParam;
	// DispMemParam_T    	tDISP;
	// DcMemParam_T    	tDC;
	UsbMemParam_T   	tUSB;

	#if(boardBMS_IFACE)
	BmsMemParam_T   	tBMS;
	#endif  //boardBMS_IFACE

	#if(boardMPPT_IFACE)
	MpptMemParam_T  	tMPPT;
	#endif  //boardMPPT_IFACE

	#if(boardDCAC_IFACE)
	DcacMemParam_T  	tDCAC;
	#endif  //boardDCAC_IFACE
	
	SysMemParam_T    	tSYS;
}AppMemParam_T;
#pragma pack()
extern AppMemParam_T  	tAppMemParam;

#if(boardEASY_FLASH)
extern const ef_env default_env_set[];
#endif

void vApp_JumpToBoot(uint32_t cmd);
s8 cApp_BootInfoInit(void);
s8 cApp_AppInfoInit(void);
s8 cApp_MemParamInit(const char* id_str);
s16 cApp_UpdataMemParam(const char* id_str);
s16 cApp_GetMemParam(const char* id_str);
u16 usApp_GetMemParamSize(void);
s16 cApp_BootUpdataMemParam(const char* id_str);
s16 cApp_BootGetMemParam(const char* id_str);
void vApp_Test(void);
#endif
