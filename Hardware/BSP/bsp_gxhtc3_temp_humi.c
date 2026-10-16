/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\BSP
 * File    : bsp_gxhtc3_temp_humi.c
 * Date    : 2026-01-23
 * Author  : LJD(291483914@qq.com)
 * Desc    : 温湿度传感器Gxhtc3驱动
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "bsp_gxhtc3_temp_humi.h"

#if (1)
#include <math.h>
#include <stdlib.h>
#include "driver/i2c.h"
#include "esp_err.h"
#include "Print/print_task.h"

//****************************************************Macros********************************************************************//
#define			gxhtc3TASK_WORK_CYCLE_TIME				10		// 任务调度周期(ms)
#define			gxhtc3MEASURE_WAIT_TIME_MS				20		// 测量等待时间(ms, 规格要求>14.4ms)
#define			gxhtc3SAMPLE_INTERVAL_MS				1000	// 采样间隔时间(ms)
#define			GXHTC3_CMD_WAKEUP						0x3517	// 唤醒命令
#define			GXHTC3_CMD_SLEEP						0xB098	// 休眠命令
#define			GXHTC3_CMD_MEASURE_NORMAL_T_FIRST		0x7CA2	// 常规测量(禁用时钟延展, 温度优先)
#define			GXHTC3_CMD_READ_ID						0xEFC8	// 读取ID命令
#define			GXHTC3_CMD_SOFT_RESET					0x8005	// 软件复位命令
#define			GXHTC3_CRC_POLYNOMIAL					0x31	// CRC8多项式: x^8 + x^5 + x^4 + 1
#define			GXHTC3_CRC_INIT							0xFF	// CRC8初始值

//****************************************************Parameter Initialization**************************************************//
static struct
{
	bool				bInited;			//是否初始化
	vu16				usHumi;				//湿度 %
	vs16				sTemp;				//温度 0.1摄氏度
}tGxhtc3 = {
	.bInited = false,
	.usHumi  = 0xFF,
	.sTemp   = 0x7FFF,
};

//****************************************************Function Declaration******************************************************//
static uint8_t   uc_gxhtc3_calc_crc(const uint8_t *p_data, uint8_t uc_len);
static esp_err_t s_gxhtc3_write_cmd(uint16_t us_cmd);
static esp_err_t s_gxhtc3_read_bytes(uint8_t *p_buff, uint8_t uc_len);
static esp_err_t s_gxhtc3_wake_up(void);
static esp_err_t s_gxhtc3_sleep(void);
static esp_err_t s_gxhtc3_measure(void);
static esp_err_t s_gxhtc3_read_tah(uint8_t *p_buff);

/***********************************************************************************************************************
 * 函数功能    : 温湿度传感器接口初始化
 * 说明(备注)  : 配置I2C总线并检测传感器在线状态
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:成功  false:失败
 ************************************************************************************************************************/
bool bGxhtc3_IfaceInit(void)
{
    //板级共享 I2C 总线,所以这部分注释掉
    // int i2c_master_port = I2C_MASTER_NUM;

    // i2c_config_t conf = {
    //     .mode = I2C_MODE_MASTER,
    //     .sda_io_num = I2C_MASTER_SDA_IO,
    //     .scl_io_num = I2C_MASTER_SCL_IO,
    //     .sda_pullup_en = GPIO_PULLUP_ENABLE,
    //     .scl_pullup_en = GPIO_PULLUP_ENABLE,
    //     .master.clk_speed = I2C_MASTER_FREQ_HZ,
    // };

    // i2c_param_config(i2c_master_port, &conf);

    // esp_err_t err = i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    // if ((err != ESP_OK) && (err != ESP_ERR_INVALID_STATE))
    //     return false;

    uint16_t us_id = 0;
    if (cGxhtc3_ReadId(&us_id) < 0)
        return false;

    tGxhtc3.bInited      = true;

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 获取并计算温湿度数据
 * 说明(备注)  : 周期状态机调度获取数据并计算温湿度
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : 小于0:失败  0:测量中  1:完成一次采集更新
 ************************************************************************************************************************/
s8 cGxhtc3_GetTempHumi(void)
{
    static uint8_t  s_uc_step = 0;
    static uint16_t s_us_delay_cnt = 0;

    switch (s_uc_step)
    {
        case 0:
        {
            s_us_delay_cnt = 0;
            if (s_gxhtc3_wake_up() != ESP_OK)
                return -1;

            if (s_gxhtc3_measure() != ESP_OK)
            {
                s_gxhtc3_sleep();
                return -1;
            }
            s_uc_step++;
        }
        break;

        case 1:
        {
            s_us_delay_cnt++;
            if (s_us_delay_cnt >= (gxhtc3MEASURE_WAIT_TIME_MS / gxhtc3TASK_WORK_CYCLE_TIME))
            {
                s_us_delay_cnt = 0;
                s_uc_step++;
            }
        }
        break;

        case 2:
        {
            uint8_t tah_data[6] = {0};

            esp_err_t e_ret = s_gxhtc3_read_tah(tah_data);
            s_gxhtc3_sleep();

            if (e_ret != ESP_OK)
            {
                s_uc_step = 0;
                return -1;
            }

            if ((tah_data[2] != uc_gxhtc3_calc_crc(tah_data, 2)) ||
                (tah_data[5] != uc_gxhtc3_calc_crc(&tah_data[3], 2)))
            {
                s_uc_step = 0;
                return -1;
            }

            uint16_t us_raw_temp = (uint16_t)((tah_data[0] << 8) | tah_data[1]);
            uint16_t us_raw_humi = (uint16_t)((tah_data[3] << 8) | tah_data[4]);
            float f_temp = (175.0f * (float)us_raw_temp) / 65535.0f - 45.0f;
            float f_humi = (100.0f * (float)us_raw_humi) / 65535.0f;

            if (f_humi < 0.0f)
                f_humi = 0.0f;
            else if (f_humi > 100.0f)
                f_humi = 100.0f;

            tGxhtc3.sTemp        = (vs16)roundf(f_temp * 10.0f);
            tGxhtc3.usHumi       = (vu16)roundf(f_humi);

            // sMyPrint("SysTask:温度%d.%d°C, 湿度%d%%\r\n", tGxhtc3.sTemp / 10, abs(tGxhtc3.sTemp % 10), tGxhtc3.usHumi);

            s_uc_step++;
            return 1;
        }
        break;

        case 3:
        {
            s_us_delay_cnt++;
            if (s_us_delay_cnt >= (gxhtc3SAMPLE_INTERVAL_MS / gxhtc3TASK_WORK_CYCLE_TIME))
            {
                s_us_delay_cnt = 0;
                s_uc_step = 0;
            }
        }
        break;

        default:
        {
            s_uc_step = 0;
            s_us_delay_cnt = 0;
        }
        break;
    }

    return 0;
}

/***********************************************************************************************************************
 * 函数功能    : 读取传感器芯片ID
 * 说明(备注)  : 唤醒并发送0xEFC8命令读取2字节ID
 * 传入参数    : none
 * 输出参数    : p_id:输出16位ID指针
 * 返回值      : 1:成功  小于0:失败
 ************************************************************************************************************************/
s8 cGxhtc3_ReadId(uint16_t *p_id)
{
    uint8_t data[3] = {0};

    esp_err_t ret = s_gxhtc3_wake_up();
    if (ret != ESP_OK)
        return -1;

    ret = s_gxhtc3_write_cmd(GXHTC3_CMD_READ_ID);
    if (ret != ESP_OK)
    {
        s_gxhtc3_sleep();
        return -2;
    }

    ret = s_gxhtc3_read_bytes(data, sizeof(data));
    s_gxhtc3_sleep();

    if (ret != ESP_OK)
        return -3;

    if (data[2] != uc_gxhtc3_calc_crc(data, 2))
        return -4;

    if (p_id != NULL)
        *p_id = (uint16_t)((data[0] << 8) | data[1]);

    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 传感器软件复位
 * 说明(备注)  : 发送0x8005命令复位传感器
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : 1:成功  小于0:失败
 ************************************************************************************************************************/
s8 cGxhtc3_SoftReset(void)
{
    esp_err_t ret = s_gxhtc3_write_cmd(GXHTC3_CMD_SOFT_RESET);
    return (ret == ESP_OK) ? 1 : -1;
}

/***********************************************************************************************************************
 * 函数功能    : 获取当前温度放大10倍后的整数值
 * 说明(备注)  : 单位 0.1°C，未就绪时返回 0x7FFF
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : 温度*10
 ************************************************************************************************************************/
int16_t sGxhtc3_GetTempX10(void)
{
    return tGxhtc3.sTemp;
}

/***********************************************************************************************************************
 * 函数功能    : 获取当前湿度整数百分比
 * 说明(备注)  : 范围 0~100%，未就绪时返回 0xFF
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : 湿度%
 ************************************************************************************************************************/
uint8_t ucGxhtc3_GetHumi(void)
{
    return (uint8_t)tGxhtc3.usHumi;
}

/***********************************************************************************************************************
 * 函数功能    : 获取温湿度接口是否初始化
 * 说明(备注)  : 查询当前温湿度是否已有效更新
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:有效  false:无效
 ************************************************************************************************************************/
bool bGxhtc3_IsIfaceInited(void)
{
    return tGxhtc3.bInited;
}

/***********************************************************************************************************************
 * 函数功能    : 计算CRC校验码
 * 说明(备注)  : 多项式 0x31 (x^8 + x^5 + x^4 + 1)
 * 传入参数    : p_data:待计算数据指针  uc_len:数据长度
 * 输出参数    : none
 * 返回值      : 计算得到的CRC值
 ************************************************************************************************************************/
static uint8_t uc_gxhtc3_calc_crc(const uint8_t *p_data, uint8_t uc_len)
{
    uint8_t uc_crc = GXHTC3_CRC_INIT;

    for (uint8_t i = 0; i < uc_len; i++)
    {
        uc_crc ^= p_data[i];
        for (uint8_t j = 8; j > 0; --j)
        {
            if (uc_crc & 0x80)
                uc_crc = (uc_crc << 1) ^ GXHTC3_CRC_POLYNOMIAL;
            else
                uc_crc = (uc_crc << 1);
        }
    }
    return uc_crc;
}

/***********************************************************************************************************************
 * 函数功能    : 向传感器写入16位命令
 * 说明(备注)  : 内部I2C写封装
 * 传入参数    : us_cmd:16位命令字
 * 输出参数    : none
 * 返回值      : ESP_OK:成功  其它:失败
 ************************************************************************************************************************/
static esp_err_t s_gxhtc3_write_cmd(uint16_t us_cmd)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL)
        return ESP_ERR_NO_MEM;

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (GXHTC3_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, (uint8_t)(us_cmd >> 8), true);
    i2c_master_write_byte(cmd, (uint8_t)(us_cmd & 0xFF), true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

/***********************************************************************************************************************
 * 函数功能    : 从传感器读取数据字节
 * 说明(备注)  : 内部I2C读封装
 * 传入参数    : p_buff:接收缓冲区指针  uc_len:读取长度
 * 输出参数    : p_buff:存储读取的数据
 * 返回值      : ESP_OK:成功  其它:失败
 ************************************************************************************************************************/
static esp_err_t s_gxhtc3_read_bytes(uint8_t *p_buff, uint8_t uc_len)
{
    if (p_buff == NULL || uc_len == 0)
        return ESP_ERR_INVALID_ARG;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == NULL)
        return ESP_ERR_NO_MEM;

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (GXHTC3_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, p_buff, uc_len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

/***********************************************************************************************************************
 * 函数功能    : 唤醒传感器
 * 说明(备注)  : 发送唤醒命令 0x3517
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : ESP_OK:成功  其它:失败
 ************************************************************************************************************************/
static esp_err_t s_gxhtc3_wake_up(void)
{
    return s_gxhtc3_write_cmd(GXHTC3_CMD_WAKEUP);
}

/***********************************************************************************************************************
 * 函数功能    : 使传感器进入休眠模式
 * 说明(备注)  : 发送休眠命令 0xB098
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : ESP_OK:成功  其它:失败
 ************************************************************************************************************************/
static esp_err_t s_gxhtc3_sleep(void)
{
    return s_gxhtc3_write_cmd(GXHTC3_CMD_SLEEP);
}

/***********************************************************************************************************************
 * 函数功能    : 启动测量
 * 说明(备注)  : 发送常规测量命令 0x7CA2
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : ESP_OK:成功  其它:失败
 ************************************************************************************************************************/
static esp_err_t s_gxhtc3_measure(void)
{
    return s_gxhtc3_write_cmd(GXHTC3_CMD_MEASURE_NORMAL_T_FIRST);
}

/***********************************************************************************************************************
 * 函数功能    : 读取温湿度测量数据
 * 说明(备注)  : 读取6字节原始数据
 * 传入参数    : p_buff:输出数据缓冲区
 * 输出参数    : p_buff:存储6字节温湿度及CRC
 * 返回值      : ESP_OK:成功  其它:失败
 ************************************************************************************************************************/
static esp_err_t s_gxhtc3_read_tah(uint8_t *p_buff)
{
    return s_gxhtc3_read_bytes(p_buff, 6);
}

#endif  /* 1 */
