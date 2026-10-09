#include "MD_Bms/md_bms_iface.h"

#if(boardBMS_IFACE)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
// #include "Updata/updata_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "lwrb.h"

#if(boardIC_TYPE == boardIC_ESP32C3 || boardIC_TYPE == boardIC_ESP32C61)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uhci.h"
#include "soc/uart_reg.h"
#endif  //boardIC_TYPE

#if(boardBMS_485_IFACE_EN)
#if(boardUSE_OS)
#include "timer_task.h"
#else
#include "systick.h"
#endif
#endif  //boardCONSOLE_485_IFACE_EN


//****************************************************参数初始化**************************************************//
#define    		bmsRX_DMA_BUFF_SIZE   					256
#define    		bmsTX_DMA_BUFF_SIZE        				256      //DMA数组大小

static vu16  	S_DataSendSize = 0;
static vu16  	S_DataSendCnt = 0;

static __ALIGNED(4) u8   ucaBmsRxDmaBuffData[bmsRX_DMA_BUFF_SIZE];   //用于把数据装载到DMA发送 
static __ALIGNED(4) u8   ucaBmsTxDmaBuffData[bmsTX_DMA_BUFF_SIZE];   //取出接收缓存器中的数据,用于DMA发送

#if(boardIC_TYPE == boardIC_ESP32C3 || boardIC_TYPE == boardIC_ESP32C61)
typedef enum : uint8_t
{
    UHCI_EVT_DATA,
    UHCI_EVT_EOF,
} uhci_event_t;

// 接收回调结构体定义，如果要使用双buf接收，那么需要定义两个buf空间
typedef struct
{
    QueueHandle_t queue;     // 用于接收DMA传输完成事件的队列句柄
    uint8_t recv_buf1[1024]; // 指向接收缓冲区的指针，用于存储DMA接收到的数据
    uint8_t recv_buf2[1024]; // 指向接收缓冲区的指针，用于存储DMA接收到的数据
    bool use_buf;            // 确定双buf的接收
    uint32_t data_len;       // 接收缓冲区中当前已接收的数据长度
} qyq_uhci_ctx_t;

static uart_event_t tUartEvent;
static QueueHandle_t tUartQueue;
#endif  //boardIC_TYPE

//****************************************************函数声明****************************************************//
#if(boardIC_TYPE == boardIC_ESP32C3 || boardIC_TYPE == boardIC_ESP32C61)
#if(boardPRINT_IFACE_DMA_EN)
static bool uhci_tx_callback(uhci_controller_handle_t uhci_ctrl, const uhci_tx_done_event_data_t *edata, void *user_ctx);
static bool uhci_rx_callback(uhci_controller_handle_t uhci_ctrl, const uhci_rx_event_data_t *edata, void *user_ctx);
#endif  //boardPRINT_IFACE_DMA_EN
#endif  //boardIC_TYPE

/*****************************************************************************************************************
-----函数功能    串口相关IO初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_bms_io_init(void)//IO设置
{
	#if(boardBMS_485_IFACE_EN)
    
	#endif
}


/*****************************************************************************************************************
-----函数功能    串口配置
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_bms_usart_init(void)
{
	const uart_config_t uart_config = {
        .baud_rate = bmsUSART_BAUD,             // 波特率（如 115200）
        .data_bits = UART_DATA_8_BITS,            // 数据位为 8 位
        .parity = UART_PARITY_DISABLE,            // 禁用奇偶校验
        .stop_bits = UART_STOP_BITS_1,            // 1 个停止位
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,    // 关闭硬件流控
        .source_clk = UART_SCLK_DEFAULT,          // 使用默认时钟源
    };
    //1.串口配置
    uart_param_config(bmsUSART, &uart_config);

    //2.安装串口驱动,注册了串口消息队列
    uart_driver_install(bmsUSART, bmsRX_DMA_BUFF_SIZE * 2, 0, 20, &tUartQueue, 0);
    
    //3.中断配置
    #if(bmsUSART_IRQ_EN)
    uart_intr_config_t intr_conf = {
        .intr_enable_mask = UART_RXFIFO_FULL_INT_ENA | 
                            UART_RXFIFO_TOUT_INT_ENA,
        .rx_timeout_thresh = 10,                         // 超时阈值(单位：比特时间)
        .rxfifo_full_thresh = 112                       // FIFO触发阈值(0-127)
    };
    uart_intr_config(bmsUSART, &intr_conf);
    uart_enable_rx_intr(bmsUSART);

    //5.注册中断
    // esp_intr_alloc(ETS_UART0_INTR_SOURCE + bmsUSART,          // 中断源
    //                ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL1,   // 在IRAM中运行
    //                bmsUSART_IRQ_HANDLER,                      // ISR函数
    //                (void*)bmsUSART,                           // 参数
    //                NULL);                                       // 返回句柄
    #endif  //bmsUSART_IRQ_EN

    //5.引脚配置
    uart_set_pin(bmsUSART, bmsUSART_GPIO_TX_PIN, bmsUSART_GPIO_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}



/***********************************************************************************************************************
-----函数功能    DMA初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
#if(boardBMS_IFACE_DMA_EN)
static void v_bms_dma_init(void)
{

}
#endif

/*****************************************************************************************************************
-----函数功能    串口初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vBms_IfaceInit(void)
{
    v_bms_io_init();
    v_bms_usart_init();
	#if(boardBMS_IFACE_DMA_EN)
	v_bms_dma_init();
	#endif
}

/*****************************************************************************************************************
-----函数功能    串口重置
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vBms_IfaceDeInit(void)
{
	#if(boardBMS_IFACE_DMA_EN)

	#endif  //boardBMS_IFACE_DMA_EN
}


/*****************************************************************************************************************
-----函数功能    串口发射函数
-----说明(备注)  none
-----传入参数    addr:要发送数据的地址
-----输出参数    none
-----返回值      true:成功    false:失败
******************************************************************************************************************/
bool bBms_DataSendStart(u8* data,u16 len)
{
	if(data == 0 || len == 0)
		return false;
	
	if(len > bmsTX_DMA_BUFF_SIZE)
		len = bmsTX_DMA_BUFF_SIZE;

    #if(boardBMS_485_IFACE_EN)
	vBms_485TransEnable(true);
	#endif
	
	memcpy(ucaBmsTxDmaBuffData, data, len);
	
	#if(boardBMS_IFACE_DMA_EN)
	//清除全部发送完成标志位
	dma_flag_clear(bmsUSART_DMA, bmsUSART_DMA_TX_CH, DMA_FLAG_FTF); 
	//装载数据
	dma_memory_address_config(bmsUSART_DMA, bmsUSART_DMA_TX_CH,(uint32_t)ucaBmsTxDmaBuffData);
	//装载长度
	dma_transfer_number_config(bmsUSART_DMA,bmsUSART_DMA_TX_CH,len);
	//开始DMA发送
	dma_channel_enable(bmsUSART_DMA, bmsUSART_DMA_TX_CH);
	return true;
	
	#else
	
    if(S_DataSendCnt) //发送中
        return false; 
    
	// 非DMA方式发送
	 S_DataSendSize = len;
	uart_write_bytes(bmsUSART, (const char *)ucaBmsTxDmaBuffData, S_DataSendSize);
    S_DataSendCnt = 0;          
    return true;
	
	#endif
}

/*****************************************************************************************************************
-----函数功能    485发送使能
-----说明(备注)  none
-----传入参数    en true:开启    false:关闭
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
#if(boardBMS_485_IFACE_EN)
void vBms_485TransEnable(bool en)
{
	if(en == true)
		bmsGPIO_485_TX_EN_ON();  //切换为发送模式
	else
	{
		bmsGPIO_485_TX_EN_OFF();  //切换为接收模式
		#if(!boardUSE_OS)
		bSysTick_BmsSendFinish = false;
		#endif  //boardUSE_OS
	}
}
#endif



#if(boardBMS_IFACE_DMA_EN)
/***********************************************************************************************************************
-----函数功能    DMA发送完成中断
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void bmsUSART_DMA_TX_IRQ_HANDLER(void)
{
    if(dma_interrupt_flag_get(bmsUSART_DMA, bmsUSART_DMA_TX_CH, DMA_INT_FLAG_FTF)) 
	{
        dma_interrupt_flag_clear(bmsUSART_DMA, bmsUSART_DMA_TX_CH, DMA_INT_FLAG_G);
		//关闭DMA发送
	    dma_channel_disable(bmsUSART_DMA, bmsUSART_DMA_TX_CH);
		//发送完成
		S_DataSendSize = 0;
		
		//延时关闭
		#if(boardBMS_485_IFACE_EN)
		#if(boardUSE_OS)
		xTimerResetFromISR(tBmsRxEnTimer,0);
		#else
		bSysTick_BmsSendFinish = true;
		#endif  //boardUSE_OS
		#endif  //boardBMS_485_IFACE_EN
    }
}


/***********************************************************************************************************************
-----函数功能    串口接收完成中断
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static u8 uc_read_buff_len=0;
void bmsUSART_IRQ_HANDLER(void)
{
    if(RESET != usart_interrupt_flag_get(bmsUSART, USART_INT_FLAG_IDLE)) 
	{
		//清除中断
        usart_interrupt_flag_clear(bmsUSART, USART_INT_FLAG_IDLE);
		//清除空闲标志位
		usart_data_receive(bmsUSART);
		//关闭DMA传输
		dma_channel_disable(bmsUSART_DMA, bmsUSART_DMA_RX_CH); 
		
		//获取接收到的数据长度，单位：字节
		uc_read_buff_len = bmsRX_DMA_BUFF_SIZE - dma_transfer_number_get(bmsUSART_DMA,bmsUSART_DMA_RX_CH);
		//转存数据到待处理数据缓冲区
		if(tpBmsProtoRx != NULL)
			lwrb_write(&tpBmsProtoRx->tRxBuff, ucaBmsRxDmaBuffData, uc_read_buff_len);
		//通知接收任务
		#if(boardUSE_OS)
		if(tUpdata.eWorkState != UWS_NULL)
		{
			vTaskNotifyGiveFromISR(tUpdataTaskHandle,NULL);
		}
		else 
		{
			vTaskNotifyGiveFromISR(tBmsRecTaskHandle,NULL);
		}
		#endif  //boardUSE_OS

		//重新设置DMA传输
		//装载长度
		dma_transfer_number_config(bmsUSART_DMA,bmsUSART_DMA_RX_CH,bmsRX_DMA_BUFF_SIZE);
		//开启DMA传输
		dma_channel_enable(bmsUSART_DMA, bmsUSART_DMA_RX_CH);    
    }
}
#else
/*****************************************************************************************************************
-----函数功能    串口接收（非DMA模式非中断）
-----说明(备注)  需要在任务中调用
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static u8 uc_read_buff_len=0;
void vBms_QyqRecData(void)
{
    //检查队列是否已初始化
    if(tUartQueue == NULL)
    {
        #if(boardUSE_OS)
        vTaskDelay(bmsREC_TASK_CYCLE_TIME);
        #endif
        return;
    }
        
    //这里用于接收串口的消息队列
    if (xQueueReceive(tUartQueue, (void *)&tUartEvent, (TickType_t)bmsREC_TASK_CYCLE_TIME))
    {
        switch (tUartEvent.type)
        {
            // 数据可读写的状态
            case UART_DATA:
            {
                // 有数据可读
                uc_read_buff_len = uart_read_bytes(bmsUSART, ucaBmsRxDmaBuffData, tUartEvent.size, bmsREC_TASK_CYCLE_TIME);
                if(uc_read_buff_len && tpBmsProtoRx != NULL)
                    lwrb_write(&tpBmsProtoRx->tRxBuff, ucaBmsRxDmaBuffData, uc_read_buff_len);
               //通知接收任务
                #if(boardUSE_OS)
                // if(tUpdata.eWorkState != UWS_NULL)
                // {
                //     vTaskNotifyGiveFromISR(tUpdataTaskHandle,NULL);
                // }
                // else
                {
                    vTaskNotifyGiveFromISR(tBmsRecTaskHandle,NULL);
                }
                #endif  //boardUSE_OS
            }
            break;

            case UART_FIFO_OVF:
            {
                // 硬件数据溢出
                uart_flush_input(bmsUSART);
                xQueueReset(tUartQueue);
            }
            break;

            case UART_BUFFER_FULL:
            {
                // 环形缓冲区已满
                uart_flush_input(bmsUSART);
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
#endif  //boardBMS_IFACE_DMA_EN

#endif  //boardBMS_IFACE














