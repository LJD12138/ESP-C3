#include "UsbDevice/usb_iface.h"


I2cObj_T  		tUSB_IC1_I2C;
I2cObj_T  		tUSB_IC2_I2C;
I2cObj_T  		tUSB_IC3_I2C;

/*****************************************************************************************************************
-----函数功能    DC相关IO初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/

static void v_usb_gpio_init(void)
{
    gpio_config_t io_conf = {
		.intr_type = GPIO_INTR_DISABLE, //falling edge interrupt
		.mode = GPIO_MODE_OUTPUT, //set as input mode
		.pin_bit_mask = (1ULL << usbPOWER_EN_PIN),
		.pull_down_en = 0, //disable pull-down mode
		.pull_up_en = 0 //enable pull-up mode
		};
	//configure GPIO with the given settings
	gpio_config(&io_conf);
    usbPOWER_EN_ON();

	//PD100W   IIC初始化
    tUSB_IC1_I2C.ulGPIO_PIN_SCL  = usbIC1_SCL_PIN;
    tUSB_IC1_I2C.ulGPIO_PIN_SDA  = usbIC1_SDA_PIN;
    tUSB_IC1_I2C.Addr = 0x3C;
    tUSB_IC1_I2C.AddrType = AddrType_7bit;
    tUSB_IC1_I2C.usDelay = 20;
    vI2C_ObjInit(&tUSB_IC1_I2C);
	
	//USB充电 IIC初始化
    tUSB_IC2_I2C.ulGPIO_PIN_SCL  = usbIC2_SCL_PIN;
    tUSB_IC2_I2C.ulGPIO_PIN_SDA  = usbIC2_SDA_PIN;
    tUSB_IC2_I2C.Addr = 0x3C;
    tUSB_IC2_I2C.AddrType = AddrType_7bit;
    tUSB_IC2_I2C.usDelay = 80;
    vI2C_ObjInit(&tUSB_IC2_I2C);

    //USB充电 IIC初始化
    tUSB_IC3_I2C.ulGPIO_PIN_SCL  = usbIC3_SCL_PIN;
    tUSB_IC3_I2C.ulGPIO_PIN_SDA  = usbIC3_SDA_PIN;
    tUSB_IC3_I2C.Addr = 0x3C;
    tUSB_IC3_I2C.AddrType = AddrType_7bit;
    tUSB_IC3_I2C.usDelay = 80;
    vI2C_ObjInit(&tUSB_IC3_I2C);
}


/***********************************************************************************************************************
-----函数功能    LED初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vUsb_IfaceInit(void)
{
	v_usb_gpio_init();
}
