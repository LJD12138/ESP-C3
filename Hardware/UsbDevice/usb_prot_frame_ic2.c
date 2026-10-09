/***********************************************************************************************************************
-----@file usb_prot_frame_ic2.c
-----函数功能   SW6303 单向1C+1A/45W
-----作者       LJD
-----版本 1.0.0
-----日期       2026-01-21
-----@copyright Copyright (c) 2026  深圳市百酷新能源有限公司
-----@par 修改日志:
************************************************************************************************************************/
#include "UsbDevice/usb_prot_frame.h"

#if(1)
#include "UsbDevice/usb_sw6303_common.h"
#include "UsbDevice/usb_queue_task.h"
#include "UsbDevice/usb_task.h"
#include "UsbDevice/usb_iface.h"
#include "Print/print_task.h"

#include "string.h"
#include "math.h"
#include "ntc.h"

//****************************************************参数初始化**************************************************//
USB_IC_T tUsbIc2 = {0}; //USB芯片

// I2C写操作使能状态管理
static bool s_usb_ic2_i2c_write_enabled = false;  // I2C写操作使能状态

// 外部I2C对象定义（需要在usb_iface.c中定义）
extern I2cObj_T tUSB_IC2_I2C;

/*****************************************************************************************************************
-----函数功能    指令:初始化IC
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      s8: 1:成功  0:处理中  -1:失败
******************************************************************************************************************/
s8 c_usb_cs_ic2_init(void)
{
	s8 c_ret = 0;
	static u8 index = 0;
	static u8 uc_lost_cnt = 0;

	SW6303_ForceCtrl_T t_param = {
				.usVbus = 12000,	//mV 	20V		这个不设置
				.usChgVolt = 14600,	//mV  	14.6V
				.usMaxInPwr = 0,	//W		100W
				.usChgHoldVolt = 0,	//mV 22V		这个不设置
				.usIbat = 250,		//0.1A	10A
				.usIbus = 500,		//0.01A	10A
				.usMaxOutPwr = 45	//W		100W
			};

	switch(index)
	{
		//退出低功耗模式
		case 0:
		{
			//自带开启写使能
			c_ret = c_usb_sw6303_set_low_power_state(&tUSB_IC2_I2C, true);
			if(c_ret <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC2退出低功耗失败,错误代码%d", c_ret);
				break;
			}
			//没进入低功耗,打开写使能
			// else if(c_ret == 2)
			{
				c_ret = c_usb_sw6303_write_enable(&tUSB_IC2_I2C, true);
				if(c_ret <= 0)
				{
					if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
						log_w("bUsbTask:IC2没进入休眠,写使能失败,错误代码%d", c_ret);
					break;
				}
			}

			uc_lost_cnt = 0;
			index++;
		}
		//校验是否已经初始化
		case 1:
		{
			// s16 s_ret = s_usb_sw6303_get_chg_target_volt(&tUSB_IC2_I2C);
			// if(s_ret < 0)
			// {
			// 	if(uc_lost_cnt < 0xff) 
			// 		uc_lost_cnt++;

			// 	if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
			// 		log_w("bUsbTask:IC2读取校验电压错误,代码%d", s_ret);
			// 	break;
			// }

			// //电压相等,无需初始化
			// if(s_ret == t_param.usChgVolt)
			// {
			// 	//关闭写使能
			// 	c_usb_sw6303_write_enable(&tUSB_IC2_I2C, false);
			// 	return 2;
			// }

			uc_lost_cnt = 0;
			index++;
		}
		//角色设置为放电
		case 2:
		{
			c_ret = c_usb_sw6303_set_typec_role(&tUSB_IC2_I2C, SW6303_TYPEC_ROLE_SRC);
			if(c_ret <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC2角色设置为双向失败,错误代码%d", c_ret);
				break;
			}

			uc_lost_cnt = 0;
			index++;
		}
		//设置保护参数
		case 3:
		{
			c_ret = c_usb_sw6303_set_force_ctrl_param(&tUSB_IC2_I2C, SW6303_CTRL_OBJ_ALL, &t_param);
			if(c_ret <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC2设置保护参数失败,错误代码%d", c_ret);
				break;
			}

			uc_lost_cnt = 0;
			index++;
		}
		//设置放电配置参数
		case 4:
		{
			SW6303_DchgCfg_T t_dchg_cfg = {
				// REG0x100: 放电配置0
				.bTempLoopEnable = false,           // 使能恒温环
				.eTempThr = SW6303_DCHG_TEMP_THR_80C,    // 80°C阈值
				.bPwrAutoDetect = false,           // 手动设置功率
				.eOutPwr = SW6303_OUT_PWR_45W,    // 45W输出功率
				
				// REG0x101: 放电配置1
				.eMultiPortMaxI = SW6303_MULTI_PORT_MAX_I_4_2A,  	// 多口输出最大电流4.2A
				.eVbusIOffset = SW6303_VBUS_I_OFFSET_300MA,        // VBUS限流固定偏移300mA
				.eVbusVOffset = SW6303_VBUS_V_OFFSET_100MV,        // 输出电压固定偏移100mV
				.eLineComp = SW6303_LINE_COMP_60MV_A,            	// 线补设置60mV/A
				
				// REG0x102: 放电配置2
				.bFcLineCompEnable = false,          // 使能快充模式线补
				.bVbusFastOvpEnable = false,        // 打开VBUS快速过压保护
				
				// REG0x103: 放电配置3
				.eDchgIbatDef = SW6303_DCHG_IBAT_12A,    // 放电IBAT限流默认值12A
				.ucLipoUvloThr = 0,                  // 磷酸铁锂电池欠压门限2.75V*N
				
				// REG0x104: 放电配置4
				.eUvloHyst = SW6303_UVLO_HYST_0_4V,        // UVLO迟滞0.4V*N
				.eLipoUvlo = SW6303_LIPO_UVLO_3_0V,        // 三元锂电池欠压门限3.0V*N
				
				// REG0x106: 放电配置5
				.eInductor = SW6303_INDUCTOR_10UH,       // 电感设置10uH
				.eNtcHighThr = SW6303_DCHG_NTC_HIGH_60C,    // 放电NTC高温门限60°C
				.eNtcLowThr = SW6303_DCHG_NTC_LOW_M20C,     // 放电NTC低温门限-20°C
			};
			c_ret = c_usb_sw6303_set_dischg_cfg(&tUSB_IC2_I2C, SW6303_DCHG_CFG_OBJ_ALL, &t_dchg_cfg);
			if(c_ret <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC2设置放电配置参数失败,错误代码%d", c_ret);
				break;
			}

			uc_lost_cnt = 0;
			index++;
		}
		//打开充放电
		case 5:
		{
			// 清除放电异常事件标志
			c_usb_sw6303_clear_abnormal_event(&tUSB_IC2_I2C, SW6303_EVENT_DCHG_ABNORMAL);

			// 触发短按键事件恢复放电 (REG0x20 = 0x01)
			u8 uc_data = 0x01;
			c_usb_sw6303_write_bytes(&tUSB_IC2_I2C, 0x20, &uc_data, 1);

			c_ret = c_usb_sw6303_set_work_mode(&tUSB_IC2_I2C, SW6303_MODE_DISCHG, true);
			if(c_ret <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC2开启充放电失败,错误代码%d", c_ret);
				break;
			}
			// c_usb_cs_ic2_test();
			uc_lost_cnt = 0;
			index = 99;
		}
		break;
		
		default:
			index = 0;
		break;
	}

	//关闭写使能
	c_ret = c_usb_sw6303_write_enable(&tUSB_IC2_I2C, false);
	if(c_ret <= 0)
		return -2;

	if(uc_lost_cnt >= 10)
	{
		if(tUsb.uErrCode.tCode.bIc2Lost == 0)
		{
			bUsb_SetErrCode(UEC_IC2_LOST, true);
		
			if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
				log_e("bUsbTask:IC2丢失");
		}
		
		return -1;
	}
	else 
	{
		if(tUsb.uErrCode.tCode.bIc2Lost == 1)
			bUsb_SetErrCode(UEC_IC2_LOST, false);
		
		if(index == 99)
		{
			index = 0;

			if(uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:IC2初始化完成 \r\n");

			return 1;
		}

		return 0;
	}
}

/*****************************************************************************************************************
-----函数功能    指令:获取参数
-----说明(备注)  通过REG0x30 ADC配置寄存器获取对应的ADC数据
-----传入参数    none
-----输出参数    none
-----返回值      s8: 1:成功  0:处理中  -1:失败
******************************************************************************************************************/
s8 c_usb_cs_get_ic2_param(void)
{
	static u8 index;
	s16 adc_data;
	static u8 uc_lost_cnt = 0;
	static SW6303_PortStatus_T t_State;

	switch (index)
	{
		//读取端口状态
		case 0:
		{
			adc_data = c_usb_sw6303_get_port_status(&tUSB_IC2_I2C, &t_State);
		   	if(adc_data <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
					
				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC2获取NTC电压失败,错误代码%d", adc_data);	
				break;
			}

			// c_usb_cs_ic2_test();

			if(t_State.bC1Online)
				tUsbDev.eC1DevState = DS_WORK;
			else
				tUsbDev.eC1DevState = DS_SHUT_DOWN;

			if(t_State.bAOnline)
				tUsbDev.eUsbADevState = DS_WORK;
			else
				tUsbDev.eUsbADevState = DS_SHUT_DOWN;

			if(t_State.bC1Online == false && t_State.bAOnline == false)
			{
				tUsbDev.usC1Pwr = 0;
				memset(&tUsbIc2, 0, sizeof(tUsbIc2));
				break;
			}
				
			uc_lost_cnt = 0;
			index++;
		}

		//开启写入使能
		case 1:
		{
			adc_data = c_usb_sw6303_write_enable(&tUSB_IC2_I2C, true);
			if(adc_data <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC2设置充放电模式失败,错误代码%d", adc_data);
				break;
			}

			uc_lost_cnt = 0;
			index++;
		}

		case 2:
		{
			// 获取VBUS电压 (8mV/step)
			adc_data = s_usb_sw6303_get_adc_data(&tUSB_IC2_I2C, ADC_SEL_VBUS);
			if(adc_data < 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC2获取VBUS电压失败,错误代码%d", adc_data);	
				break;
			}
			// 转换为mV: ADC数据 * 8mV
			tUsbIc2.usVolt = (u16)(adc_data * 8);
			uc_lost_cnt = 0;
			index++;
		}
		
		// 获取IBUS电流 (4mA/step)
		case 3:
		{
			adc_data = s_usb_sw6303_get_adc_data(&tUSB_IC2_I2C, ADC_SEL_IBUS);
			if(adc_data < 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC2获取IBUS电流失败,错误代码%d", adc_data);	
				break;
			}
			// 转换为mA: ADC数据 * 4mA
			tUsbIc2.usPdCurr = (u16)(adc_data * 4);
			uc_lost_cnt = 0;
			index++;
		}
		
		// 获取电池电压 (7mV/step)
		case 4:
		{
			adc_data = s_usb_sw6303_get_adc_data(&tUSB_IC2_I2C, ADC_SEL_VBAT);
			if(adc_data < 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
					
				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC2获取电池电压失败,错误代码%d", adc_data);	
				break;
			}
			// 转换为mV: ADC数据 * 7mV
			tUsbIc2.usQcCurr = (u16)(adc_data * 7);
			uc_lost_cnt = 0;
			index++;
		}
		
		// 获取IBAT电流 (5mA/step)
		case 5:
		{
			adc_data = s_usb_sw6303_get_adc_data(&tUSB_IC2_I2C, ADC_SEL_IBAT);
			if(adc_data < 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
					
				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC2获取IBAT电流失败,错误代码%d", adc_data);	
				break;
			}
			// 转换为mA: ADC数据 * 5mA
			tUsbIc2.usQcPwr = (u16)(adc_data * 5);
			uc_lost_cnt = 0;
			index++;
		}
		
		// 获取芯片温度 (1/6.82°C)
		case 6:
		{
			// adc_data = s_usb_sw6303_get_adc_data(&tUSB_IC2_I2C, ADC_SEL_TEMP);
			// if(adc_data < 0)
			// {
			// 	if(uc_lost_cnt < 0xff) 
			// 		uc_lost_cnt++;
					
			// 	if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
			// 		log_w("bUsbTask:IC2获取芯片温度失败,错误代码%d", adc_data);	
			// 	break;
			// }
			// // 计算芯片温度
			// float f_temp = adc_data * 6.82f;
			// // tUsbIc2.cTemp = (vs8)f_temp;
			// sMyPrint("IC2 IC温度数据%d \r\n", adc_data);
			uc_lost_cnt = 0;
			index++;
		}

		// 获取NTC电压 (1.1mV)
		case 7:
		{
			adc_data = s_usb_sw6303_get_adc_data(&tUSB_IC2_I2C, ADC_SEL_NTC);
			if(adc_data < 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
					
				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC2获取NTC电压失败,错误代码%d", adc_data);	
				break;
			}
			// 转换为mV
			float f_temp = ((adc_data * 1.1f) / 8) * 100;
			// sMyPrint("IC2 温度数据%.2fR \r\n", f_temp);
			tUsbIc2.cTemp = sNtc_GetTempByRes(ula10K_B3950_RES_TABLE, ntc10K_B3950_INDEX_ZERO_TEMP, ntc10K_B3950_RES_TABLE_SIZE, (u32)f_temp);
			uc_lost_cnt = 0;
			index++;
		}

		// 计算功率
		case 8:
		{
			//关闭写使能
			c_usb_sw6303_write_enable(&tUSB_IC2_I2C, false);

			// 计算VBUS功率: 电压(V) * 电流(A)
			float f_voltage = tUsbIc2.usVolt / 1000.0f;
			float f_current = tUsbIc2.usPdCurr / 1000.0f;
			tUsbIc2.sPower = (vs16)(f_voltage * f_current);
			tUsbDev.usC1Pwr = tUsbIc2.sPower;

			us_usb_total_out_pwr += tUsbIc2.sPower;
			s_total_temp = MAX2(s_total_temp, tUsbIc2.cTemp);

			// c_usb_sw6303_test(&tUSB_IC2_I2C);
			
			index = 0;
			
			if(uPrint.tFlag.bUsbTask)
				log_i("bUsbTask:IC2 SW6303 VBUS=%dmV IBUS=%dmA 功率=%dW 温度=%d°C", 
					tUsbIc2.usVolt, tUsbIc2.usPdCurr, tUsbIc2.sPower, tUsbIc2.cTemp);
		}
		break;

		default:
			index = 0;
			break ;
	}
	
	//处理状态
	if(uc_lost_cnt >= 16)
	{
		if(tUsb.uErrCode.tCode.bIc2Lost == 0)
		{
			c_usb_sw6303_write_enable(&tUSB_IC2_I2C, false);

			bUsb_SetErrCode(UEC_IC2_LOST, true);
		
			if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
				log_e("bUsbTask:IC2丢失");	
		}
		return -1;
	}
	else if(!uc_lost_cnt)
	{
		if(tUsb.uErrCode.tCode.bIc2Lost == 1)
			bUsb_SetErrCode(UEC_IC2_LOST, false);
		return 1;
	}
	else
		return 0;
}

s8 c_usb_cs_ic2_test(void)
{
	//写低寄存器
	u8 data[1] = {0};
	u16 reg_addr = 0;
	I2cObj_T *p_i2c_obj = &tUSB_IC2_I2C;

	reg_addr = 0x23;
	cI2C_ReadBytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("2REG0x%x %x \r\n", reg_addr, data[0]);

	reg_addr = 0x24;
	c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);

	reg_addr = 0x18;
	c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);

	reg_addr = 0x19;
	c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);

	reg_addr = 0x28;
	c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);

	reg_addr = 0x2A;
	c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);

	reg_addr = 0x2B;
	c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);

	reg_addr = 0x2C;
	c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);
	return 1;
}


#endif  //1
