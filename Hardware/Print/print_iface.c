#include "board_config.h"

#if(boardPRINT_IFACE)
#include "Print/print_iface.h"
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"

#include "lwrb.h"

#if(boardIC_TYPE == boardIC_ESP32C3 || boardIC_TYPE == boardIC_ESP32C61)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "soc/uart_reg.h"
#endif  //boardIC_TYPE

#if(!boardUSE_OS)
#include "systick.h"
#else
#include "timer_task.h"
#endif  //boardUSE_OS

//****************************************************宏定义**************************************************// 
#define    		printRX_DMA_BUFF_SIZE   				256
#define    		printTX_DMA_BUFF_SIZE   				256
#define    		printUART_TX_TIMEOUT_MS					1000  // 发送超时时间(ms)


//****************************************************参数初始化**************************************************// 
static volatile bool 	bDmaTxBusy = false;	//DMA发送忙标志
static volatile size_t 	S_DataSendSize = 0;

static __ALIGNED(4) u8 ucaPrintRxDmaBuffData[printRX_DMA_BUFF_SIZE];   //用于接收数据
static __ALIGNED(4) u8 ucaPrintTxDmaBuffData[printTX_DMA_BUFF_SIZE];   	//用于把数据装载到DMA发送

#if(boardIC_TYPE == boardIC_ESP32C3 || boardIC_TYPE == boardIC_ESP32C61)
static uart_event_t tUartEvent;
static QueueHandle_t tUartQueue;
#endif  //boardIC_TYPE


/***********************************************************************************************************************
-----函数功能    printf接口映射
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
#if boardCM_BACKTRACE
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 
 
FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{
	#if(boardIC_TYPE == boardIC_GD32F30X)
	usart_data_transmit(printUSART, (uint8_t) ch);
    while(RESET == usart_flag_get(printUSART, USART_FLAG_TBE));
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	HAL_UART_Transmit(&UartHandle, (uint8_t *)&ch, 1, 0xFFFF);
	#endif
    return ch;
}
#endif 

/***********************************************************************************************************************
-----函数功能    IO口初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_print_gpio_init(void)
{
	// //458 发射使能 
	// #if(boardPRINT_485_IFACE_EN)
    
	// #endif
	
	// //接口使能
}

/***********************************************************************************************************************
-----函数功能    串口初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_print_usart_init( void )
{
	const uart_config_t uart_config = {
        .baud_rate = printUSART_BAUD,             // 波特率（如 115200）
        .data_bits = UART_DATA_8_BITS,            // 数据位为 8 位
        .parity = UART_PARITY_DISABLE,            // 禁用奇偶校验
        .stop_bits = UART_STOP_BITS_1,            // 1 个停止位
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,    // 关闭硬件流控
        .source_clk = UART_SCLK_DEFAULT,          // 使用默认时钟源
    };
    //1.串口配置
    uart_param_config(printUSART, &uart_config);

    //2.安装串口驱动,注册了串口消息队列(ESP32 UART驱动内部使用DMA)
    //增大RX缓冲区以支持DMA传输
    uart_driver_install(printUSART, printRX_DMA_BUFF_SIZE * 2, printTX_DMA_BUFF_SIZE * 2, 20, &tUartQueue, 0);
    
    //3.中断配置
    #if(printUSART_IRQ_EN)
    uart_intr_config_t intr_conf = {
        .intr_enable_mask = UART_RXFIFO_FULL_INT_ENA | 
                            UART_RXFIFO_TOUT_INT_ENA,
        .rx_timeout_thresh = 10,                         // 超时阈值(单位：比特时间)
        .rxfifo_full_thresh = 112                       // FIFO触发阈值(0-127)
    };
    uart_intr_config(printUSART, &intr_conf);
    uart_enable_rx_intr(printUSART);

    //5.注册中断
    // esp_intr_alloc(ETS_UART0_INTR_SOURCE + printUSART,          // 中断源
    //                ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL1,   // 在IRAM中运行
    //                printUSART_IRQ_HANDLER,                      // ISR函数
    //                (void*)printUSART,                           // 参数
    //                NULL);                                       // 返回句柄
    #endif  //printUSART_IRQ_EN

    //5.引脚配置
    uart_set_pin(printUSART, printUSART_GPIO_TX_PIN, printUSART_GPIO_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}


/***********************************************************************************************************************
-----函数功能    Print初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vPrint_Init( void )//串口初始化
{
	v_print_gpio_init();
	v_print_usart_init();
}

/***********************************************************************************************************************
-----函数功能    端口重置
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vPrint_DeInit( void )
{

}

/*****************************************************************************************************************
-----函数功能    串口发射函数
-----说明(备注)  none
-----传入参数    SendSize:要发送数据的大小
-----输出参数    none
-----返回值      true:成功    false:失败
******************************************************************************************************************/
bool bPrint_DataSendStart(u16 len)
{
	#if(boardPRINT_485_IFACE_EN)
	vPrint_485TransEnable(true);
	#endif
	
	if(len == 0)
		return false;
	
	if(len > printTX_DMA_BUFF_SIZE)
		len = printTX_DMA_BUFF_SIZE;
	
	//取出数据
	S_DataSendSize = lwrb_read(&tPrintTxBuff, ucaPrintTxDmaBuffData, len);

    if(S_DataSendSize == 0)
        return false;
	
	#if(boardPRINT_IFACE_DMA_EN)
    // 检查DMA是否忙碌，如果是则返回失败
    if(bDmaTxBusy)
        return false;
    
    // ESP32的uart_write_bytes内部使用DMA，发送完成后会自动清除TX FIFO
    // 我们不需要手动管理DMA忙标志，直接发送即可
    int ret = uart_write_bytes(printUSART, (const char *)ucaPrintTxDmaBuffData, S_DataSendSize);
    
    // 检查发送是否成功
    if(ret < 0)
    {
        return false;
    }
    
	return true;
	#else
	// 非DMA方式发送
	uart_write_bytes(printUSART, (const char *)ucaPrintTxDmaBuffData, S_DataSendSize);
    return true;
	#endif  //boardPRINT_IFACE_DMA_EN
}

/*****************************************************************************************************************
-----函数功能    485发送使能
-----说明(备注)  none
-----传入参数    en true:开启    false:关闭
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
#if(boardPRINT_485_IFACE_EN)
void vPrint_485TransEnable(bool en)
{
 	if(en == true)
 		printGPIO_485_TX_EN_ON();  //切换为发送模式
 	else
 	{
 		printGPIO_485_TX_EN_OFF();  //切换为接收模式
	
 		#if(!boardUSE_OS)
 		bSysTick_PrintSendFinish = false;
 		#endif
 	}
}
#endif

/***********************************************************************************************************************
-----函数功能    检查发射情况
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送完成    false:发送中  
************************************************************************************************************************/
bool bPrint_CheckSendFinish(void)
{
    #if(boardPRINT_IFACE_DMA_EN)
    // 非阻塞方式检查DMA发送是否完成
	if(bDmaTxBusy)
	{
        // 检查TX FIFO是否为空
        size_t tx_bytes_waiting = 0;
        if(uart_get_buffered_data_len(printUSART, &tx_bytes_waiting) == ESP_OK)
        {
            if(tx_bytes_waiting == 0)
            {
                // FIFO为空，发送完成
                bDmaTxBusy = false;
                S_DataSendSize = 0;
                
                // 485模式下切换回接收模式
                #if(boardPRINT_485_IFACE_EN)
                vPrint_485TransEnable(false);
                #endif
            }
        }
		return false;  // 正在发送
	}
	else 
		return true;  // 发送完成
	#else
	return true;
	#endif  //boardPRINT_IFACE_DMA_EN
}

/***********************************************************************************************************************
-----函数功能    进入低功耗/清除中断
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
#if(boardLOW_POWER)
void vPrint_EnterLowPower( void )
{
	
}
#endif //boardLOW_POWER


#if(boardPRINT_IFACE_DMA_EN)
/***********************************************************************************************************************
-----函数功能    串口接收处理（DMA模式）
-----说明(备注)  DMA模式下使用UART事件队列接收数据
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vPrint_QyqRecData(void)
{
    //这里用于接收串口的消息队列
    if (xQueueReceive(tUartQueue, (void *)&tUartEvent, pdMS_TO_TICKS(10)))
    {
        switch (tUartEvent.type)
        {
            // 数据可读写的状态
            case UART_DATA:
            {
                // 有数据可读
                int len = uart_read_bytes(printUSART, ucaPrintRxDmaBuffData, tUartEvent.size, portMAX_DELAY);
                if(len > 0 && tpPrintProtoRx != NULL)
                {
                    lwrb_write(&tpPrintProtoRx->tRxBuff, ucaPrintRxDmaBuffData, len);
                    
                    // 通知接收任务
                    #if(boardUSE_OS)
                    vTaskNotifyGiveFromISR(tPrintTaskHandler, NULL);
                    #endif  //boardUSE_OS
                }
            }
            break;

            case UART_FIFO_OVF:
            {
                // 硬件数据溢出
                uart_flush_input(printUSART);
                xQueueReset(tUartQueue);
            }
            break;

            case UART_BUFFER_FULL:
            {
                // 环形缓冲区已满
                uart_flush_input(printUSART);
                xQueueReset(tUartQueue);
            }
            break;

            case UART_BREAK:
                break;

            case UART_PARITY_ERR:
                break;

            case UART_FRAME_ERR:
                break;

            case UART_PATTERN_DET:
                break;

            case UART_EVENT_MAX:
                break;

            default:
                break;
        }
    }
}

#else
/***********************************************************************************************************************
-----函数功能    串口接收（非DMA模式）
-----说明(备注)  需要在任务中调用
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static u8 uc_read_buff_len=0;
void vPrint_QyqRecData(void)
{
    //这里用于接收串口的消息队列
    if (xQueueReceive(tUartQueue, (void *)&tUartEvent, (TickType_t)portMAX_DELAY))
    {
        switch (tUartEvent.type)
        {
            // 数据可读写的状态
            case UART_DATA:
            {
                // 有数据可读
                uc_read_buff_len = uart_read_bytes(printUSART, ucaPrintRxDmaBuffData, tUartEvent.size, portMAX_DELAY);
                if(uc_read_buff_len && tpPrintProtoRx != NULL)
                    lwrb_write(&tpPrintProtoRx->tRxBuff, ucaPrintRxDmaBuffData, uc_read_buff_len);
               //通知接收任务
                #if(boardUSE_OS)
                {
                    vTaskNotifyGiveFromISR(tPrintTaskHandler,NULL);
                }
                #endif  //boardUSE_OS
            }
            break;

            case UART_FIFO_OVF:
            {
                // 硬件数据溢出
                uart_flush_input(printUSART);
                xQueueReset(tUartQueue);
            }
            break;

            case UART_BUFFER_FULL:
            {
                // 环形缓冲区已满
                uart_flush_input(printUSART);
                xQueueReset(tUartQueue);
            }
            break;

            case UART_BREAK:
                break;

            case UART_PARITY_ERR:
                break;

            case UART_FRAME_ERR:
                break;

            case UART_PATTERN_DET:
                break;

            default:
                break;
        }
    }
}

#endif  //boardPRINT_IFACE_DMA_EN

#endif  //boardPRINT_IFACE