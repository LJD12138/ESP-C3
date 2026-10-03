
/****************************************************/

    /*                                             
                   _ooOoo_
                  o8888888o
                  88" . "88
                  (| -_- |)
                  O\  =  /O
               ____/`---'\____
            .'  \\|     |//  `.
            /  \\|||  :  |||//  \
           /  _||||| -:- |||||-  \
           |   | \\\  -  /// |   |
           | \_|  ''\---/''  |   |
           \  .-\__  `-`  ___/-. /
         ___`. .'  /--.--\  `. . __
      ."" '<  `.___\_<|>_/___.'  >'"".
     | | :  `- \`.;`\ _ /`;.`/ - ` : | |
     \  \ `-.   \_ __\ /__ _/   .-` /  /
======`-.____`-.___\_____/___.-`____.-'======
                   `=---='
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    佛祖保佑       永不宕机     永无BUG
***************************************************/
#include "main.h"
#include "board_config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//****************************************************任务初始化**************************************************//
TaskHandle_t    StartTask_Handler; 
#define      	START_TASK_PRIO             			1                   		//任务优先级 
void vBoard_StartTask(void *pvParameters);


void app_main(void)
{
    vBoard_SysInit();

    // 创建开始任务
    xTaskCreate((TaskFunction_t )vBoard_StartTask,      //任务函数
                (const char* )"StartTask",              //任务名称
                (uint16_t ) 4096,                       //任务堆栈大小 (增大至4096以支持日志输出)
                (void* )NULL,                           //传递给任务函数的参数
                (UBaseType_t ) START_TASK_PRIO,         //任务优先级
                (TaskHandle_t*)&StartTask_Handler);     //任务句柄
    
    // vTaskStartScheduler(); 
}
