/***********************************************************************************************************************
-----@file usb_prot_frame.c
-----函数功能	SW7226 12V 8A DC输出
-----作者       LJD
-----版本 1.0.0
-----日期       2026-01-21
-----
-----@copyright Copyright (c) 2026  深圳市百酷新能源有限公司
-----
-----@par 修改日志:
************************************************************************************************************************/
#include "UsbDevice/usb_prot_frame.h"

#if(1)
#include "UsbDevice/usb_queue_task.h"
#include "UsbDevice/usb_task.h"
#include "UsbDevice/usb_iface.h"
#include "Print/print_task.h"

// #include "check.h"
#include "math.h"

// #define       	usbDEV_ADRR                          	0x01
// #define  		usbWAIT_NOTIFY_OUTTIME              	1000     //任务通知超时时间 MS
// #define       	usbTX_PROTO_BUFF_LEN                   128
// #define       	usbRX_PROTO_BUFF_LEN                   256

//*********************************寄存器地址********************************
// SW7226 寄存器地址
#define     	SW7226_REG_IRQ_ENABLE1             0x02    // IRQ中断使能1
#define     	SW7226_REG_IRQ_ENABLE2             0x03    // IRQ中断使能2
#define     	SW7226_REG_WORK_STATUS             0x06    // 工作状态指示
#define     	SW7226_REG_FUNC_SET                0x0D    // 功能设置
#define     	SW7226_REG_CHG_DISCHG_EN          0x0E    // 充放电使能
#define     	SW7226_REG_ADC                     0x10    // ADC
#define     	SW7226_REG_ADC_DATA_HIGH           0x11    // ADC数据高8位
#define     	SW7226_REG_ADC_DATA_LOW            0x12    // ADC数据低4位
#define     	SW7226_REG_ADC_STATE               0x13    // ADC状态
#define     	SW7226_REG_DISCHG_VBUS_HIGH        0x20    // 放电电压设置高4位
#define     	SW7226_REG_DISCHG_VBUS_LOW         0x21    // 放电电压设置低8位
#define     	SW7226_REG_DISCHG_IBUS_LIMIT       0x22    // IBUS输出限流设置
#define     	SW7226_REG_DISCHG_IBAT_LIMIT       0x23    // IBAT输出限流设置
#define     	SW7226_REG_BAT_UVLO                0x24    // 放电电池欠压门限设置
#define     	SW7226_REG_BAT_UVLO_HYS           0x25    // 放电电池欠压迟滞设置
#define     	SW7226_REG_CHG_VOLTAGE_HIGH        0x30    // 充电目标电压设置高4位
#define     	SW7226_REG_CHG_VOLTAGE_LOW         0x31    // 充电目标电压设置低8位
#define     	SW7226_REG_IBUS_CHG_CUR            0x32    // IBUS充电电流设置
#define     	SW7226_REG_IBAT_CHG_CUR            0x33    // IBAT充电电流设置
#define     	SW7226_REG_TRICKLE_VOLTAGE         0x34    // 涓流充电电压设置
#define     	SW7226_REG_TRICKLE_CURRENT          0x35    // 涓流充电电流设置
#define     	SW7226_REG_CHG_END_CUR             0x36    // 充电截止电流
#define     	SW7226_REG_VINDPM_HIGH             0x37    // 输入限压门限设置高1位
#define     	SW7226_REG_VINDPM_LOW              0x38    // 输入限压门限设置低8位
#define     	SW7226_REG_CHG_NTC_CONFIG          0x39    // 充电功能相关配置1
#define     	SW7226_REG_CHG_TIMEOUT_CONFIG      0x3A    // 充电功能相关配置2
#define     	SW7226_REG_CHG_DISCHG_CONFIG1      0x71    // 充放电功能配置1
#define     	SW7226_REG_CHG_DISCHG_CONFIG2      0x72    // 充放电功能配置2
#define     	SW7226_REG_CHG_DISCHG_CONFIG3      0x73    // 充放电功能配置3
#define     	SW7226_REG_CHG_DISCHG_CONFIG4      0x74    // 充放电功能配置4
#define     	SW7226_REG_CHG_DISCHG_CONFIG5      0x75    // 充放电功能配置5
#define     	SW7226_REG_CHG_DISCHG_CONFIG8      0x7A    // 充放电功能配置8
#define     	SW7226_REG_CHG_DISCHG_CONFIG7      0x79    // 充放电功能配置7

// SW7226 ADC通道定义
#define     	SW7226_ADC_CH_VBAT                0x00    // VBAT电压
#define     	SW7226_ADC_CH_VBUS                0x01    // VBUS电压
#define     	SW7226_ADC_CH_IBAT_CHG            0x02    // IBAT充电电流
#define     	SW7226_ADC_CH_IBAT_DISCHG         0x03    // IBAT放电电流
#define     	SW7226_ADC_CH_IBUS_CHG            0x04    // IBUS充电电流
#define     	SW7226_ADC_CH_IBUS_DISCHG         0x05    // IBUS放电电流
#define     	SW7226_ADC_CH_TDIE               0x06    // 芯片温度
#define     	SW7226_ADC_CH_VNTC               0x07    // NTC电压




//****************************************************参数初始化**************************************************//
// __ALIGNED(4) 	ModbusProtoTx_t *tpUsbProtoTx = NULL;	//发送协议
// __ALIGNED(4) 	ModbusProtoRx_t *tpUsbProtoRx = NULL;	//接受协议

USB_IC_T tUsbIc1 = {0}; //SW7226芯片

// SW7226 ADC数据结构
typedef struct
{
	u16 usVbat;        // VBAT电压(mV)
	u16 usVbus;        // VBUS电压(mV)
	u16 usIbatChg;     // IBAT充电电流(mA)
	u16 usIbatDischg;  // IBAT放电电流(mA)
	u16 usIbusChg;     // IBUS充电电流(mA)
	u16 usIbusDischg;  // IBUS放电电流(mA)
	s16 sTdie;         // 芯片温度(0.1°C)
	u16 usVntc;        // NTC电压(mV)
}SW7226_ADC_T;

//****************************************************函数声明****************************************************//
// static s8 c_usb_data_trans(u8 cmd, u16 reg_addr, u8* data, u8 len);
s8 c_usb_read_out_volt_cs(void);


/***********************************************************************************************************************
-----函数功能    通讯协议初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bUsb_SendProtInit(void)
{
	// s8 c_result = 1;

	// c_result = cModbus_TransProtoInit(&tpUsbProtoTx, usbTX_PROTO_BUFF_LEN, usbDEV_ADRR);
	// if(c_result <= 0)
	// {
	// 	if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
	// 		log_e("bUsbTask:tpUsbProtoTx协议对象初始化失败,代码%d",c_result);
		
	// 	return false;
	// }
	
	return true;
}

bool bUsb_RecProtInit(void)
{
	// s8 c_result = cModbus_RecProtoInit(&tpUsbProtoRx, 	//协议指针
	// 							usbRX_PROTO_BUFF_LEN,	//协议缓存器大小
	// 							usbDEV_ADRR,			//协议设备ID
	// 							boardREPET_TIMER_CYCLE_TMIE);			//计数器采样时间
	// if(c_result <= 0)
	// {
	// 	if(uPrint.tFlag.bUsbRecTask || uPrint.tFlag.bImportant)
	// 		log_e("bUsbRecTask:tpUsbProtoRx协议对象初始化失败,代码%d",c_result);
	// 	return false;
	// }
	
	return true;
}

/*****************************************************************************************************************
-----函数功能    指令:初始化IC
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_usb_cs_ic1_init(void)
{
	u8 data[1] = {0};
	s8 c_ret = 0;
	static u8 index = 0;
	static u8 uc_lost_cnt = 0;
	
	switch(index)
	{
		case 0:
		{
			c_ret = c_usb_read_out_volt_cs();
			if(c_ret == -1)
			{
				usbPOWER_EN_ON();
				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:读取IC1失败");
				vTaskDelay(1000);
				break;
			}
			//已经初始化
			else if(c_ret > 0)
			{
				return 2;
			}

			uc_lost_cnt = 0;
			index++;
		}

		/* (1) 充电VBUS过压保护门限设置, reg0x75[5:4] */
		case 1:
		{
			/* 读取当前reg0x75的值 */
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_DISCHG_CONFIG5, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			/* 设置充电VBUS过压保护门限为40V (bit[5:4]=3) */
			data[0] = (data[0] & 0xCF) | 0x30;
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_DISCHG_CONFIG5, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (2) 放电VBUS过载门限设置, reg0x74[4] */
		case 2:
		{
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_DISCHG_CONFIG4, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			/* 设置峰值限流为14A (bit[6:5]=0) */
			/* 设置放电VBUS过载门限为70%*VBUS (bit[4]=0) */
			/* 设置放电VBAT过压保护门限为26V (bit[3:2]=0) */
			data[0] = data[0] & 0x83;
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_DISCHG_CONFIG4, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (4) Q2功率管内阻设置, reg0x71[7:6] */
		case 3:
		{
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_DISCHG_CONFIG1, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			/* 设置Q2功率管内阻为20mΩ (bit[7:6]=0) */
			data[0] = (data[0] & 0x3F) | 0x00;
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_DISCHG_CONFIG1, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (6) 抖频功能, reg0x72[5:4] */
		case 4:
		{
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_DISCHG_CONFIG2, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			/* 使能抖频功能,抖频范围±4% (bit[5:4]=0) */
			data[0] = (data[0] & 0xCF) | 0x00;
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_DISCHG_CONFIG2, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (7) 充电NTC低温保护门限设置, reg0x39[6:4] */
		case 5:
		{
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_NTC_CONFIG, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			/* 设置充电NTC低温保护门限为关闭 (bit[6:4] = 7) */
			/* 设置充电NTC高温保护门限为65°C (bit[2:0]=4) */
			data[0] = (data[0] & 0xC8) | 0x00 | 0x04;
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_NTC_CONFIG, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (9) 放电NTC低温保护门限设置, reg0x73[6:4] */
		case 6:
		{
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_DISCHG_CONFIG3, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			/* 设置放电NTC低温保护门限为-10°C (bit[6:4]=2) */
			/* 设置放电NTC高温保护门限为65°C (bit[2:0]=4) */
			data[0] = (data[0] & 0x88) | 0x10 | 0x04;
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_DISCHG_CONFIG3, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (11) 恒流充电超时门限设置, reg0x3A[3:2] */
		case 7:
		{
			// if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_TIMEOUT_CONFIG, data, sizeof(data)) <= 0)
			// {
			// 	if(uc_lost_cnt < 0xff) 
			// 		uc_lost_cnt++;
			// 	break;
			// }
			/* 设置恒流充电超时门限为24h (bit[3:2]=1) */
			/* 设置涓流充电超时门限为2h (bit[1:0]=1) */
			data[0] = 0x9F;
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_TIMEOUT_CONFIG, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (13) IRQ中断使能设置, reg0x02和reg0x03 */
		case 8:
		{
			// /* 使能所有IRQ中断, reg0x02 = 0x00 */
			// data[0] = 0x00;
			// if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_IRQ_ENABLE1, data, sizeof(data)) <= 0)
			// {
			// 	if(uc_lost_cnt < 0xff) 
			// 		uc_lost_cnt++;
			// 	break;
			// }
			// /* 使能所有IRQ中断, reg0x03 = 0x00 */
			// data[0] = 0x00;
			// if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_IRQ_ENABLE2, data, sizeof(data)) <= 0)
			// {
			// 	if(uc_lost_cnt < 0xff) 
			// 		uc_lost_cnt++;
			// 	break;
			// }
			uc_lost_cnt = 0;
			index++;
		}

		/* === 2.2.2 I2C调压打开放电 === */
		
		/* (14) I2C调压设置, reg0x7A[6] */
		case 9:
		{
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_DISCHG_CONFIG8, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			/* 设置I2C调压模式 (bit[6]=0) */
			data[0] = data[0] & 0xBF;
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_DISCHG_CONFIG8, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (15) 放电VBUS输出电压设置高4位, reg0x20[3:0] */
		case 10:
		{
			/* 设置VBUS输出电压为12V (Vbus_dischg = 12V = (12-3)/0.1 = 90 = 0x05A)
			 * 高4位: 0x05, 低8位: 0x5A */
			data[0] = 0x03;  // reg0x20[3:0] = 0x05
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_DISCHG_VBUS_HIGH, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (16) 放电VBUS输出电压设置低8位, reg0x21[7:0] */
		case 11:
		{
			/* 必须紧接着上一步写reg0x21, 中间不能有其他I2C操作 */
			data[0] = 0x84;  // reg0x21[7:0] = 0x5A
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_DISCHG_VBUS_LOW, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (17) 放电VBUS输出限流设置, reg0x22 */
		case 12:
		{
			/* 设置VBUS输出限流为8A (Ibus_dischg_lim = (159+1)*0.05 = 8A) */
			data[0] = 0x9F;  // 159
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_DISCHG_IBUS_LIMIT, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (18) 放电VBAT输出限流设置, reg0x23 */
		case 13:
		{
			// /* 设置VBAT输出限流为8A (Ibat_dischg_lim = (79+1)*0.1 = 8A) */
			// data[0] = 0x4F;  // 79
			// if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_DISCHG_IBAT_LIMIT, data, sizeof(data)) <= 0)
			// {
			// 	if(uc_lost_cnt < 0xff) 
			// 		uc_lost_cnt++;
			// 	break;
			// }
			uc_lost_cnt = 0;
			index++;
		}

		/* (19) 放电VBAT欠压门限设置, reg0x24 */
		case 14:
		{
			/* 设置VBAT欠压门限为9V (Bat_uvlo = (12-3.8)/0.1 = 82 = 0x52) */
			data[0] = 0x34;
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_BAT_UVLO, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (20) 放电VBAT欠压迟滞设置, reg0x25[4:0] */
		case 15:
		{
			// if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_BAT_UVLO_HYS, data, sizeof(data)) <= 0)
			// {
			// 	if(uc_lost_cnt < 0xff) 
			// 		uc_lost_cnt++;
			// 	break;
			// }
			// /* 设置VBAT欠压迟滞为1V (Bat_uvlo_hys = (1-0.2)/0.1 = 8) */
			// data[0] = (data[0] & 0xE0) | 0x08;
			// if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_BAT_UVLO_HYS, data, sizeof(data)) <= 0)
			// {
			// 	if(uc_lost_cnt < 0xff) 
			// 		uc_lost_cnt++;
			// 	break;
			// }

			index = 99;
			uc_lost_cnt = 0;
		}
		break;
		
		default:
			index = 0;
		break;
	}

	if(uc_lost_cnt >= 10)
	{
		if(tUsb.uErrCode.tCode.bIc1Lost == 0)
		{
			bUsb_SetErrCode(UEC_IC1_LOST,true);
		
			if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
				log_e("bUsbTask:IC1丢失");
		}
		return -1;
	}
	else 
	{
		if(tUsb.uErrCode.tCode.bIc1Lost == 1)
			bUsb_SetErrCode(UEC_IC1_LOST,false);
		
		if(index == 99)
		{
			index = 0;

			if(uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:IC1初始化完成 \r\n");

			return 1;
		}

		return 0;
	}
}


/*****************************************************************************************************************
-----函数功能    指令:获取IC1参数(SW7226 ADC数据读取)
-----说明(备注)  按照软件应用手册2.4 ADC数据读取实现
-----传入参数    none
-----输出参数    tSw7226Adc: ADC数据结构体
-----返回值      -1:失败
              	0:进行中
              	1:完成
******************************************************************************************************************/
s8 c_usb_cs_get_ic1_param(void)
{
	u8 data[1] = {0};
	u8 adc_busy = 0;
	static u8 uc_lost_cnt = 0;
	static u8 index = 0;
	static u16 usAdcData = 0;  // 12位ADC数据
	static SW7226_ADC_T tSw7226Adc = {0};  // SW7226 ADC数据缓存
	
	switch(index)
	{
		/* (6) 选择ADC通道: VBUS, 等待ADC转换完成 */
		case 0:
		{
			/* 选择VBUS通道 (bit[3:0]=1) */
			data[0] = 0x11;  // 11010001
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_ADC, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (7) 检查VBUS ADC转换状态 */
		case 1:
		{
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_ADC_STATE, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			adc_busy = data[0] & 0x01;
			if(adc_busy != 0)
				break;
			uc_lost_cnt = 0;
			index++;
		}

		/* (8) 读取VBUS ADC数据 */
		case 2:
		{
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_ADC_DATA_HIGH, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			usAdcData = (u16)(data[0] << 4);
			
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_ADC_DATA_LOW, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			usAdcData |= (data[0] & 0x0F);
			
			/* Vbus = adc_data * 10mV */
			tSw7226Adc.usVbus = usAdcData * 10;
			
			uc_lost_cnt = 0;
			index++;
		}

		/* (18) 选择ADC通道: IBUS放电电流, 等待ADC转换完成 */
		case 3:
		{
			/* 选择IBUS放电电流通道 (bit[3:0]=5) */
			data[0] = 0x15;  // 01010101
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_ADC, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (19) 检查IBUS放电电流ADC转换状态 */
		case 4:
		{
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_ADC_STATE, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			adc_busy = data[0] & 0x01;
			if(adc_busy != 0)
				break;
			uc_lost_cnt = 0;
			index++;
		}

		/* (20) 读取IBUS放电电流ADC数据 */
		case 5:
		{
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_ADC_DATA_HIGH, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			usAdcData = (u16)(data[0] << 4);
			
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_ADC_DATA_LOW, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			usAdcData |= (data[0] & 0x0F);
			
			/* Ibus_dischg = adc_data * 5mA */
			tSw7226Adc.usIbusDischg = usAdcData * 5;
			
			uc_lost_cnt = 0;
			index++;
		}

		/* (21) 选择ADC通道: 芯片温度, 等待ADC转换完成 */
		case 6:
		{
			/* 选择芯片温度通道 (bit[3:0]=6) */
			data[0] = 0x16;  // 01010110
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_ADC, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (22) 检查芯片温度ADC转换状态 */
		case 7:
		{
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_ADC_STATE, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			adc_busy = data[0] & 0x01;
			if(adc_busy != 0)
				break;
			uc_lost_cnt = 0;
			index++;
		}

		/* (23) 读取芯片温度ADC数据并计算 */
		case 8:
		{
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_ADC_DATA_HIGH, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			usAdcData = (u16)(data[0] << 4);
			
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_ADC_DATA_LOW, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			usAdcData |= (data[0] & 0x0F);
			
			/* Tdie = (adc_data - 1839) / 6.82°C */
			tSw7226Adc.sTdie = (s16)((s32)usAdcData - 1839) * 100 / 682;
			
			uc_lost_cnt = 0;
			index++;
		}

		/* (24) 选择ADC通道: NTC电压, 等待ADC转换完成 */
		case 9:
		{
			/* 选择NTC电压通道 (bit[3:0]=7) */
			data[0] = 0x17;  // 01010111
			if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_ADC, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			uc_lost_cnt = 0;
			index++;
		}

		/* (25) 检查NTC电压ADC转换状态 */
		case 10:
		{
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_ADC_STATE, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			adc_busy = data[0] & 0x01;
			if(adc_busy != 0)
				break;
			uc_lost_cnt = 0;
			index++;
		}

		/* (26) 读取NTC电压ADC数据并计算 */
		case 11:
		{
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_ADC_DATA_HIGH, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			usAdcData = (u16)(data[0] << 4);
			
			if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_ADC_DATA_LOW, data, 1) <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
				break;
			}
			usAdcData |= (data[0] & 0x0F);
			
			/* Vntc = adc_data * 11mV (NTC检测电流40uA时) */
			tSw7226Adc.usVntc = usAdcData * 11;
			
			/* 更新USB_IC_T结构体数据 */
			tUsbIc1.cTemp = (vs8)(tSw7226Adc.sTdie);
			tUsbIc1.usVolt = tSw7226Adc.usVbus;
			
			/* 根据工作状态计算功率 */
			tUsbIc1.usQcCurr = tSw7226Adc.usIbusDischg;   // 放电电流
			
			/* 计算功率 (W) */
			tUsbIc1.usQcPwr = (u16)(tSw7226Adc.usVbus * tSw7226Adc.usIbusDischg / 1000000);
			tUsbIc1.sPower = (tUsbIc1.usPdPwr + tUsbIc1.usQcPwr) * 0.4f;
			tUsbDev.usDcPwr = tUsbIc1.sPower;

			us_usb_total_out_pwr += tUsbIc1.sPower;
			s_total_temp = MAX2(s_total_temp, tUsbIc1.cTemp);

			if(tUsbIc1.usVolt >= 10000)
			{
				tUsbDev.eDcDevState = DS_WORK;
			}
			else
			{
				tUsbDev.eDcDevState = DS_SHUT_DOWN;
			}
			

			uc_lost_cnt = 0;
			index = 0;
			
			if(uPrint.tFlag.bUsbTask)
			{
				sMyPrint("  VBUS=%dmV, IBUS_DISCHG=%dmA \r\n", tSw7226Adc.usVbus, tSw7226Adc.usIbusDischg);
				sMyPrint("  TDIE=%d°C, VNTC=%dmV\r\n", tSw7226Adc.sTdie, tSw7226Adc.usVntc);
				sMyPrint("  电压=%dmV, 功率=%dW\r\n", tUsbIc1.usVolt, tUsbIc1.sPower);
			}
			break;
		}

		default:
			index = 0;
			break;
	}

	/* 处理IC丢失状态 */
	if(uc_lost_cnt >= 20)
	{
		if(tUsb.uErrCode.tCode.bIc1Lost == 0)
		{
			bUsb_SetErrCode(UEC_IC1_LOST, true);
			tUsbDev.eDcDevState = DS_SHUT_DOWN;
			if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
				log_e("bUsbTask:SW7226 IC1丢失");
		}
		return -1;
	}
	else if(!uc_lost_cnt && index == 0)
	{
		if(tUsb.uErrCode.tCode.bIc1Lost == 1)
			bUsb_SetErrCode(UEC_IC1_LOST, false);
		return 1;
	}
	else
	{
		return 0;
	}
}

/*****************************************************************************************************************
-----函数功能    指令:开关MPPT
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_usb_set_pwr_cs(u16 pwr)
{
	// if(c_usb_data_trans(modbusWRITE_SINGLE_REG, 
	// 					usbREG_ADDR_SET_CHG_PWR, 
	// 					(u8*)&pwr, 
	// 					1) <= 0)
	// 	return false;
	
	return 1;
}

/*****************************************************************************************************************
-----函数功能    指令:开关12V 8A DC
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_usb_switch_dc_cs(bool sw)
{
	u8 data[1] ={0};

	if(sw == true)
		data[0] = 0x10;

	if(cI2C_WriteBytes(&tUSB_IC1_I2C, SW7226_REG_CHG_DISCHG_EN, data, 1) <= 0)
		return -1;

	return 1;
}

/***********************************************************************************************************************
-----函数功能	指令：读取输出电压
-----返回值     s8
-----作者       LJD
-----日期       2026-01-20
************************************************************************************************************************/
s8 c_usb_read_out_volt_cs(void)
{
	u8 data[2] ={0};

	if(cI2C_ReadBytes(&tUSB_IC1_I2C, SW7226_REG_DISCHG_VBUS_HIGH, data, sizeof(data)) <= 0)
		return -1;

	//未初始化
	if(data[0] == 0x00 && data[1] == 0xC8)
		return -2;
	//初始化完成
	return 1;
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
// static s8 c_usb_data_trans(u8 cmd, u16 reg_addr, u8* data, u8 len)
// {
// 	s8 result = 0;
	
// 	if(dcacSemaphoreMutex == NULL)
// 		return 0;
	
// 	if(tpDcacProtoTx == NULL)
// 		return 0;
	
// 	//开始互斥
// 	#if(boardUSE_OS)
// 	if(xSemaphoreTake(dcacSemaphoreMutex, pdMS_TO_TICKS(1000)) == pdFAIL)
// 		return -99;
// 	#endif  //boardUSE_OS
	
// 	//开始发送
// 	#if(boardDCAC_IFACE)
// 	result = cModbus_ProtoCreate(tpDcacProtoTx, cmd, reg_addr, data, len);
// 	if(result > 0)
// 	{
// 		if(bDcac_DataSendStart(tpDcacProtoTx->ucaFrameData, tpDcacProtoTx->ucFrameLen) == true)
// 		{
// 			//等待任务通知,等待时间为1S
// 			#if(boardUSE_OS)
// 			if(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(usbWAIT_NOTIFY_OUTTIME)) <= 0) 
// 			{
// 				if(uPrint.tFlag.bUsbTask)
// 					log_w("bUsbTask:等待指令0x%x,地址0x%x回复超时", cmd, reg_addr);
				
// 				result = -2;
// 			}
// 			#endif  //boardUSE_OS
// 		}
// 		else 
// 			result = -3;
// 	}
// 	#endif
	
// 	cModbus_ResetTx(tpDcacProtoTx, usbTX_PROTO_BUFF_LEN);
// 	cModbus_ResetTx(tpUsbProtoTx, usbTX_PROTO_BUFF_LEN);
	
// 	//释放互斥量
// 	#if(boardUSE_OS)
// 	xSemaphoreGive(dcacSemaphoreMutex);
// 	#endif  //boardUSE_OS
	
// 	return result;
// }

#endif  //boardMPPT_IFACE
