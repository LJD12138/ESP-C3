#include "gpio_init.h"

#include "Sys/sys_task.h"
#include "timer_task.h"
#include "Print/print_task.h"

/***********************************************************************************************************************
-----函数功能    IO口初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void vGPIO_Init(void)
{
	gpio_config_t io_conf = {
		.intr_type = GPIO_INTR_DISABLE, //falling edge interrupt
		.mode = GPIO_MODE_OUTPUT, //set as input mode
		.pin_bit_mask = (1ULL << gpioASSIST_OPEN_PIN),
		.pull_down_en = 0, //disable pull-down mode
		.pull_up_en = 0 //enable pull-up mode
		};
	//configure GPIO with the given settings
	gpio_config(&io_conf);
	gpioASSIST_OPEN_OFF();
}

/***********************************************************************************************************************
-----函数功能    协助BMS开启
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:操作成功    false:操作失败
************************************************************************************************************************/ 
bool vGPIO_AssistBmsOpen(bool en)
{
	if(en == true)
	{
		tSysInfo.uInit.tFinish.bIF_Gpio = 0;  //关闭初始化,预防按键误触发
		gpioASSIST_OPEN_ON();
		xTimerReset(tWakeUpBmsTimer,0);  //开启软件定时,2S后关闭
		
		if(uPrint.tFlag.bSysTask)
			sMyPrint("BMS电源辅助开启打开\r\n");
		
		return  true;
	}
	else 
	{
		tSysInfo.uInit.tFinish.bIF_Gpio = 1;  //初始化完成
		gpioASSIST_OPEN_OFF();
		
		if(uPrint.tFlag.bSysTask)
			sMyPrint("定时2S到达,关闭BMS辅助开启\r\n");
		
		return  true;
	}
}


#if(boardLOW_POWER)
/***********************************************************************************************************************
-----函数功能    IO口进入低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void vKey_EnterLowPower(void)
{
	rcu_periph_clock_disable(RCU_GPIOA);
	rcu_periph_clock_disable(RCU_GPIOB);
	rcu_periph_clock_disable(RCU_GPIOD); 
	rcu_periph_clock_disable(RCU_GPIOE);
	rcu_periph_clock_disable(RCU_GPIOF);
	rcu_periph_clock_disable(RCU_GPIOG);
	
	/* enable clock */
    rcu_periph_clock_enable(RCU_PMU);
	rcu_periph_clock_enable(KEY_0_GPIO_RCU);     //C
	rcu_periph_clock_enable(KEY_WP_RCU);        //A
	rcu_periph_clock_enable(attiSENSOR_INT_RCU);//A
	rcu_periph_clock_enable(PRINT_RX_RCU);      //D
	rcu_periph_clock_enable(RCU_AF);
	
	gpio_init(KEY_0_GPIO_PORT,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_2MHZ,KEY_0_GPIO_PIN);             //中断 电源按键 PC13
	gpio_init(KEY_WP_GPIO,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_2MHZ,KEY_WP_PIN);                   //中断 唤醒脚 PA0
	gpio_init(KEY_KEY1_GPIO,GPIO_MODE_AIN,GPIO_OSPEED_2MHZ,KEY_KEY1_PIN);
	gpio_init(attiSENSOR_INT_GPIO,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_2MHZ,attiSENSOR_INT_PIN);   //中断 状态传感器 PA9
	gpio_init(PRINT_RX_GPIO,GPIO_MODE_IN_FLOATING,GPIO_OSPEED_2MHZ,PRINT_RX_PIN);               //中断 串口接收   PD2
	
	/* enable and set key EXTI interrupt to the lowest priority */
	nvic_irq_enable(EXTI10_15_IRQn, 2U, 0U);
	nvic_irq_enable(EXTI5_9_IRQn, 2U, 0U);
	nvic_irq_enable(EXTI2_IRQn, 2U, 0U);
	nvic_irq_enable(EXTI0_IRQn, 2U, 0U);

	/* connect key EXTI line to key GPIO pin */
	gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOC, GPIO_PIN_SOURCE_13); //PC13
	gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOA, GPIO_PIN_SOURCE_9);  //PA9
	gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOD, GPIO_PIN_SOURCE_2);  //PD2
	gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOA, GPIO_PIN_SOURCE_0);  //PA0

	/* configure key EXTI line */
	exti_init(EXTI_13, EXTI_INTERRUPT, EXTI_TRIG_FALLING); //下降沿触发
	exti_init(EXTI_9, EXTI_INTERRUPT, EXTI_TRIG_RISING);   //上升沿触发
	exti_init(EXTI_2, EXTI_INTERRUPT, EXTI_TRIG_RISING);   //上升沿触发
	exti_init(EXTI_0, EXTI_INTERRUPT, EXTI_TRIG_RISING);   //上升沿触发
	exti_interrupt_flag_clear(EXTI_13);
	exti_interrupt_flag_clear(EXTI_9);
	exti_interrupt_flag_clear(EXTI_2);
	exti_interrupt_flag_clear(EXTI_0);
	exti_interrupt_enable(EXTI_13);//
	exti_interrupt_enable(EXTI_9);//
	exti_interrupt_enable(EXTI_2);//
	exti_interrupt_enable(EXTI_0);//
}

/***********************************************************************************************************************
-----函数功能    IO口退出低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void vGPIO_ExitLowPower(void)
{
	vGPIO_Init();
}



/***********************************************************************************************************************
-----函数功能    IO口进入APP程序
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void vGPIO_EnterApp(void)
{
	gpio_init(KEY_0_GPIO_PORT,GPIO_MODE_AIN,GPIO_OSPEED_2MHZ,KEY_0_GPIO_PIN);             //中断 电源按键   PC13
	gpio_init(KEY_WP_GPIO,GPIO_MODE_AIN,GPIO_OSPEED_2MHZ,KEY_WP_PIN);                   //中断 唤醒脚     PA0
	gpio_init(KEY_KEY1_GPIO,GPIO_MODE_AIN,GPIO_OSPEED_2MHZ,KEY_KEY1_PIN);
	gpio_init(attiSENSOR_INT_GPIO,GPIO_MODE_AIN,GPIO_OSPEED_2MHZ,attiSENSOR_INT_PIN);   //中断 状态传感器 PA9
//	gpio_init(PRINT_RX_GPIO,GPIO_MODE_AIN,GPIO_OSPEED_2MHZ,PRINT_RX_PIN);               //中断 串口接收   PD2
	
	nvic_irq_disable(EXTI10_15_IRQn);
	nvic_irq_disable(EXTI5_9_IRQn);
	nvic_irq_disable(EXTI2_IRQn);
	nvic_irq_disable(EXTI0_IRQn);
	
	exti_interrupt_flag_clear(EXTI_13);
	exti_interrupt_flag_clear(EXTI_9);
	exti_interrupt_flag_clear(EXTI_2);
	exti_interrupt_flag_clear(EXTI_0);
	exti_interrupt_disable(EXTI_13);//
	exti_interrupt_disable(EXTI_9);//
	exti_interrupt_disable(EXTI_2);//
	exti_interrupt_disable(EXTI_0);//
}

#endif





