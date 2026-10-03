/*******************************************************************************************************************************
 * Project : ESP32-C3
 * Module  : G:\TempFile\ESP32-C3\Hardware\BSP
 * File    : bsp_humi_gxhtc3.c
 * Date    : 2026-01-23 23:21:17
 * Author  : LJD(291483914@qq.com)
 * Desc    : 温湿度传感器Gxhtc3
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
*******************************************************************************************************************************/


//****************************************************Includes******************************************************************//
#include "bsp_gxhtc3_temp_humi.h"

#if(1)
#include <math.h>
#include "driver/i2c.h"
#include "esp_err.h"
#include "Print/print_task.h"

//****************************************************Macros*******************************************************************//
#define     	gxhtc3TASK_WORK_CYCLE_TIME				10 //任务时间
#define         POLYNOMIAL                              0x31 // P(x) = x^8 + x^5 + x^4 + 1 = 00110001



//****************************************************Parameter Initialization************************************************//
uint16_t rawValueTemp, rawValueHumi;
float temp=0, humi=0;
uint8_t temp_int, humi_int;
uint8_t tah_data[6];
Gxhtc3_T tGxhtc3;

//****************************************************Function Declaration****************************************************//
static uint8_t uc_gxhtc3_calc_crc(uint8_t *crcdata, uint8_t len);
static esp_err_t s_gxhtc3_read_id(void);
static esp_err_t s_gxhtc3_wake_up(void);
static esp_err_t s_gxhtc3_measure(void);
static esp_err_t s_gxhtc3_read_tah(void);
static esp_err_t s_gxhtc3_sleep(void);


/***********************************************************************************************************************
-----函数功能   温湿度传感器初始化
-----作者       LJD
-----日期       2026-01-23
************************************************************************************************************************/
bool bGxhtc3_IfaceInit(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    if(i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0) != ESP_OK)
        return false;
    return true;
}

/***********************************************************************************************************************
-----函数功能   获取并计算温湿度数据
-----返回值     esp_err_t
-----作者       LJD
-----日期       2026-01-23
************************************************************************************************************************/
s8 cGxhtc3_GetTempHumi(void)
{
    int ret;

    static u8 index = 0;
    static vu16 us_task_delay_cnt = 0;

    switch (index)
    {
        case 0:
        {
             us_task_delay_cnt = 0;
            s_gxhtc3_wake_up();
            s_gxhtc3_measure();
            index++;
        }
        break;

        case 1:
        {
           us_task_delay_cnt++;
           if(us_task_delay_cnt >= (20 / gxhtc3TASK_WORK_CYCLE_TIME))
           {
                us_task_delay_cnt = 0;
                index++;
           }
        }
        break;

        case 2:
        {
            s_gxhtc3_read_tah();
            s_gxhtc3_sleep();

            if((tah_data[2]!=uc_gxhtc3_calc_crc(tah_data,2)||(tah_data[5]!=uc_gxhtc3_calc_crc(&tah_data[3],2))))
            {     
                temp = 0;
                humi = 0;
                temp_int = 0;
                humi_int = 0;
                ret = ESP_FAIL;
            }
            else
            {
                rawValueTemp = (tah_data[0]<<8) | tah_data[1];
                rawValueHumi = (tah_data[3]<<8) | tah_data[4];
                tGxhtc3.usTemp = (175.0 * (float)rawValueTemp) / 65535.0 - 45.0; 
                tGxhtc3.usHumi = (100.0 * (float)rawValueHumi) / 65535.0;
                temp_int = round(temp);
                humi_int = round(humi);
                ret = ESP_OK;
            }

            //失败,重新获取
            if(ret != ESP_OK)
            {
                index = 0;
                 return -1;
            }

            sMyPrint("SysTask:温度%d, 湿度%d \r\n", tGxhtc3.usTemp, tGxhtc3.usHumi);
            
            //成功,延时获取
            index++;
            return 1;
        }
        break;

        case 3:
        {
            us_task_delay_cnt++;
            if(us_task_delay_cnt >= (1000 / gxhtc3TASK_WORK_CYCLE_TIME))
            {
                    us_task_delay_cnt = 0;
                    index = 0;
            }
        }
        break;

        default:
            break;
    }

    return 0;
}

/***********************************************************************************************************************
-----函数功能   CRC校验
-----传入参数   crcdata
-----传入参数   len
-----返回值     uint8_t
-----作者       LJD
-----日期       2026-01-23
************************************************************************************************************************/
static uint8_t uc_gxhtc3_calc_crc(uint8_t *crcdata, uint8_t len)
{
    uint8_t crc = 0xFF; 
  
    for(uint8_t i = 0; i < len; i++)
    {
        crc ^= (crcdata[i]);
        for(uint8_t j = 8; j > 0; --j)
        {
            if(crc & 0x80) crc = (crc << 1) ^ POLYNOMIAL;
            else           crc = (crc << 1);
        }
    }
    return crc;
}

/***********************************************************************************************************************
-----函数功能   读取ID
-----返回值     esp_err_t
-----作者       LJD
-----日期       2026-01-23
************************************************************************************************************************/
static esp_err_t s_gxhtc3_read_id(void)
{
    esp_err_t ret;
    uint8_t data[3];

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0x70 << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0xEF, true);
    i2c_master_write_byte(cmd, 0xC8, true);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000);
    if (ret != ESP_OK) {
        goto end;
    }
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0x70 << 1 | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 3, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000);

    if(data[2]!=uc_gxhtc3_calc_crc(data,2)){     
        ret = ESP_FAIL;
    }
end:
    i2c_cmd_link_delete(cmd);

    return ret;
}

/***********************************************************************************************************************
-----函数功能   唤醒
-----返回值     esp_err_t
-----作者       LJD
-----日期       2026-01-23
************************************************************************************************************************/
static esp_err_t s_gxhtc3_wake_up(void)
{
    int ret;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0x70 << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x35, true);
    i2c_master_write_byte(cmd, 0x17, true);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000);

    i2c_cmd_link_delete(cmd);
    return ret;
}

/***********************************************************************************************************************
-----函数功能   测量
-----返回值     esp_err_t
-----作者       LJD
-----日期       2026-01-23
************************************************************************************************************************/
static esp_err_t s_gxhtc3_measure(void)
{
    int ret;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0x70 << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x7c, true);
    i2c_master_write_byte(cmd, 0xa2, true);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000);

    i2c_cmd_link_delete(cmd);
    return ret;
}

/***********************************************************************************************************************
-----函数功能   读出温湿度数据
-----返回值     esp_err_t
-----作者       LJD
-----日期       2026-01-23
************************************************************************************************************************/
static esp_err_t s_gxhtc3_read_tah(void)
{
    int ret;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0x70 << 1 | I2C_MASTER_READ, true);
    i2c_master_read(cmd, tah_data, 6, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000);

    i2c_cmd_link_delete(cmd);
    return ret;
}

/***********************************************************************************************************************
-----函数功能   休眠
-----返回值     esp_err_t
-----作者       LJD
-----日期       2026-01-23
************************************************************************************************************************/
static esp_err_t s_gxhtc3_sleep(void)
{
    int ret;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0x70 << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0xB0, true);
    i2c_master_write_byte(cmd, 0x98, true);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000);

    i2c_cmd_link_delete(cmd);
    return ret;
}


#endif  //
