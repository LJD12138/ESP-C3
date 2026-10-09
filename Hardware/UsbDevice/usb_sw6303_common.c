/***********************************************************************************************************************
-----@file sw6303_common.c
-----函数功能   SW6303 芯片通用驱动
-----作者       LJD
-----版本 1.0.0
-----日期       2026-01-22
-----@copyright Copyright (c) 2026  深圳市百酷新能源有限公司
-----@par 修改日志:
************************************************************************************************************************/
#include "UsbDevice/usb_sw6303_common.h"
#include "UsbDevice/usb_iface.h"
#include "Print/print_task.h"
#include "string.h"

/*****************************************************************************************************************
-----函数功能    读数据(有寄存器地址)
-----说明(备注)  默认是操作低位,如果需要操作高位,就需要打开操作完后马上关闭
				 读取高位寄存器时候也是需要开启写使能
-----传入参数    p_i2c_obj:IIC对象
				 reg_addr:寄存器地址
				 buf:数据的指针
				 len:数据的字节长度
-----输出参数    none
-----返回值      小于0:操作失败   等于0:等待    大于0:读取成功
******************************************************************************************************************/
s8 c_usb_sw6303_read_bytes(const I2cObj_T *p_i2c_obj, u16 reg_addr, u8 *buf, u8 len)
{
	s8 c_ret = 0;
	u8 addr = (u8)reg_addr;
	bool en = false;

	if(p_i2c_obj == NULL ||
		buf == NULL ||
		len == 0)
		return -1;

	//高地址模式
	if(reg_addr >= 0x100)
		en = true;

	if(en == true)
	{
		c_ret = c_usb_sw6303_high_reg_addr_enable(p_i2c_obj, true);
		if(c_ret <= 0)
		{
			if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
				log_w("SW6303:设置高位地址模式失败,错误代码%d", c_ret);
			return -2;
		}
	}
	
	c_ret = cI2C_ReadBytes(p_i2c_obj, addr, buf, len);
	if(c_ret <= 0)
	{
		if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
			log_w("SW6303:读取地址0x%x失败,错误代码%d ", reg_addr, c_ret);
		return -3;
	}

	if(en == true)
	{
		c_ret = c_usb_sw6303_high_reg_addr_enable(p_i2c_obj, false);
		if(c_ret <= 0)
		{
			c_usb_sw6303_high_reg_addr_enable(p_i2c_obj, false);
			if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
				log_w("SW6303:重置高位地址模式失败,错误代码%d", c_ret);
			return -4;
		}
	}

	return 1;
}

/*****************************************************************************************************************
-----函数功能    写数据(有寄存器地址)
-----说明(备注)  默认是操作低位,如果需要操作高位,就需要打开操作完后马上关闭
				 需要格外打开写使能
-----传入参数    p_i2c_obj:IIC对象
				 reg_addr:寄存器地址
				 buf:数据的指针
				 len:数据的字节长度
-----输出参数    none
-----返回值      小于0:操作失败   等于0:等待    大于0:写入成功
******************************************************************************************************************/
s8 c_usb_sw6303_write_bytes(const I2cObj_T *p_i2c_obj, u16 reg_addr, const u8 *buf, u8 len)
{
	s8 c_ret = 0;
	u8 addr = (u8)reg_addr;
	bool en = false;

	if(p_i2c_obj == NULL ||
		buf == NULL ||
		len == 0)
		return -1;

	//高地址模式
	if(reg_addr >= 0x100)
		en = true;

	if(en == true)
	{
		c_ret = c_usb_sw6303_high_reg_addr_enable(p_i2c_obj, true);
		if(c_ret <= 0)
		{
			if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
				log_w("SW6303:设置高位地址模式失败,错误代码%d", c_ret);
			return -2;
		}
	}

	c_ret = cI2C_WriteBytes(p_i2c_obj, addr, buf, len);
	if(c_ret <= 0)
	{
		if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
			log_w("SW6303:写入地址0x%x失败,错误代码%d ", reg_addr, c_ret);
		return -3;
	}

	if(en == true)
	{
		c_ret = c_usb_sw6303_high_reg_addr_enable(p_i2c_obj, false);
		if(c_ret <= 0)
		{
			c_usb_sw6303_high_reg_addr_enable(p_i2c_obj, false);
			if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
				log_w("SW6303:重置高位地址模式失败,错误代码%d", c_ret);
			return -4;
		}
	}

	return 1;
}

/***********************************************************************************************************************
-----函数功能	SW6303 I2C使能操作高位地址寄存器 reg24 reg1FF
-----说明		bit0
				若要操作 REG0x100~REG0x156 时， 需写此 bit 为 1 若此 bit
				为 1 时，要操作低于 REG0x100 的地址时，向 REG0x1FF 写 0
				即可将此 bit 清零,reg24的其他bit不会变。
-----传入参数   p_i2c_obj: I2C对象
				enable  true:操作高位寄存器    false:操作低位寄存器
-----返回值     s8  1:成功    -1:失败
-----作者       LJD
-----日期       2026-01-22
************************************************************************************************************************/
s8 c_usb_sw6303_high_reg_addr_enable(const I2cObj_T *p_i2c_obj, bool en)
{
	u8 data[1] = {0};

	if(en == true)
	{
		data[0] = 0x81;
		if(cI2C_WriteBytes(p_i2c_obj, SW6303_REG_I2C_ENABLE, data, sizeof(data)) <= 0)
			return -1;

		//校验是否正确的开启高位模式
		if(cI2C_ReadBytes(p_i2c_obj, SW6303_REG_I2C_ENABLE, data, sizeof(data)) > 0)
		{
			if(data[0] != 0x81)
				return -2;
		}
	}
	else
	{
		data[0] = 0;
		if(cI2C_WriteBytes(p_i2c_obj, SW6303_REG_I2C_ENABLE_8BIT_CLR, data, sizeof(data)) <= 0)
			return -3;

		//校验是否正确的关闭高位模式
		if(cI2C_ReadBytes(p_i2c_obj, SW6303_REG_I2C_ENABLE, data, sizeof(data)) > 0)
		{
			if(data[0] != 0x80)
				return -4;
		}
	}

	return 1;
}

/***********************************************************************************************************************
-----函数功能	SW6303 写操作使能封装 reg24
-----说明(备注) bit5-7
				I2C 写操作使能，如果要写其他寄存器（除 REG0x20~0x24 之
				外），需要先执行如下操作：
				1) 写 REG0x24=0x20
				2) 写 REG0x24=0x40 
				3) 写 REG0x24=0x80
-----传入参数   p_i2c_obj: I2C对象
				en  true:开始写入 false:关闭写入  
-----返回值     s8  1:成功    -1:失败
-----作者       LJD
-----日期       2026-01-22
************************************************************************************************************************/
s8 c_usb_sw6303_write_enable(const I2cObj_T *p_i2c_obj, bool en)
{
	u8 data[1] = {0};
	
	/* 执行I2C写操作使能：写 REG0x24=0x20, 0x40, 0x80 */
	if(en)
	{
		/* 写 REG0x24 = 0x20 */
		data[0] = 0x20;
		if(cI2C_WriteBytes(p_i2c_obj, SW6303_REG_I2C_ENABLE, data, sizeof(data)) <= 0)
			return -1;
		
		/* 写 REG0x24 = 0x40 */
		data[0] = 0x40;
		if(cI2C_WriteBytes(p_i2c_obj, SW6303_REG_I2C_ENABLE, data, sizeof(data)) <= 0)
			return -2;
		
		/* 写 REG0x24 = 0x80 */
		data[0] = 0x80;
		if(cI2C_WriteBytes(p_i2c_obj, SW6303_REG_I2C_ENABLE, data, sizeof(data)) <= 0)
			return -3;
		
		//校验是否正确的开启
		if(cI2C_ReadBytes(p_i2c_obj, SW6303_REG_I2C_ENABLE, data, sizeof(data)) > 0)
		{
			if(data[0] != 0x80)
				return -4;
		}
	}
	else
	{
		/* 退出I2C写操作使能：写REG0x1FF=0x00 */
		data[0] = 0x00;
		if(cI2C_WriteBytes(p_i2c_obj, SW6303_REG_I2C_ENABLE, data, sizeof(data)) <= 0)
			return -5;
	}
	
	/* 如果已经使能，直接返回 */
	return 1;
}

/***********************************************************************************************************************
-----函数功能	指令：设置低功耗状态 reg23
-----说明		REG0x23:
				bit0: 关机低功耗使能 (0:打开低功耗, 1:关闭低功耗) - 可写
				bit1: 低功耗的状态 (0:处于低功耗状态, 1:退出低功耗状态) - 只读状态位
				要退出低功耗，需要：
				1) REG0x23 写 0x01 关闭低功耗
				2) REG0x24 写 0x20，0x40，0x80，解锁 I2C 写操作
-----传入参数   p_i2c_obj: I2C对象
				en  true:退出低功耗    false:进入低功耗
-----返回值     s8  1:成功    -1:失败
-----作者       LJD
-----日期       2026-01-22
************************************************************************************************************************/
s8 c_usb_sw6303_set_low_power_state(const I2cObj_T *p_i2c_obj, bool en)
{
	u8 data[1] = {0};
	const u8 reg_state_value = 0x03;
	const u8 reg_value = 0x01;

	/* 读取当前REG0x23寄存器值 */
	if(cI2C_ReadBytes(p_i2c_obj, SW6303_REG_LOW_POWER_SET, data, sizeof(data)) <= 0)
		return -1;
	
	// bit1 - 只读状态位
	// 低功耗的状态
	// 0：处于低功耗状态
	// 1：退出低功耗状态
	bool low_en = ((data[0] & reg_state_value) == reg_state_value)? true : false;
	if((en == low_en))
		return 2;

	// bit0 - 控制位
	// 关机低功耗使能
	// 0：打开低功耗
	// 1：关闭低功耗
	if(en == true)
	{
		/* 退出低功耗：bit0=1 */
		data[0] = reg_value;
		if(cI2C_WriteBytes(p_i2c_obj, SW6303_REG_LOW_POWER_SET, data, sizeof(data)) <= 0)
			return -3;

		/* 执行REG0x24解锁序列，使bit1状态位生效 */
		data[0] = 0x20;
		if(cI2C_WriteBytes(p_i2c_obj, SW6303_REG_I2C_ENABLE, data, sizeof(data)) <= 0)
			return -4;
		
		data[0] = 0x40;
		if(cI2C_WriteBytes(p_i2c_obj, SW6303_REG_I2C_ENABLE, data, sizeof(data)) <= 0)
			return -5;
		
		data[0] = 0x80;
		if(cI2C_WriteBytes(p_i2c_obj, SW6303_REG_I2C_ENABLE, data, sizeof(data)) <= 0)
			return -6;
	}
	else
	{
		/* 进入低功耗：bit0=0 */
		data[0] = 0x00;
		if(cI2C_WriteBytes(p_i2c_obj, SW6303_REG_LOW_POWER_SET, data, sizeof(data)) <= 0)
			return -7;
	}

	/* 读取校验bit1状态位 */
	if(cI2C_ReadBytes(p_i2c_obj, SW6303_REG_LOW_POWER_SET, data, sizeof(data)) <= 0)
		return -8;

	low_en = ((data[0] & reg_state_value) == reg_state_value)? true : false;
	if((en != low_en))
		return -9;

	if(uPrint.tFlag.bUsbTask)
		sMyPrint("bUsbTask:-------退出低功耗成功---------\r\n");

	return 1;
}

/***********************************************************************************************************************
-----函数功能	指令：设置TYPEC角色
-----传入参数   p_i2c_obj: I2C对象
				role  TYPEC角色 (DRP/SRC/SNK/AUTO)
-----返回值     s8  1:成功    -1:失败
-----作者       LJD
-----日期       2026-01-22
************************************************************************************************************************/
s8 c_usb_sw6303_set_typec_role(const I2cObj_T *p_i2c_obj, SW6303_TYPEC_ROLE_E role)
{
	u8 data[1] = {0};
	u8 target_value;
	
	/* 读取当前REG0x132寄存器值 */
	if(c_usb_sw6303_read_bytes(p_i2c_obj, SW6303_REG_TYPEC_SET1, data, sizeof(data)) <= 0)
		return -1;

	/* 根据角色参数计算目标值
	 * 假设bit[1:0]用于设置TYPEC角色:
	 * 00 = DRP (双角色)
	 * 01 = SRC (放电)
	 * 10 = SNK (充电))
	 */
	target_value = (data[0] & 0xFC) | (u8)role;

	/* 判断当前角色是否一致，一致则不设置 */
	if((data[0] & 0x03) == ((u8)role & 0x03))
		return 2;

	/* 角色不一致，设置新值 */
	data[0] = target_value;
	if(c_usb_sw6303_write_bytes(p_i2c_obj, SW6303_REG_TYPEC_SET1, data, sizeof(data)) <= 0)
		return -2;

	return 1;
}

/***********************************************************************************************************************
-----函数功能	设置强制控制参数
-----传入参数   p_i2c_obj: I2C对象
				obj: 设置的对象（输出/输入/充电/放电/充电最大功率）
				param: 设置对象的参数
-----返回值     s8  1:成功    非0:失败
-----作者       LJD
-----日期       2026-01-22
************************************************************************************************************************/
s8 c_usb_sw6303_set_force_ctrl_param(const I2cObj_T *p_i2c_obj, SW6303_CtrlObj_E obj, SW6303_ForceCtrl_T *param)
{
	u8 data[2] = {0};
	u8 uc_force_set_bit = 0;
	u8 uc_read_force_set_bit = 0;
	s8 c_ret = 0;
	vu16 us_reg_value = 0;

	/* 参数有效性检查 */
	if(obj >= SW6303_CTRL_OBJ_MAX)
		return -1;

	if(p_i2c_obj == (&tUSB_IC2_I2C))
	{
		// 设置使能bit为1
		if(obj == SW6303_CTRL_OBJ_ALL)
			uc_force_set_bit = 0x80;
		else
			uc_force_set_bit = 0x01 << obj;
	}
	else if(p_i2c_obj == (&tUSB_IC3_I2C))
	{
		// 设置使能bit为1
		if(obj == SW6303_CTRL_OBJ_ALL)
			uc_force_set_bit = 0x86;
		else
			uc_force_set_bit = 0x01 << obj;
	}
	else
		return 0;
	

	/* 读取当前REG0x132寄存器值 */
	if(c_usb_sw6303_read_bytes(p_i2c_obj, SW6303_REG_CTRG_FORCECTL, &uc_read_force_set_bit, 1) <= 0)
		return -1;

	//要一直开启,不可以关闭
	data[0] = uc_read_force_set_bit | uc_force_set_bit;
	c_ret = c_usb_sw6303_write_bytes(p_i2c_obj, SW6303_REG_CTRG_FORCECTL, data, 1);
	if(c_ret <= 0)
		return -2;

	//设置参数
	switch(obj)
	{
		// 输出电压 mV
		// 输出电压=dischg_vbus[11:0]*10mV
		case SW6303_CTRL_OBJ_ALL:
		case SW6303_CTRL_OBJ_VBUS:
		{
			// us_reg_value = param->usVbus / 10;
			// memcpy(data, &us_reg_value, sizeof(data));
			// if(c_usb_sw6303_write_bytes(p_i2c_obj, 0x41, &data[0], 1) <= 0)
			// 	return -3;

			// if(c_usb_sw6303_write_bytes(p_i2c_obj, 0x42, &data[1], 1) <= 0)
			// 	return -4;

			// if(obj != SW6303_CTRL_OBJ_ALL)
			// 	break;
		}
		
		// 充电电压 输入mV
		// 充电目标电压=charger_vol[11:0]*10mV
		case SW6303_CTRL_OBJ_CHG_VOLT:
		{
			// if(p_i2c_obj == (&tUSB_IC3_I2C))
			{
				us_reg_value = param->usChgVolt / 10;
				memcpy(data, &us_reg_value, sizeof(data));
				if(c_usb_sw6303_write_bytes(p_i2c_obj, 0x46, &data[0], 1) <= 0)
					return -5;

				if(c_usb_sw6303_write_bytes(p_i2c_obj, 0x47, &data[1], 1) <= 0)
					return -6;

				if(obj != SW6303_CTRL_OBJ_ALL)
					break;
			}
			
		}

		// 最大输入功率 1W
		// 输入最大功率=input_pow_set*1W 范围0~100W
		case SW6303_CTRL_OBJ_MAX_IN_PWR:
		{
			if(p_i2c_obj == (&tUSB_IC3_I2C))
			{
				data[0] = (u8)param->usMaxInPwr;
				if(c_usb_sw6303_write_bytes(p_i2c_obj, 0x45, &data[0], 1) <= 0)
					return -7;

				if(obj != SW6303_CTRL_OBJ_ALL)
					break;
			}
			
		}

		// 充电 hold 门限 0.1V
		// 可设置值 4.1V-25.6V，充电欠压=charger_hold*0.1V
		case SW6303_CTRL_OBJ_CHG_HOLD_VOLT:
		{
			// data[0] = (u8)param->usChgHoldVolt;
			// if(c_usb_sw6303_write_bytes(p_i2c_obj, 0x48, &data[0], 1) <= 0)
			// 	return -8;

			// if(obj != SW6303_CTRL_OBJ_ALL)
			// 	break;
		}

		//这个设置带载就断开
		// 电池限流 0.1A
		// 可设置值 0.1~12A，输出 IBAT 限流=dischg_ibat_limit*100mA
		case SW6303_CTRL_OBJ_IBAT:
		{
			// data[0] = (u8)param->usIbat;
			// if(c_usb_sw6303_write_bytes(p_i2c_obj, 0x4A, &data[0], 1) <= 0)
			// 	return -9;

			// if(obj != SW6303_CTRL_OBJ_ALL)
			// 	break;
		}
		
		// 输出电流 0.01A
		// 可设置值 0.2~7A，输出 IBUS 限流=dischg_ibus_limit*50mA
		case SW6303_CTRL_OBJ_IBUS:
		{
			// 放电
			// data[0] = (u8)(param->usIbus / 5);
			// if(c_usb_sw6303_write_bytes(p_i2c_obj, 0x43, &data[0], 1) <= 0)
			// 	return -10;

			// 充电
			// data[0] = (u8)(param->usIbus / 5);
			// if(c_usb_sw6303_write_bytes(p_i2c_obj, 0x49, &data[0], 1) <= 0)
			// 	return -10;

			// if(obj != SW6303_CTRL_OBJ_ALL)
			// 	break;
		}

		// 最大输出功率 1W
		// 可设置值 1~100W，输出功率= poset*1W
		case SW6303_CTRL_OBJ_MAX_OUT_PWR:
		{
			data[0] = (u8)param->usMaxOutPwr;
			if(c_usb_sw6303_write_bytes(p_i2c_obj, 0x4F, &data[0], 1) <= 0)
				return -11;

			break;
		}

		default:
			if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
				log_e("SW6303:不支持的对象参数%d", obj);
			return 0;
	}

	return 1;
}

/***********************************************************************************************************************
-----函数功能	设置充电配置参数
-----说明(备注)	配置REG0x107~REG0x10F寄存器，用于对IC3进行充电配置
-----传入参数   p_i2c_obj: I2C对象
				obj: 设置的对象（CFG0~CFG8或ALL）
				param: 充电配置参数结构体
-----返回值     s8  1:成功    非0:失败
-----作者       LJD
-----日期       2026-02-04
************************************************************************************************************************/
s8 c_usb_sw6303_set_chg_cfg(const I2cObj_T *p_i2c_obj, SW6303_ChgCfgObj_E obj, SW6303_ChgCfg_T *param)
{
	u8 data[1] = {0};
	s8 c_ret = 1;
	s8 c_ret_1 = 1;

	/* 参数有效性检查 */
	if(p_i2c_obj == NULL || param == NULL)
		return -1;
	
	if(obj >= SW6303_CHG_CFG_OBJ_MAX)
		return -2;

	if(c_usb_sw6303_high_reg_addr_enable(p_i2c_obj, true) <= 0)
		return -3;

	switch(obj)
	{
		case SW6303_CHG_CFG_OBJ_ALL:

		// 配置REG0x107: 充电配置0
		case SW6303_CHG_CFG_OBJ_CFG0:
		{
			// bit[7:5]: reserved，先读取再写入以保留保留位
			c_ret_1 = cI2C_ReadBytes(p_i2c_obj, SW6303_REG_STRG_CHG_CFG0, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -10;
				break;
			}
			
			// bit4: 快充是否按照固定功率充电 (0-按最大电流, 1-按固定功率)
			data[0] = (data[0] & 0xEF) | (param->bFcFixPower ? 0x10 : 0x00);
			
			// bit3: 输入最高功率设置方式 (0-自动, 1-手动设置)
			data[0] = (data[0] & 0xF7) | (param->bAutoDetectInPwr ? 0x00 : 0x08);
			
			// bit[2:0]: 输入最高功率设置
			data[0] = (data[0] & 0xF8) | ((u8)(param->eInPwr) & 0x07);

			c_ret_1 = cI2C_WriteBytes(p_i2c_obj, SW6303_REG_STRG_CHG_CFG0, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -11;
				break;
			}

			if(obj != SW6303_CHG_CFG_OBJ_ALL)
				break;
		}

		// 配置REG0x108: 充电配置1
		case SW6303_CHG_CFG_OBJ_CFG1:
		{
			data[0] = 0;
			
			// bit7: 电池类型设置方式 (0-自动, 1-手动设置)
			data[0] |= param->bAutoDetectBatType ? 0x00 : 0x80;
			
			// bit[6:4]: 电池类型设置
			data[0] |= ((u8)(param->eBatType) & 0x07) << 4;
			
			// bit3: 电池节数设置方式 (0-自动, 1-手动设置)
			data[0] |= param->bAutoDetectBatCell ? 0x00 : 0x08;
			
			// bit[2:0]: 电池节数设置
			data[0] |= (u8)(param->eBatCell) & 0x07;

			c_ret_1 = cI2C_WriteBytes(p_i2c_obj, SW6303_REG_STRG_CHG_CFG1, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -20;
				break;
			}

			if(obj != SW6303_CHG_CFG_OBJ_ALL)
				break;
		}

		// 配置REG0x109: 充电配置2
		case SW6303_CHG_CFG_OBJ_CFG2:
		{
			data[0] = 0;
			
			// bit7: 电池电压低于1.5V时是否充电 (0-允许, 1-不允许)
			data[0] |= param->bChgAbove1_5V ? 0x80 : 0x00;
			
			// bit[6:4]: 5V限压门限设置
			data[0] |= ((u8)(param->e5VOvp) & 0x07) << 4;
			
			// bit3: 涓流充电超时设置 (0-40分钟, 1-关闭超时)
			data[0] |= param->bTrickleTimeout ? 0x08 : 0x00;
			
			// bit[2:0]: 9V限压门限设置
			data[0] |= (u8)(param->e9VOvp) & 0x07;
			
			c_ret_1 = cI2C_WriteBytes(p_i2c_obj, SW6303_REG_STRG_CHG_CFG2, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -30;
				break;
			}

			if(obj != SW6303_CHG_CFG_OBJ_ALL)
				break;
		}

		// 配置REG0x10A: 充电配置3
		case SW6303_CHG_CFG_OBJ_CFG3:
		{
			data[0] = 0;
			
			// bit7: 适配器拉挂使能 (0-使能, 1-禁止)
			data[0] |= param->bAdapterPullDisable ? 0x80 : 0x00;
			
			// bit[6:4]: 10V限压门限设置
			data[0] |= ((u8)(param->e10VOvp) & 0x07) << 4;
			
			// bit3: 边充边放设置 (0-允许5V边充边放, 1-充电优先，禁止5V边充边放)
			data[0] |= ((u8)(param->eBypassMode) & 0x01) << 3;
			
			// bit[2:0]: 12V限压门限设置
			data[0] |= (u8)(param->e12VOvp) & 0x07;
			
			c_ret_1 = cI2C_WriteBytes(p_i2c_obj, SW6303_REG_STRG_CHG_CFG3, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -40;
				break;
			}

			if(obj != SW6303_CHG_CFG_OBJ_ALL)
				break;
		}

		// 配置REG0x10B: 充电配置4
		case SW6303_CHG_CFG_OBJ_CFG4:
		{
			// bit3: reserved，先读取再写入以保留保留位
			c_ret_1 = cI2C_ReadBytes(p_i2c_obj, SW6303_REG_STRG_CHG_CFG4, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -50;
				break;
			}
			
			// bit7: 边充边放hold门限设置 (0-提高欠压到4.8V, 1-不变)
			data[0] = (data[0] & 0x7F) | (param->bBypassHoldChange ? 0x80 : 0x00);
			
			// bit[6:4]: 15V限压门限设置
			data[0] = (data[0] & 0x8F) | (((u8)(param->e15VOvp) & 0x07) << 4);
			
			// bit3: reserved (保留原值)
			
			// bit[2:0]: 20V限压门限设置
			data[0] = (data[0] & 0xF8) | ((u8)(param->e20VOvp) & 0x07);
			
			c_ret_1 = cI2C_WriteBytes(p_i2c_obj, SW6303_REG_STRG_CHG_CFG4, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -51;
				break;
			}

			if(obj != SW6303_CHG_CFG_OBJ_ALL)
				break;
		}

		// 配置REG0x10C: 充电配置5
		case SW6303_CHG_CFG_OBJ_CFG5:
		{
			// bit[7:4]: reserved，先读取再写入以保留保留位
			c_ret_1 = cI2C_ReadBytes(p_i2c_obj, SW6303_REG_STRG_CHG_CFG5, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -60;
				break;
			}
			
			
			// bit3: 限压门限自适应 (0-电流>3A降低限压, 1-不降低)
			data[0] = (data[0] & 0xF7) | (param->bLimitVoltageAdapt ? 0x08 : 0x00);
			
			// bit[2:0]: reserved (保留原值)
			
			c_ret_1 = cI2C_WriteBytes(p_i2c_obj, SW6303_REG_STRG_CHG_CFG5, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -61;
				break;
			}

			if(obj != SW6303_CHG_CFG_OBJ_ALL)
				break;
		}

		// 配置REG0x10D: 充电配置6
		case SW6303_CHG_CFG_OBJ_CFG6:
		{
			data[0] = 0;
			
			// bit[7:6]: 磷酸铁锂电池涓流充电门限
			data[0] |= ((u8)(param->eFeTrickle) & 0x03) << 6;
			
			// bit[5:4]: 涓流充电电流
			data[0] |= ((u8)(param->eTrickleI) & 0x03) << 4;
			
			// bit[3:2]: 涓流充电电压迟滞
			data[0] |= ((u8)(param->eTrickleHyst) & 0x03) << 2;
			
			// bit[1:0]: 恒流充电超时设置
			data[0] |= (u8)(param->eCcTimeout) & 0x03;
			
			c_ret_1 = cI2C_WriteBytes(p_i2c_obj, SW6303_REG_STRG_CHG_CFG6, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -70;
				break;
			}

			if(obj != SW6303_CHG_CFG_OBJ_ALL)
				break;
		}

		// 配置REG0x10E: 充电配置7
		case SW6303_CHG_CFG_OBJ_CFG7:
		{
			// bit[5:4]: reserved，先读取再写入以保留保留位
			c_ret_1 = cI2C_ReadBytes(p_i2c_obj, SW6303_REG_STRG_CHG_CFG7, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -80;
				break;
			}
			
			// bit[7:6]: 充电截至电流设置
			data[0] = (data[0] & 0x3F) | (((u8)(param->eChgEndI) & 0x03) << 6);
			
			// bit[5:4]: reserved (保留原值)
			
			// bit[3:0]: C口5V充电电流设置
			data[0] = (data[0] & 0xF0) | ((u8)(param->eC5VI) & 0x0F);
			
			c_ret_1 = cI2C_WriteBytes(p_i2c_obj, SW6303_REG_STRG_CHG_CFG7, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -81;
				break;
			}

			if(obj != SW6303_CHG_CFG_OBJ_ALL)
				break;
		}

		// 配置REG0x10F: 充电配置8
		case SW6303_CHG_CFG_OBJ_CFG8:
		{
			data[0] = 0;
			
			// bit[7:5]: B口和L口5V充电电流设置
			data[0] |= ((u8)(param->eBL5VI) & 0x07) << 5;
			
			// bit[4:2]: 充电时NTC低温保护门限
			data[0] |= ((u8)(param->eChgNtcLow) & 0x07) << 2;
			
			// bit[1:0]: 充电时NTC高温保护门限
			data[0] |= (u8)(param->eChgNtcHigh) & 0x03;
			
			c_ret_1 = cI2C_WriteBytes(p_i2c_obj, SW6303_REG_STRG_CHG_CFG8, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -90;
				break;
			}

			break;
		}

		default:
			if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
				log_e("SW6303:不支持的充电配置对象%d", obj);
			return -99;
	}

	//关闭高位
	c_ret_1 = c_usb_sw6303_high_reg_addr_enable(p_i2c_obj, false);
	if(c_ret_1 <= 0)
		return -5;

	return c_ret;
}

/***********************************************************************************************************************
-----函数功能	设置放电配置参数
-----说明(备注)	参考c_usb_sw6303_set_force_ctrl_param函数实现
				配置REG0x100~REG0x106寄存器，方便对两个IC进行差异化配置
-----传入参数   p_i2c_obj: I2C对象
				obj: 设置的对象（CFG0~CFG5或ALL）
				param: 放电配置参数结构体
-----返回值     s8  1:成功    非0:失败
-----作者       LJD
-----日期       2026-01-22
************************************************************************************************************************/
s8 c_usb_sw6303_set_dischg_cfg(const I2cObj_T *p_i2c_obj, SW6303_DchgCfgObj_E obj, SW6303_DchgCfg_T *param)
{
	u8 data[1] = {0};
	s8 c_ret = 1;
	s8 c_ret_1 = 0;

	/* 参数有效性检查 */
	if(p_i2c_obj == NULL || param == NULL)
		return -1;
	
	if(obj >= SW6303_DCHG_CFG_OBJ_MAX)
		return -2;

	//高地址模式
	c_ret_1 = c_usb_sw6303_high_reg_addr_enable(p_i2c_obj, true);
	if(c_ret_1 <= 0)
		return -3;

	switch(obj)
	{
		case SW6303_DCHG_CFG_OBJ_ALL:
		
		// 配置REG0x100: 放电配置0
		case SW6303_DCHG_CFG_OBJ_CFG0:
		{
			// 该寄存器没有reserved位，直接写入
			data[0] = 0;
			
			// bit7: 芯片放电恒温环使能 (0-使能, 1-禁止)
			data[0] |= param->bTempLoopEnable ? 0x80 : 0x00;
			
			// bit[6:4]: 芯片放电恒温环阈值
			data[0] |= ((u8)(param->eTempThr) & 0x07) << 4;
			
			// bit3: 输出最大功率自动检测 (0-自动, 1-由寄存器设置)
			data[0] |= param->bPwrAutoDetect ? 0x08 : 0x00;
			
			// bit[2:0]: 输出功率设置
			data[0] |= (u8)(param->eOutPwr) & 0x07;
			
			c_ret_1 = cI2C_WriteBytes(p_i2c_obj, SW6303_REG_STRG_DCHG_CFG0, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -11;
				break;
			}

			if(obj != SW6303_DCHG_CFG_OBJ_ALL)
				break;
		}

		// 配置REG0x101: 放电配置1
		case SW6303_DCHG_CFG_OBJ_CFG1:
		{
			// 该寄存器没有reserved位，直接写入
			data[0] = 0;
			
			// bit[7:6]: 多口输出时最大输出电流设置
			data[0] |= ((u8)(param->eMultiPortMaxI) & 0x03) << 6;
			
			// bit[5:4]: VBUS限流固定偏移
			data[0] |= ((u8)(param->eVbusIOffset) & 0x03) << 4;
			
			// bit[3:2]: 输出电压固定偏移
			data[0] |= ((u8)(param->eVbusVOffset) & 0x03) << 2;
			
			// bit[1:0]: 线补设置
			data[0] |= (u8)(param->eLineComp) & 0x03;
			
			c_ret_1 = cI2C_WriteBytes(p_i2c_obj, SW6303_REG_STRG_DCHG_CFG1, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -20;
				break;
			}

			if(obj != SW6303_DCHG_CFG_OBJ_ALL)
				break;
		}

		// 配置REG0x102: 放电配置2
		case SW6303_DCHG_CFG_OBJ_CFG2:
		{
			// bit[6:3]和bit[1:0]: reserved，先读取再写入以保留保留位
			c_ret_1 = cI2C_ReadBytes(p_i2c_obj, SW6303_REG_STRG_DCHG_CFG2, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -30;
				break;
			}
			
			// bit7: 快充模式线补使能 (0-使能, 1-禁止)
			data[0] = (data[0] & 0x7F) | (param->bFcLineCompEnable ? 0x80 : 0x00);
			
			// bit[6:3]: reserved (保留原值)
			
			// bit2: VBUS快速过压保护控制 (0-打开, 1-关闭)
			data[0] = (data[0] & 0xFB) | (param->bVbusFastOvpEnable ? 0x04 : 0x00);
			
			// bit[1:0]: reserved (保留原值)
			
			c_ret_1 = cI2C_WriteBytes(p_i2c_obj, SW6303_REG_STRG_DCHG_CFG2, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -31;
				break;
			}

			if(obj != SW6303_DCHG_CFG_OBJ_ALL)
				break;
		}

		// 配置REG0x103: 放电配置3
		case SW6303_DCHG_CFG_OBJ_CFG3:
		{
			// bit[5:4]: reserved，先读取再写入以保留保留位
			c_ret_1 = cI2C_ReadBytes(p_i2c_obj, SW6303_REG_STRG_DCHG_CFG3, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -40;
				break;
			}
			
			// bit[7:6]: 放电IBAT限流的默认值
			data[0] = (data[0] & 0x3F) | (((u8)(param->eDchgIbatDef) & 0x03) << 6);
			
			// bit[5:4]: reserved (保留原值)
			
			// bit[3:0]: 磷酸铁锂电池欠压门限 (0:2.75V*N, 1~12:1.9~3.0*N,0.1V/step)
			data[0] = (data[0] & 0xF0) | (param->ucLipoUvloThr & 0x0F);
			
			c_ret_1 = cI2C_WriteBytes(p_i2c_obj, SW6303_REG_STRG_DCHG_CFG3, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -41;
				break;
			}

			if(obj != SW6303_DCHG_CFG_OBJ_ALL)
				break;
		}

		// 配置REG0x104: 放电配置4
		case SW6303_DCHG_CFG_OBJ_CFG4:
		{
			// bit7和bit3: reserved，先读取再写入以保留保留位
			c_ret_1 = cI2C_ReadBytes(p_i2c_obj, SW6303_REG_STRG_DCHG_CFG4, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -50;
				break;
			}
			
			// bit7: reserved (保留原值)
			
			// bit[6:4]: UVLO迟滞，N为电池节数
			data[0] = (data[0] & 0x8F) | (((u8)(param->eUvloHyst) & 0x07) << 4);
			
			// bit3: reserved (保留原值)
			
			// bit[2:0]: 三元锂电池欠压门限，N为电池节数
			data[0] = (data[0] & 0xF8) | ((u8)(param->eLipoUvlo) & 0x07);
			
			c_ret_1 = cI2C_WriteBytes(p_i2c_obj, SW6303_REG_STRG_DCHG_CFG4, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -51;
				break;
			}

			if(obj != SW6303_DCHG_CFG_OBJ_ALL)
				break;
		}

		// 配置REG0x106: 放电配置5
		case SW6303_DCHG_CFG_OBJ_CFG5:
		{
			// bit6和bit3: reserved，先读取再写入以保留保留位
			c_ret_1 = cI2C_ReadBytes(p_i2c_obj, SW6303_REG_STRG_DCHG_CFG5, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -60;
				break;
			}
			
			// bit7: 电感设置 (0-4.7uH, 1-10uH)
			data[0] = (data[0] & 0x7F) | (((u8)(param->eInductor) & 0x01) << 7);
			
			// bit6: reserved (保留原值)
			
			// bit[5:4]: 放电NTC高温门限
			data[0] = (data[0] & 0xCF) | (((u8)(param->eNtcHighThr) & 0x03) << 4);
			
			// bit3: reserved (保留原值)
			
			// bit[2:0]: 放电NTC低温门限
			data[0] = (data[0] & 0xF8) | ((u8)(param->eNtcLowThr) & 0x07);
			
			c_ret_1 = cI2C_WriteBytes(p_i2c_obj, SW6303_REG_STRG_DCHG_CFG5, data, 1);
			if(c_ret_1 <= 0)
			{
				c_ret = -61;
				break;
			}

			break;
		}

		default:
			if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
				log_e("SW6303:不支持的放电配置对象%d", obj);
			return -9;
	}
	
	//关闭高位
	c_ret_1 = c_usb_sw6303_high_reg_addr_enable(p_i2c_obj, false);
	if(c_ret_1 <= 0)
		return -4;

	return c_ret;
}

/***********************************************************************************************************************
-----函数功能	SW6303 设置工作模式 reg28
-----传入参数   p_i2c_obj: I2C对象
				mode: SW6303模式
				en	true:开启    false:关闭
-----返回值     s8	1:成功    -1:失败
-----作者       LJD
-----日期       2026-01-22
************************************************************************************************************************/
s8 c_usb_sw6303_set_work_mode(const I2cObj_T *p_i2c_obj, SW6303_Mode_E mode, bool en)
{
	u8 data[1] = {0};
	bool en_temp = false;  //true:开启

	// 充电强制关闭 bit2
	// 0：无影响
	// 1：强制关闭充电
	// 此 bit 不会自动清零，在强制关闭充电时，只关闭 charger，通路管开关不受影响
	const u8 reg_chg_value = 0x04;

	// 强制关闭输出 bit3
	// 0：无影响
	// 1：强制关闭输出使能
	// 此 bit 不会自动清零，在强制关闭输出期间，A 口插入检测关闭，不响应按键打开输出，Type-C 口只作为 sink
	const u8 reg_dischg_value = 0x08;

	if(c_usb_sw6303_read_bytes(p_i2c_obj, SW6303_REG_CTRG_MODE, data, sizeof(data)) <= 0)
		return -1;

	//检查状态是否一致
	switch(mode)
	{
		case SW6303_MODE_ALL:
		case SW6303_MODE_DISCHG:
		{
				en_temp = ((data[0] & reg_dischg_value) != reg_dischg_value)? true : false;
				if(en != en_temp)
					break;

				if(mode != SW6303_MODE_ALL)
					return 2;
		}

		case SW6303_MODE_CHG:
		{
				en_temp = ((data[0] & reg_chg_value) != reg_chg_value)? true : false;
				if(en != en_temp)
					break;

				//最后一个,直接返回	
				return 2;
		}
		
		default:
				return -2;
	}

	//更新状态
	// 0：无影响
	// 1：强制关闭充电
	switch(mode)
	{
		case SW6303_MODE_ALL:
		case SW6303_MODE_DISCHG:
		{
				//关闭放电
				if(en == false)
					data[0] = data[0] | reg_dischg_value;
				else
					data[0] = data[0] & (~reg_dischg_value);

				if(mode != SW6303_MODE_ALL)
					break;
		}

		case SW6303_MODE_CHG:
		{
				//关闭充电
				if(en == false)
					data[0] = data[0] | reg_chg_value;
				else
					data[0] = data[0] & (~reg_chg_value);
		}
		break;
	
	   default:
			return -3;
	}

	if(c_usb_sw6303_write_bytes(p_i2c_obj, SW6303_REG_CTRG_MODE, data, sizeof(data)) <= 0)
		return -4;

	return 1;
}

/***********************************************************************************************************************
-----函数功能    获取ADC数据
-----说明(备注)  通过REG0x30配置ADC选择，然后读取REG0x31和REG0x32获取ADC数据
-----传入参数    p_i2c_obj: I2C对象
				adc_sel: ADC数据选择类型
-----返回值     s16: ADC原始数据(12位)，失败返回-1
-----作者       LJD
-----日期       2026-01-22
************************************************************************************************************************/
s16 s_usb_sw6303_get_adc_data(const I2cObj_T *p_i2c_obj, ADC_Sel_E adc_sel)
{
	u8 data[1] = {0};
	u8 adc_low = 0;
	u8 adc_high = 0;
	u16 adc_value = 0;
	u8 reg_value;

	// 根据ADC类型设置REG0x30的值
	switch(adc_sel)
	{
		case ADC_SEL_VBUS:
			reg_value = 0;   // 输入输出电压（8mV）
			break;
		case ADC_SEL_IBUS:
			reg_value = 1;   // 输入/输出IBUS电流（4mA）
			break;
		case ADC_SEL_VBAT:
			reg_value = 2;   // 电池电压（7mV）
			break;
		case ADC_SEL_IBAT:
			reg_value = 3;   // 输入/输出IBAT电流（5mA）
			break;
		case ADC_SEL_TEMP:
			reg_value = 9;   // 芯片温度(1/6.82°C)
			break;
		case ADC_SEL_NTC:
			reg_value = 10;  // NTC电压（1.1mV）
			break;
		default:
			return -1;
	}

	// 1. 写REG0x30配置ADC数据选择
	data[0] = reg_value;
	if(cI2C_WriteBytes(p_i2c_obj, SW6303_REG_ADC_CONFIG, data, sizeof(data)) <= 0)
		return -2;

	// 2. 读REG0x31获取ADC数据低8位（必须先读低8位）
	if(cI2C_ReadBytes(p_i2c_obj, SW6303_REG_ADC_DATA_LOW, data, sizeof(data)) <= 0)
		return -3;
	adc_low = data[0];

	// 3. 读REG0x32获取ADC数据高4位
	if(cI2C_ReadBytes(p_i2c_obj, SW6303_REG_ADC_DATA_HIGH, data, sizeof(data)) <= 0)
		return -4;
	adc_high = data[0] & 0x0F;  // 只取低4位

	// 4. 组合12位ADC数据
	adc_value = (adc_high << 8) | adc_low;

	return (s16)adc_value;
}

/***********************************************************************************************************************
-----函数功能	SW6303 复位IC regD9
-----说明(备注) bit5-7
				I2C 写操作使能，如果要写其他寄存器（除 REG0x20~0x24 之
				外），需要先执行如下操作：
				1) 写 REG0x24=0x20
				2) 写 REG0x24=0x40 
				3) 写 REG0x24=0x80
-----传入参数   p_i2c_obj: I2C对象
-----返回值     s8  1:成功    -1:失败
-----作者       LJD
-----日期       2026-01-22
************************************************************************************************************************/
s8 c_usb_sw6303_reset(const I2cObj_T *p_i2c_obj)
{
	u8 data[1] = {0};
	u16 reg_addr = 0xD9;

	if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
		log_w("bUsbTask:开始复位");

	//开启写使能
	if(c_usb_sw6303_write_enable(p_i2c_obj, true) <= 0)
		return -1;

	//关闭高位地址
	// if(c_usb_sw6303_high_reg_addr_enable(p_i2c_obj, false) <= 0)
	// 	return -2;

	/* 写 REG0x24 = 0x20 */
	data[0] = 0x20;
	if(cI2C_WriteBytes(p_i2c_obj, reg_addr, data, sizeof(data)) <= 0)
		return -3;

	cI2C_ReadBytes(p_i2c_obj, reg_addr, data, sizeof(data));
	if(data[0] != 0x20)
	{
		if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
			log_w("bUsbTask:写入复位失败,读取数据%d", data[0]);
		return -30;
	}

	/* 写 REG0x24 = 0x40 */
	data[0] = 0x40;
	if(cI2C_WriteBytes(p_i2c_obj, reg_addr, data, sizeof(data)) <= 0)
		return -4;
	
	/* 写 REG0x24 = 0x80 */
	data[0] = 0x80;
	if(cI2C_WriteBytes(p_i2c_obj, reg_addr, data, sizeof(data)) <= 0)
		return -5;

	cI2C_ReadBytes(p_i2c_obj, reg_addr, data, sizeof(data));
	if(data[0] != 0x80)
	{
		if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
			log_w("bUsbTask:写入复位失败,读取数据%d", data[0]);
		return -6;
	}
		
	if(c_usb_sw6303_write_enable(p_i2c_obj, false) <= 0)
		return -7;
	
	if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
		log_w("bUsbTask:复位成功");

	/* 如果已经使能，直接返回 */
	return 1;
}


/***********************************************************************************************************************
-----函数功能	SW6303 获取端口状态 reg1D
-----说明(备注)	REG0x1D: 端口状态指示
				bit7: C1作为source的连接状态 (0-未连接, 1-连接)
				bit6: C1作为sink的连接状态 (0-未连接, 1-连接)
				bit5: C2作为source的连接状态 (0-未连接, 1-连接)
				bit4: C2作为sink的连接状态 (0-未连接, 1-连接)
				bit3: reserved
				bit2: A口在线状态 (0-不在线, 1-在线)
				bit1: C1口的在线状态 (0-不在线, 1-在线)
				bit0: C2口的在线状态 (0-不在线, 1-在线)
-----传入参数   p_i2c_obj: I2C对象
				p_status: 端口状态结构体指针
-----返回值     s8  1:成功    -1:失败
-----作者       LJD
-----日期       2026-02-04
************************************************************************************************************************/
s8 c_usb_sw6303_get_port_status(const I2cObj_T *p_i2c_obj, SW6303_PortStatus_T *p_status)
{
	u8 data[1] = {0};
	u8 reg_value;

	if(p_i2c_obj == NULL || p_status == NULL)
		return -1;

	/* 读取REG0x1D寄存器值 */
	if(c_usb_sw6303_read_bytes(p_i2c_obj, SW6303_REG_PORT_STATUS, data, sizeof(data)) <= 0)
	{
		if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
			log_e("SW6303:读取端口状态寄存器失败,错误代码%d", c_usb_sw6303_read_bytes(p_i2c_obj, SW6303_REG_PORT_STATUS, data, sizeof(data)));
		return -2;
	}

	reg_value = data[0];

	/* 解析寄存器值 */
	// C1口状态
	p_status->bC1SrcConn = (reg_value & 0x80) ? true : false;    // bit7
	p_status->bC1SinkConn = (reg_value & 0x40) ? true : false;   // bit6
	p_status->bC1Online = (reg_value & 0x02) ? true : false;          // bit1

	// C2口状态
	p_status->bC2SrcConn = (reg_value & 0x20) ? true : false;    // bit5
	p_status->bC2SinkConn = (reg_value & 0x10) ? true : false;   // bit4
	p_status->bC2Online = (reg_value & 0x01) ? true : false;         // bit0

	// A口状态
	p_status->bAOnline = (reg_value & 0x04) ? true : false;           // bit2

	return 1;
}

/***********************************************************************************************************************
-----函数功能    读取充电目标电压设置值
-----说明(备注)  通过REG0x46和REG0x47读取充电目标电压设置
                充电目标电压 = charger_vol[11:0] * 10mV
                范围: 3.3V ~ 27.3V (实际不允许超过26V)
-----传入参数    p_i2c_obj: I2C对象
-----输出参数    none
-----返回值      s16: 充电目标电压值(单位: mV), 失败返回-1
-----作者       LJD
-----日期       2026-02-25
************************************************************************************************************************/
s16 s_usb_sw6303_get_chg_target_volt(const I2cObj_T *p_i2c_obj)
{
    u8 data_low = 0;
    u8 data_high = 0;
    u16 reg_value = 0;
    u16 volt_mv = 0;

    if(p_i2c_obj == NULL)
        return -1;

    /* 读取REG0x46: 充电目标电压低8位 */
    if(c_usb_sw6303_read_bytes(p_i2c_obj, SW6303_REG_CTRG_CHG_VOLT_LOW, &data_low, 1) <= 0)
    {
        if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
            log_e("SW6303:读取充电目标电压低8位失败");
        return -2;
    }

    /* 读取REG0x47: 充电目标电压高4位 */
    if(c_usb_sw6303_read_bytes(p_i2c_obj, SW6303_REG_CTRG_CHG_VOLT_HIGH, &data_high, 1) <= 0)
    {
        if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
            log_e("SW6303:读取充电目标电压高4位失败");
        return -3;
    }

    /* 组合12位数据: charger_vol[11:0] */
    reg_value = ((u16)(data_high & 0x0F) << 8) | data_low;

    /* 计算电压值: 充电目标电压 = charger_vol[11:0] * 10mV */
    volt_mv = (s16)(reg_value * 10);

    if(uPrint.tFlag.bUsbTask)
        log_i("SW6303:充电目标电压=%dmV (寄存器值=0x%03X)", volt_mv, reg_value);

    return volt_mv;
}

/***********************************************************************************************************************
-----函数功能    读取系统状态指示 (REG0x18)
-----说明(备注)  REG0x18: 系统状态指示
                bit7: 充电异常触发事件指示 (0-无异常, 1-异常导致充电关闭)
                bit6: 放电异常触发事件指示 (0-无异常, 1-异常导致放电关闭)
                bit5: 充电状态指示位 (0-充电关闭, 1-充电打开)
                bit4: 放电状态指示位 (0-放电关闭, 1-放电打开)
                bit3: C1口通路状态指示位 (0-关闭, 1-打开)
                bit2: C2口通路状态指示位 (0-关闭, 1-打开)
                bit0: A口通路状态指示位 (0-关闭, 1-打开)
-----传入参数    p_i2c_obj: I2C对象
                p_status: 系统状态结构体指针
-----返回值      s8: 1-成功, -1:失败
-----作者       LJD
-----日期       2026-03-02
************************************************************************************************************************/
s8 c_usb_sw6303_get_sys_status(const I2cObj_T *p_i2c_obj, SW6303_SysStatus_T *p_status)
{
    u8 data[1] = {0};

    if(p_i2c_obj == NULL || p_status == NULL)
        return -1;

    /* 读取REG0x18寄存器值 */
    if(c_usb_sw6303_read_bytes(p_i2c_obj, SW6303_REG_SYS_STATUS, data, sizeof(data)) <= 0)
    {
        if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
            log_e("SW6303:读取系统状态寄存器失败");
        return -2;
    }

    /* 解析寄存器值 */
    p_status->bChgAbnormal = (data[0] & 0x80) ? true : false;    // bit7
    p_status->bDchgAbnormal = (data[0] & 0x40) ? true : false;   // bit6
    p_status->bChgStatus = (data[0] & 0x20) ? true : false;      // bit5
    p_status->bDchgStatus = (data[0] & 0x10) ? true : false;     // bit4
    p_status->bC1PathStatus = (data[0] & 0x08) ? true : false;   // bit3
    p_status->bC2PathStatus = (data[0] & 0x04) ? true : false;   // bit2
    p_status->bAPathStatus = (data[0] & 0x01) ? true : false;    // bit0

    return 1;
}

/***********************************************************************************************************************
-----函数功能    读取异常事件指示 (REG0x15)
-----说明(备注)  REG0x15: 异常事件指示
                bit5: 电量计量完成指示 - 写1清零
                bit4: 电池欠压UVLO事件指示 - 写1清零
                bit3: 充电异常事件指示 - 写1清零
                bit2: 放电异常事件指示 - 写1清零
                bit1: 按键事件指示 - 写1清零
                bit0: 场景变化事件指示 - 写1清零
-----传入参数    p_i2c_obj: I2C对象
                p_event: 异常事件状态结构体指针
-----返回值      s8: 1-成功, -1:失败
-----作者       LJD
-----日期       2026-03-02
************************************************************************************************************************/
s8 c_usb_sw6303_get_abnormal_event(const I2cObj_T *p_i2c_obj, SW6303_AbnormalEvent_T *p_event)
{
    u8 data[1] = {0};

    if(p_i2c_obj == NULL || p_event == NULL)
        return -1;

    /* 读取REG0x15寄存器值 */
    if(c_usb_sw6303_read_bytes(p_i2c_obj, SW6303_REG_ABNORMAL_EVENT, data, sizeof(data)) <= 0)
    {
        if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
            log_e("SW6303:读取异常事件寄存器失败");
        return -2;
    }

    /* 解析寄存器值 */
    p_event->bCapCalcDone = (data[0] & 0x20) ? true : false;         // bit5
    p_event->bUvloEvent = (data[0] & 0x10) ? true : false;           // bit4
    p_event->bChgAbnormalEvent = (data[0] & 0x08) ? true : false;    // bit3
    p_event->bDchgAbnormalEvent = (data[0] & 0x04) ? true : false;   // bit2
    p_event->bKeyEvent = (data[0] & 0x02) ? true : false;            // bit1
    p_event->bSceneChangeEvent = (data[0] & 0x01) ? true : false;    // bit0

    return 1;
}

/***********************************************************************************************************************
-----函数功能    清除异常事件标志 (REG0x15)
-----说明(备注)  向REG0x15对应bit写1清零异常事件标志
                bit7-6: reserved (保留不变)
                SW6303_EVENT_CAP_CALC_DONE:  bit5 - 电量计量完成指示
                SW6303_EVENT_UVLO:           bit4 - 电池欠压UVLO事件指示
                SW6303_EVENT_CHG_ABNORMAL:   bit3 - 充电异常事件指示
                SW6303_EVENT_DCHG_ABNORMAL:  bit2 - 放电异常事件指示
                SW6303_EVENT_KEY:            bit1 - 按键事件指示
                SW6303_EVENT_SCENE_CHANGE:   bit0 - 场景变化事件指示
-----传入参数    p_i2c_obj: I2C对象
                event_type: 异常事件类型
-----返回值      s8: 1-成功, -1:失败
-----作者       LJD
-----日期       2026-03-02
************************************************************************************************************************/
s8 c_usb_sw6303_clear_abnormal_event(const I2cObj_T *p_i2c_obj, SW6303_EventType_E event_type)
{
    u8 data[1] = {0};
    u8 bit_mask = 0;

    if(p_i2c_obj == NULL)
        return -1;

    if(event_type >= SW6303_EVENT_MAX)
        return -2;

    /* 根据事件类型计算bit掩码 */
    switch(event_type)
    {
        case SW6303_EVENT_CAP_CALC_DONE:
            bit_mask = 0x20;    // bit5
            break;
        case SW6303_EVENT_UVLO:
            bit_mask = 0x10;    // bit4
            break;
        case SW6303_EVENT_CHG_ABNORMAL:
            bit_mask = 0x08;    // bit3
            break;
        case SW6303_EVENT_DCHG_ABNORMAL:
            bit_mask = 0x04;    // bit2
            break;
        case SW6303_EVENT_KEY:
            bit_mask = 0x02;    // bit1
            break;
        case SW6303_EVENT_SCENE_CHANGE:
            bit_mask = 0x01;    // bit0
            break;
        default:
            return -3;
    }

    /* 先读取REG0x15寄存器当前值 */
    if(c_usb_sw6303_read_bytes(p_i2c_obj, SW6303_REG_ABNORMAL_EVENT, data, sizeof(data)) <= 0)
    {
        if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
            log_e("SW6303:读取异常事件寄存器失败");
        return -4;
    }

    /* 保留bit7-6不变，只修改bit0-5，写1清零对应事件标志 */
    data[0] = (data[0] & 0xC0) | bit_mask;
    if(c_usb_sw6303_write_bytes(p_i2c_obj, SW6303_REG_ABNORMAL_EVENT, data, sizeof(data)) <= 0)
    {
        if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
            log_e("SW6303:清除异常事件失败, event_type=%d", event_type);
        return -5;
    }

    if(uPrint.tFlag.bUsbTask)
        log_i("SW6303:清除异常事件成功, event_type=%d", event_type);

    return 1;
}

/***********************************************************************************************************************
-----函数功能    读取系统异常状态0 (REG0x2A) - 放电异常
-----说明(备注)  REG0x2A: 系统异常状态指示0
                bit6: 放电时电池过压异常
                bit5: 放电时芯片过温异常
                bit4: 放电时NTC过温异常
                bit3: 放电时VBUS过载异常
                bit2: 放电时VBUS短路异常
                bit1: 放电时VBUS慢速过压保护异常
                bit0: 放电时VBUS快速过压保护异常
                注意: 这些bit在下次放电事件时自动清零
-----传入参数    p_i2c_obj: I2C对象
                p_status: 异常状态0结构体指针
-----返回值      s8: 1-成功, -1:失败
-----作者       LJD
-----日期       2026-03-02
************************************************************************************************************************/
s8 c_usb_sw6303_get_abnormal_status0(const I2cObj_T *p_i2c_obj, SW6303_AbnormalStatus0_T *p_status)
{
    u8 data[1] = {0};

    if(p_i2c_obj == NULL || p_status == NULL)
        return -1;

    /* 读取REG0x2A寄存器值 */
    if(c_usb_sw6303_read_bytes(p_i2c_obj, SW6303_REG_SYS_ABNORMAL_STATUS0, data, sizeof(data)) <= 0)
    {
        if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
            log_e("SW6303:读取异常状态0寄存器失败");
        return -2;
    }

    /* 解析寄存器值 */
    p_status->bDchgBatOvp = (data[0] & 0x40) ? true : false;       // bit6
    p_status->bDchgChipOtp = (data[0] & 0x20) ? true : false;      // bit5
    p_status->bDchgNtcOtp = (data[0] & 0x10) ? true : false;       // bit4
    p_status->bDchgVbusOcp = (data[0] & 0x08) ? true : false;      // bit3
    p_status->bDchgVbusShort = (data[0] & 0x04) ? true : false;    // bit2
    p_status->bDchgVbusSlowOvp = (data[0] & 0x02) ? true : false;  // bit1
    p_status->bDchgVbusFastOvp = (data[0] & 0x01) ? true : false;  // bit0

    return 1;
}

/***********************************************************************************************************************
-----函数功能    读取系统异常状态1 (REG0x2B) - 充电异常
-----说明(备注)  REG0x2B: 系统异常状态指示1
                bit7: 电池电压低于1.5V异常
                bit6: 充电超时异常
                bit5: 充满事件指示
                bit4: 充电时电池过压异常
                bit3: 充电时芯片过温异常
                bit2: 充电时NTC过温异常
                bit1: 充电时VBUS慢速过压异常
                bit0: 充电时VBUS快速过压异常
                注意: 这些bit在下次充电事件时自动清零
-----传入参数    p_i2c_obj: I2C对象
                p_status: 异常状态1结构体指针
-----返回值      s8: 1-成功, -1:失败
-----作者       LJD
-----日期       2026-03-02
************************************************************************************************************************/
s8 c_usb_sw6303_get_abnormal_status1(const I2cObj_T *p_i2c_obj, SW6303_AbnormalStatus1_T *p_status)
{
    u8 data[1] = {0};

    if(p_i2c_obj == NULL || p_status == NULL)
        return -1;

    /* 读取REG0x2B寄存器值 */
    if(c_usb_sw6303_read_bytes(p_i2c_obj, SW6303_REG_SYS_ABNORMAL_STATUS1, data, sizeof(data)) <= 0)
    {
        if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
            log_e("SW6303:读取异常状态1寄存器失败");
        return -2;
    }

    /* 解析寄存器值 */
    p_status->bBatBelow1_5V = (data[0] & 0x80) ? true : false;      // bit7
    p_status->bChgTimeout = (data[0] & 0x40) ? true : false;        // bit6
    p_status->bChgFull = (data[0] & 0x20) ? true : false;           // bit5
    p_status->bChgBatOvp = (data[0] & 0x10) ? true : false;         // bit4
    p_status->bChgChipOtp = (data[0] & 0x08) ? true : false;        // bit3
    p_status->bChgNtcOtp = (data[0] & 0x04) ? true : false;         // bit2
    p_status->bChgVbusSlowOvp = (data[0] & 0x02) ? true : false;    // bit1
    p_status->bChgVbusFastOvp = (data[0] & 0x01) ? true : false;    // bit0

    return 1;
}

/***********************************************************************************************************************
-----函数功能    读取系统异常状态2 (REG0x2C)
-----说明(备注)  REG0x2C: 系统异常状态指示2
                bit[7:6]: 按键事件指示 (0-无, 1-短按, 2-双击, 3-长按)
                bit4: 放电时62368低温异常
                bit3: 充电时62368低温异常
                bit2: 充电62368过温异常
                bit1: 放电时DPDM 5.5V过压异常
                bit0: 放电时CC 5.5V过压异常
                注意: 这些bit在下次对应事件时自动清零
-----传入参数    p_i2c_obj: I2C对象
                p_status: 异常状态2结构体指针
-----返回值      s8: 1-成功, -1:失败
-----作者       LJD
-----日期       2026-03-02
************************************************************************************************************************/
s8 c_usb_sw6303_get_abnormal_status2(const I2cObj_T *p_i2c_obj, SW6303_AbnormalStatus2_T *p_status)
{
    u8 data[1] = {0};

    if(p_i2c_obj == NULL || p_status == NULL)
        return -1;

    /* 读取REG0x2C寄存器值 */
    if(c_usb_sw6303_read_bytes(p_i2c_obj, SW6303_REG_SYS_ABNORMAL_STATUS2, data, sizeof(data)) <= 0)
    {
        if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
            log_e("SW6303:读取异常状态2寄存器失败");
        return -2;
    }

    /* 解析寄存器值 */
    p_status->eKeyEvent = (SW6303_KeyEvent_E)((data[0] >> 6) & 0x03);  // bit[7:6]
    p_status->bDchg62368LowTemp = (data[0] & 0x10) ? true : false;     // bit4
    p_status->bChg62368LowTemp = (data[0] & 0x08) ? true : false;      // bit3
    p_status->bChg62368Otp = (data[0] & 0x04) ? true : false;          // bit2
    p_status->bDchgDpdmOvp = (data[0] & 0x02) ? true : false;          // bit1
    p_status->bDchgCcOvp = (data[0] & 0x01) ? true : false;            // bit0

    return 1;
}
