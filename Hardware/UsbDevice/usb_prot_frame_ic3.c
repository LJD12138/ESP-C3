/***********************************************************************************************************************
-----@file usb_prot_frame_ic3.c
-----函数功能   SW6303 双向PD 100W
-----作者       LJD
-----版本 1.0.0
-----日期       2026-01-21
-----@copyright Copyright (c) 2026  深圳市百酷新能源有限公司
-----@par 修改日志:1.如果要操作高位寄存器,0x24的写使能开启后,可以先写低位寄存器,再打开高位模式,再写高位
					2.要读取高位寄存器.也是如1的操作,单独把高位模式打开无法读取
************************************************************************************************************************/
#include "UsbDevice/usb_prot_frame.h"

#if(1)
#include "UsbDevice/usb_sw6303_common.h"
#include "UsbDevice/usb_queue_task.h"
#include "UsbDevice/usb_task.h"
#include "UsbDevice/usb_iface.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"

#include "string.h"
#include "math.h"
#include "ntc.h"

//****************************************************参数初始化**************************************************//
USB_IC_T tUsbIc3 = {0}; //USB芯片

// I2C写操作使能状态管理
static bool s_usb_ic3_i2c_write_enabled = false;  // I2C写操作使能状态

// 外部I2C对象定义（需要在usb_iface.c中定义）
extern I2cObj_T tUSB_IC3_I2C;

//****************************************************参数初始化**************************************************//


/*****************************************************************************************************************
-----函数功能    指令:初始化IC
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      s8: 1:成功  0:处理中  -1:失败
******************************************************************************************************************/
s8 c_usb_cs_ic3_init(void)
{
	s8 c_ret = 0;
	static u8 index = 0;
	static u8 uc_lost_cnt = 0;

	SW6303_ForceCtrl_T t_param = {
			.usVbus = 20000,	//mV 	20V
			.usChgVolt = 14600,	//mV  	14.6V
			.usMaxInPwr = 30,	//W		100W
			.usChgHoldVolt = 22000,	//mV 22V
			.usIbat = 100,		//0.1A	10A
			.usIbus = 1000,		//0.01A	10A	
			.usMaxOutPwr = 100	//W		100W
		};

	switch(index)
	{
		//退出低功耗模式
		case 0:
		{
			c_ret = c_usb_sw6303_set_low_power_state(&tUSB_IC3_I2C, true);
			if(c_ret <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC3退出低功耗失败,错误代码%d", c_ret);
				break;
			}
			//没进入低功耗,打开写使能
			// else if(c_ret == 2)
			{
				c_ret = c_usb_sw6303_write_enable(&tUSB_IC3_I2C, true);
				if(c_ret <= 0)
				{
					if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
						log_w("bUsbTask:IC3没进入休眠,写使能失败,错误代码%d", c_ret);
					break;
				}
			}

			uc_lost_cnt = 0;
			index++;
		}
		//校验是否已经初始化
		case 1:
		{
			// s16 s_ret = s_usb_sw6303_get_chg_target_volt(&tUSB_IC3_I2C);
			// if(s_ret < 0)
			// {
			// 	if(uc_lost_cnt < 0xff) 
			// 		uc_lost_cnt++;

			// 	if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
			// 		log_w("bUsbTask:IC3读取校验电压错误,代码%d", s_ret);
			// 	break;
			// }

			// //电压相等,无需初始化
			// if(s_ret == t_param.usChgVolt)
			// {
			// 	//关闭写使能
			// 	c_usb_sw6303_write_enable(&tUSB_IC3_I2C, false);
			// 	return 2;
			// }

			uc_lost_cnt = 0;
			index++;
		}
		//角色设置为双向
		case 2:
		{
			c_ret = c_usb_sw6303_set_typec_role(&tUSB_IC3_I2C, SW6303_TYPEC_ROLE_DRP);
			if(c_ret <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC3角色设置为双向失败,错误代码%d", c_ret);
				break;
			}

			uc_lost_cnt = 0;
			index++;
		}
		//设置保护参数
		case 3:
		{
			c_ret = c_usb_sw6303_set_force_ctrl_param(&tUSB_IC3_I2C, SW6303_CTRL_OBJ_ALL, &t_param);
			if(c_ret <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC3设置保护参数失败,错误代码%d", c_ret);
				break;
			}
			
			uc_lost_cnt = 0;
			index++;
		}
		//设置充电配置参数
		case 4:
		{
			SW6303_ChgCfg_T t_chg_cfg = {
				// REG0x107: 充电配置0
				.bFcFixPower = false,               // 快充按最大电流充电
				.bAutoDetectInPwr = false,         	// 手动设置输入功率
				.eInPwr = SW6303_CHG_IN_PWR_65W,     // 输入最高功率65W
				
				// REG0x108: 充电配置1
				.bAutoDetectBatType = false,       // 手动设置电池类型
				.eBatType = SW6303_BAT_TYPE_3_65V_1,   // 电池类型3.65V
				.bAutoDetectBatCell = false,       // 手动设置电池节数
				.eBatCell = SW6303_BAT_CELL_4,         // 电池节数4节
				
				// REG0x109: 充电配置2
				.bChgAbove1_5V = false,           // 电池电压低于1.5V时允许充电
				.e5VOvp = SW6303_5V_OVP_4_6V,       // 5V限压门限4.6V
				.bTrickleTimeout = false,           // 涓流充电超时40分钟
				.e9VOvp = SW6303_9V_OVP_8_5V,       // 9V限压门限8.5V
				
				// REG0x10A: 充电配置3
				.bAdapterPullDisable = false,       // 使能适配器拉挂
				.e10VOvp = SW6303_10V_OVP_9_5V,      // 10V限压门限9.5V
				.e12VOvp = SW6303_12V_OVP_11_5V,     // 12V限压门限11.5V
				
				// REG0x10B: 充电配置4
				.bBypassHoldChange = true,          // 边充边放hold门限不变
				.e15VOvp = SW6303_15V_OVP_14_5V,     // 15V限压门限14.5V
				.e20VOvp = SW6303_20V_OVP_19_5V,     // 20V限压门限19.5V
				
				// REG0x10C: 充电配置5
				.bLimitVoltageAdapt = false,       // 限压门限自适应（电流>3A降低限压）
				
				// REG0x10D: 充电配置6
				.eFeTrickle = SW6303_FE_TRICKLE_2_75V,      // 磷酸铁锂涓流充电门限2.75V*N
				.eTrickleI = SW6303_TRICKLE_I_100MA,       // 涓流充电电流100mA
				.eTrickleHyst = SW6303_TRICKLE_HYST_0_1V,    // 涓流充电电压迟滞0.1V*N
				.eCcTimeout = SW6303_CC_TIMEOUT_22H,        // 恒流充电超时22h
				
				// REG0x10E: 充电配置7
				.eChgEndI = SW6303_CHG_END_I_200MA,        // 充电截至电流200mA
				.eC5VI = SW6303_C_5V_I_3_0A,           // C口5V充电电流3.0A
				
				// REG0x10F: 充电配置8
				.eBL5VI = SW6303_BL_5V_I_2_0A,         // B口和L口5V充电电流2.0A
				.eChgNtcLow = SW6303_CHG_NTC_LOW_0C,      // 充电NTC低温保护门限0°C
				.eChgNtcHigh = SW6303_CHG_NTC_HIGH_45C,    // 充电NTC高温保护门限45°C
			};
			c_ret = c_usb_sw6303_set_chg_cfg(&tUSB_IC3_I2C, SW6303_CHG_CFG_OBJ_ALL, &t_chg_cfg);
			if(c_ret <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC3设置充电配置参数失败,错误代码%d", c_ret);
				break;
			}

			uc_lost_cnt = 0;
			index++;
		}
		//设置放电配置参数
		case 5:
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
			c_ret = c_usb_sw6303_set_dischg_cfg(&tUSB_IC3_I2C, SW6303_DCHG_CFG_OBJ_ALL, &t_dchg_cfg);
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
		case 6:
		{
			c_ret = c_usb_sw6303_set_work_mode(&tUSB_IC3_I2C, SW6303_MODE_ALL, true);
			if(c_ret <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC3开启充放电失败,错误代码%d", c_ret);
				break;
			}

			uc_lost_cnt = 0;
			index = 99;
		}
		break;
		
		default:
			index = 0;
		break;
	}

	//关闭写使能
	c_ret = c_usb_sw6303_write_enable(&tUSB_IC3_I2C, false);
	if(c_ret <= 0)
		return -2;

	if(uc_lost_cnt >= 10)
	{
		if(tUsb.uErrCode.tCode.bIc2Lost == 0)
		{
			bUsb_SetErrCode(UEC_IC2_LOST, true);
		
			if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
				log_e("bUsbTask:IC3丢失");
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
				sMyPrint("bUsbTask:IC3初始化完成 \r\n");

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
s8 c_usb_cs_get_ic3_param(void)
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
			adc_data = c_usb_sw6303_get_port_status(&tUSB_IC3_I2C, &t_State);
		   	if(adc_data <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
					
				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC3获取NTC电压失败,错误代码%d", adc_data);	
				break;
			}

			if(t_State.bC1Online)
				tUsbDev.eC2DevState = DS_WORK;
			else
				tUsbDev.eC2DevState = DS_SHUT_DOWN;

			if(t_State.bC1Online == false)
			{
				tUsb.usInPwr = 0;
				tUsbDev.usC2Pwr = 0;
				memset(&tUsbIc3, 0, sizeof(tUsbIc3));
				break;
			}
				
			uc_lost_cnt = 0;
			index++;
		}

		//开启写入使能
		case 1:
		{
			adc_data = c_usb_sw6303_write_enable(&tUSB_IC3_I2C, true);
			if(adc_data <= 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC3设置充放电模式失败,错误代码%d", adc_data);
				break;
			}

			uc_lost_cnt = 0;
			index++;
		}

		case 2:
		{
			// 获取VBUS电压 (8mV/step)
			adc_data = s_usb_sw6303_get_adc_data(&tUSB_IC3_I2C, ADC_SEL_VBUS);
			if(adc_data < 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC3获取VBUS电压失败,错误代码%d", adc_data);	
				break;
			}
			// 转换为mV: ADC数据 * 8mV
			tUsbIc3.usVolt = (u16)(adc_data * 8);
			uc_lost_cnt = 0;
			index++;
		}
		
		// 获取IBUS电流 (4mA/step)
		case 3:
		{
			adc_data = s_usb_sw6303_get_adc_data(&tUSB_IC3_I2C, ADC_SEL_IBUS);
			if(adc_data < 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;

				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC3获取IBUS电流失败,错误代码%d", adc_data);	
				break;
			}
			// 转换为mA: ADC数据 * 4mA
			tUsbIc3.usPdCurr = (u16)(adc_data * 4);
			uc_lost_cnt = 0;
			index++;
		}
		
		// 获取电池电压 (7mV/step)
		case 4:
		{
			adc_data = s_usb_sw6303_get_adc_data(&tUSB_IC3_I2C, ADC_SEL_VBAT);
			if(adc_data < 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
					
				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC3获取电池电压失败,错误代码%d", adc_data);	
				break;
			}
			// 转换为mV: ADC数据 * 7mV
			tUsbIc3.usQcCurr = (u16)(adc_data * 7);
			uc_lost_cnt = 0;
			index++;
		}
		
		// 获取IBAT电流 (5mA/step)
		case 5:
		{
			adc_data = s_usb_sw6303_get_adc_data(&tUSB_IC3_I2C, ADC_SEL_IBAT);
			if(adc_data < 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
					
				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC3获取IBAT电流失败,错误代码%d", adc_data);	
				break;
			}
			// 转换为mA: ADC数据 * 5mA
			tUsbIc3.usQcPwr = (u16)(adc_data * 5);
			uc_lost_cnt = 0;
			index++;
		}
		
		// 获取芯片温度 (1/6.82°C)
		case 6:
		{
			// adc_data = s_usb_sw6303_get_adc_data(&tUSB_IC3_I2C, ADC_SEL_TEMP);
			// if(adc_data < 0)
			// {
			// 	if(uc_lost_cnt < 0xff) 
			// 		uc_lost_cnt++;
					
			// 	if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
			// 		log_w("bUsbTask:IC3获取芯片温度失败,错误代码%d", adc_data);	
			// 	break;
			// }
			// // 计算芯片温度
			// float f_temp = adc_data * 6.82f;
			// // tUsbIc3.cTemp = (vs8)f_temp;
			uc_lost_cnt = 0;
			index++;
		}

		// 获取NTC电压 (1.1mV)
		case 7:
		{
			adc_data = s_usb_sw6303_get_adc_data(&tUSB_IC3_I2C, ADC_SEL_NTC);
			if(adc_data < 0)
			{
				if(uc_lost_cnt < 0xff) 
					uc_lost_cnt++;
					
				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_w("bUsbTask:IC3获取NTC温度失败,错误代码%d", adc_data);	
				break;
			}
			// 转换为mV
			float f_temp = ((adc_data * 1.1f) / 8) * 100;
			sMyPrint("IC3 温度数据 %dmV %.2fR \r\n", adc_data, f_temp);
			tUsbIc3.cTemp = sNtc_GetTempByRes(ula10K_B3950_RES_TABLE, ntc10K_B3950_INDEX_ZERO_TEMP, ntc10K_B3950_RES_TABLE_SIZE, (u32)f_temp);
			uc_lost_cnt = 0;
			index++;
		}
		
		// 计算功率
		case 8:
		{
			//关闭写使能
			c_usb_sw6303_write_enable(&tUSB_IC3_I2C, false);

			// 计算VBUS功率: 电压(V) * 电流(A)
			float f_voltage = tUsbIc3.usVolt / 1000.0f;
			float f_current = tUsbIc3.usPdCurr / 1000.0f;
			tUsbIc3.sPower = (vs16)(f_voltage * f_current);
			tUsbDev.usC2Pwr = tUsbIc3.sPower;

			if(t_State.bC1SinkConn)
			{
				tUsb.usInPwr = tUsbIc3.sPower;
			}
			else if (t_State.bC1SrcConn)
			{
				tUsb.usInPwr = 0;
				us_usb_total_out_pwr += tUsbIc3.sPower;
			}
			else
				tUsb.usInPwr = 0;

			
			s_total_temp = MAX2(s_total_temp, tUsbIc3.cTemp);

			if(t_State.bC1Online)
				tUsbDev.eC2DevState = DS_WORK;
			else
				tUsbDev.eC2DevState = DS_SHUT_DOWN;
			
			index = 0;
			
			if(uPrint.tFlag.bUsbTask)
				log_i("bUsbTask:IC3 SW6303 VBUS=%dmV IBUS=%dmA 功率=%dW 温度=%d°C", 
					tUsbIc3.usVolt, tUsbIc3.usPdCurr, tUsbIc3.sPower, tUsbIc3.cTemp);
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
			//关闭写功能
			c_usb_sw6303_write_enable(&tUSB_IC3_I2C, false);

			bUsb_SetErrCode(UEC_IC2_LOST, true);
		
			if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
				log_e("bUsbTask:IC3丢失");	
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

/*****************************************************************************************************************
-----函数功能    指令:设置IC3充电功率
-----说明(备注)  通过REG0x45设置输入最大功率
				当us_req_chg_pwr=0时，关闭充电功率并关闭充电功能
				当us_req_chg_pwr非零时，设置充电功率
-----传入参数    us_req_chg_pwr 充电功率，单位W
-----输出参数    none
-----返回值      s8: 1:成功  2:无需修改  -1:失败
******************************************************************************************************************/
s8 c_usb_cs_set_chg_pwr(u8 uc_req_chg_pwr)
{
	s8 c_ret = 0;
	u8 data[1] = {0};

	/* 读取当前REG0x45值 */
	c_ret = c_usb_sw6303_read_bytes(&tUSB_IC3_I2C, 0x45, data, sizeof(data));
	if(c_ret <= 0)
		return -1;

	/* 判断是否需要修改 */
	if(data[0] == uc_req_chg_pwr)
		return 2;

	/* 开启写入功能 */
	if(c_usb_sw6303_write_enable(&tUSB_IC3_I2C, true) <= 0)
		return -2;

	/* 设置充电功率：写入REG0x45 */
	data[0] = uc_req_chg_pwr;
	c_ret = c_usb_sw6303_write_bytes(&tUSB_IC3_I2C, 0x45, data, 1);
	if(c_ret <= 0)
	{
		c_usb_sw6303_write_enable(&tUSB_IC3_I2C, false);
		return -3;
	}

	/* 当充电功率为0时，关闭充电功能 */
	if(uc_req_chg_pwr == 0)
	{
		c_ret = c_usb_sw6303_set_work_mode(&tUSB_IC3_I2C, SW6303_MODE_CHG, false);
		if(c_ret <= 0)
			return -5;
		
		if(uPrint.tFlag.bUsbTask)
			sMyPrint("bUsbTask:IC3关闭充电功率和充电功能\r\n");
	}
	else
	{
		/* 非零时确保充电功能打开 */
		c_ret = c_usb_sw6303_set_work_mode(&tUSB_IC3_I2C, SW6303_MODE_CHG, true);
		if(c_ret <= 0 && c_ret != 2)  /* c_ret=2表示状态一致 */
			return -6;
		
		if(uPrint.tFlag.bUsbTask)
			sMyPrint("bUsbTask:IC3设置充电功率=%dW\r\n", uc_req_chg_pwr);
	}

	/* 关闭写入功能 */
	if(c_usb_sw6303_write_enable(&tUSB_IC3_I2C, false) <= 0)
		return -4;

	return 1;
}


s8 c_usb_cs_ic3_test(void)
{
	//写低寄存器
	u8 data[1] = {0};
	u16 reg_addr = 0;
	I2cObj_T *p_i2c_obj = &tUSB_IC3_I2C;

	reg_addr = 0x23;
	cI2C_ReadBytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("2REG0x%x %x \r\n", reg_addr, data[0]);

	reg_addr = 0x24;
	c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);

	reg_addr = 0x0E;
	c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);

	reg_addr = 0x10;
	c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);

	reg_addr = 0x11;
	c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);

	// c_usb_sw6303_write_enable(p_i2c_obj, true);

	// SW6303_ForceCtrl_T t_param = {
	// 		.usVbus = 20000,	//mV 	20V
	// 		.usChgVolt = 14600,	//mV  	14.6V
	// 		.usMaxInPwr = 30,	//W		100W
	// 		.usChgHoldVolt = 22000,	//mV 22V
	// 		.usIbat = 100,		//0.1A	10A
	// 		.usIbus = 1000,		//0.01A	10A	
	// 		.usMaxOutPwr = 100	//W		100W
	// 	};
	// c_usb_sw6303_set_force_ctrl_param(&tUSB_IC3_I2C, SW6303_CTRL_OBJ_ALL, &t_param);

	reg_addr = 0x40;
	c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);

	reg_addr = 0x45;
	c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);

	reg_addr = 0x46;
	c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);

	reg_addr = 0x47;
	c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);


	// SW6303_ChgCfg_T t_chg_cfg = {
	// 			// REG0x107: 充电配置0
	// 			.bFcFixPower = false,               // 快充按最大电流充电
	// 			.bAutoDetectInPwr = false,         	// 手动设置输入功率
	// 			.eInPwr = SW6303_CHG_IN_PWR_65W,     // 输入最高功率65W
				
	// 			// REG0x108: 充电配置1
	// 			.bAutoDetectBatType = false,       // 手动设置电池类型
	// 			.eBatType = SW6303_BAT_TYPE_3_65V_1,   // 电池类型3.65V
	// 			.bAutoDetectBatCell = false,       // 手动设置电池节数
	// 			.eBatCell = SW6303_BAT_CELL_3,         // 电池节数4节
				
	// 			// REG0x109: 充电配置2
	// 			.bChgAbove1_5V = false,           // 电池电压低于1.5V时允许充电
	// 			.e5VOvp = SW6303_5V_OVP_4_6V,       // 5V限压门限4.6V
	// 			.bTrickleTimeout = false,           // 涓流充电超时40分钟
	// 			.e9VOvp = SW6303_9V_OVP_8_5V,       // 9V限压门限8.5V
				
	// 			// REG0x10A: 充电配置3
	// 			.bAdapterPullDisable = false,       // 使能适配器拉挂
	// 			.e10VOvp = SW6303_10V_OVP_9_5V,      // 10V限压门限9.5V
	// 			.e12VOvp = SW6303_12V_OVP_11_5V,     // 12V限压门限11.5V
				
	// 			// REG0x10B: 充电配置4
	// 			.bBypassHoldChange = true,          // 边充边放hold门限不变
	// 			.e15VOvp = SW6303_15V_OVP_14_5V,     // 15V限压门限14.5V
	// 			.e20VOvp = SW6303_20V_OVP_19_5V,     // 20V限压门限19.5V
				
	// 			// REG0x10C: 充电配置5
	// 			.bLimitVoltageAdapt = false,       // 限压门限自适应（电流>3A降低限压）
				
	// 			// REG0x10D: 充电配置6
	// 			.eFeTrickle = SW6303_FE_TRICKLE_2_75V,      // 磷酸铁锂涓流充电门限2.75V*N
	// 			.eTrickleI = SW6303_TRICKLE_I_100MA,       // 涓流充电电流100mA
	// 			.eTrickleHyst = SW6303_TRICKLE_HYST_0_1V,    // 涓流充电电压迟滞0.1V*N
	// 			.eCcTimeout = SW6303_CC_TIMEOUT_22H,        // 恒流充电超时22h
				
	// 			// REG0x10E: 充电配置7
	// 			.eChgEndI = SW6303_CHG_END_I_200MA,        // 充电截至电流200mA
	// 			.eC5VI = SW6303_C_5V_I_3_0A,           // C口5V充电电流3.0A
				
	// 			// REG0x10F: 充电配置8
	// 			.eBL5VI = SW6303_BL_5V_I_2_0A,         // B口和L口5V充电电流2.0A
	// 			.eChgNtcLow = SW6303_CHG_NTC_LOW_0C,      // 充电NTC低温保护门限0°C
	// 			.eChgNtcHigh = SW6303_CHG_NTC_HIGH_45C,    // 充电NTC高温保护门限45°C
	// 		};
	// c_usb_sw6303_set_chg_cfg(&tUSB_IC3_I2C, SW6303_CHG_CFG_OBJ_ALL, &t_chg_cfg);

	// reg_addr = 0x107;
	// c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	// sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);

	// reg_addr = 0x108;
	// c_usb_sw6303_read_bytes(p_i2c_obj, reg_addr, data, sizeof(data));
	// sMyPrint("REG0x%x %x \r\n", reg_addr, data[0]);

	// // c_usb_sw6303_high_reg_addr_enable(p_i2c_obj, false);
	// c_usb_sw6303_write_enable(p_i2c_obj, false);
	// cI2C_ReadBytes(p_i2c_obj, SW6303_REG_I2C_ENABLE, data, 1);
	// sMyPrint("SW6303: After high_addr REG0x24=0x%x\r\n", data[0]);

	/* 如果已经使能，直接返回 */
	return 1;
}

#endif  //1
