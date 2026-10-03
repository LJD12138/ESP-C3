#ifndef USB_PROT_FRAME_H_
#define USB_PROT_FRAME_H_

#include "board_config.h"

#if(1)
#include "i2c2.h"
// #include "Modbus/modbus_proto.h"


// extern			ModbusProtoTx_t 						*tpUsbProtoTx;
// extern 			ModbusProtoRx_t 						*tpUsbProtoRx;

#pragma pack(1)
typedef struct
{
	vs8            		cTemp;              //温度
    vs16           		sPower;             //1W 总功率 
    vu16           		usVolt;             //mV
    vu16           		usPdCurr;       	//mA
	vu16           		usQcCurr;       	//mA
	vu16           		usPdPwr;        	//1W
    vu16           		usQcPwr;        	//1W
}USB_IC_T; 
#pragma pack()

// TYPEC角色定义
typedef enum
{
	TYPEC_ROLE_DRP = 0,    // 双角色
	TYPEC_ROLE_SRC = 1,    // 放电
	TYPEC_ROLE_SNK = 2,    // 充电
}TYPEC_ROLE_E;

//模式设置
typedef enum : uint8_t
{
	IMS_ALL = 0,  		//充放电
	IMS_DISCHG,  		//放电
    IMS_CHG,         	//充电
}Ic2ModeSet_E;

//SW6303控制对象定义
typedef enum : uint8_t
{
	IC2_CTRL_OBJ_VBUS = 0,      //输出电压
	IC2_CTRL_OBJ_CHG_VOLT,     	//充电电压
	IC2_CTRL_OBJ_MAX_IN_PWR,    //最大输入功率
	IC2_CTRL_OBJ_CHG_HOLD_VOLT, //充电 hold 门限
	IC2_CTRL_OBJ_TEMP,			//预留
	IC2_CTRL_OBJ_IBAT,     		//电池限流
	IC2_CTRL_OBJ_IBUS,      	//输出电流
	IC2_CTRL_OBJ_MAX_OUT_PWR,   //最大输出功率
	IC2_CTRL_OBJ_ALL,   		//设置所有
	IC2_CTRL_OBJ_MAX
}Ic2CtrlObj_E;

//IC1
s8 c_usb_cs_ic1_init(void);
s8 c_usb_cs_get_ic1_param(void);
s8 c_usb_set_pwr_cs(u16 pwr);
s8 c_usb_switch_dc_cs(bool sw);

//IC2
s8 c_usb_cs_ic2_init(void);
s8 c_usb_cs_get_ic2_param(void);
s8 c_usb_cs_ic2_test(void);

//IC3
s8 c_usb_cs_ic3_init(void);
s8 c_usb_cs_get_ic3_param(void);
s8 c_usb_cs_set_chg_pwr(u8 uc_req_chg_pwr);
s8 c_usb_cs_ic3_test(void);

bool bUsb_SendProtInit(void);
bool bUsb_RecProtInit(void);

#endif  //boardMPPT_IFACE

#endif  //USB_PROT_FRAME_H_
