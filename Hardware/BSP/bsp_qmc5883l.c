/*****************************************************************************************************************
*                                            QMC5883L 3轴磁力计驱动                                              *
*                                      封装层 - 基于esp-idf-lib组件库                                            *
******************************************************************************************************************/
#include "BSP/bsp_qmc5883l.h"
#include <esp_log.h>
#include <math.h>

static const char *TAG = "qmc5883l";

// 全局对象
Qmc5883l_T   tQmc5883l;

/*****************************************************************************************************************
-----函数功能    接口初始化
-----说明(备注)  初始化I2C接口
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vQmc5883l_IfaceInit(void)
{
    // 默认配置
    tQmc5883l.tConfig.eMode = QMC5883L_MODE_CONTINUOUS;
    tQmc5883l.tConfig.eOdr = QMC5883L_ODR_100HZ;
    tQmc5883l.tConfig.eRange = QMC5883L_RNG_2G;
    tQmc5883l.tConfig.eOsr = QMC5883L_OSR_512;
    
    tQmc5883l.eDevState = DS_INIT;
}

/*****************************************************************************************************************
-----函数功能    初始化
-----说明(备注)  初始化传感器
-----传入参数    none
-----输出参数    none
-----返回值      小于0:失败   大于0:成功
******************************************************************************************************************/
s8 cQmc5883l_Init(void)
{
    esp_err_t e_ret;
    u8 uc_id;
    
    // 接口初始化
    vQmc5883l_IfaceInit();
    
    // 初始化I2C设备描述符 (根据网表: IO0_I2C_SDA, IO1_I2C_SCL)
    e_ret = qmc5883l_init_desc(&tQmc5883l.tDev, QMC5883L_I2C_ADDR, I2C_NUM_0, 0, 1);
    if(e_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C init failed");
        tQmc5883l.eDevState = DS_LOST;
        return -1;
    }
    
    // 复位设备
    e_ret = qmc5883l_reset(&tQmc5883l.tDev);
    if(e_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Reset failed");
        return -2;
    }
    
    // 读取芯片ID
    e_ret = qmc5883l_get_chip_id(&tQmc5883l.tDev, &uc_id);
    if(e_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Read ID failed");
        return -3;
    }
    ESP_LOGI(TAG, "Chip ID: 0x%02X", uc_id);
    
    // 配置传感器
    e_ret = qmc5883l_set_config(&tQmc5883l.tDev, tQmc5883l.tConfig.eOdr, 
                                tQmc5883l.tConfig.eOsr, tQmc5883l.tConfig.eRange);
    if(e_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Config failed");
        return -4;
    }
    
    // 设置工作模式
    e_ret = qmc5883l_set_mode(&tQmc5883l.tDev, tQmc5883l.tConfig.eMode);
    if(e_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Set mode failed");
        return -5;
    }
    
    tQmc5883l.eDevState = DS_WORK;
    ESP_LOGI(TAG, "Init success");
    
    return 1;
}

/*****************************************************************************************************************
-----函数功能    配置传感器
-----说明(备注)  配置量程、ODR、过采样率
-----传入参数    p_config:配置参数指针
-----输出参数    none
-----返回值      小于0:失败   大于0:成功
******************************************************************************************************************/
s8 cQmc5883l_Config(Qmc5883l_Config_T *p_config)
{
    esp_err_t e_ret;
    
    if(p_config == NULL)
    {
        return -1;
    }
    
    tQmc5883l.tConfig = *p_config;
    
    e_ret = qmc5883l_set_config(&tQmc5883l.tDev, p_config->eOdr, p_config->eOsr, p_config->eRange);
    if(e_ret != ESP_OK)
    {
        return -2;
    }
    
    e_ret = qmc5883l_set_mode(&tQmc5883l.tDev, p_config->eMode);
    if(e_ret != ESP_OK)
    {
        return -3;
    }
    
    return 1;
}

/*****************************************************************************************************************
-----函数功能    设置工作模式
-----说明(备注)  设置待机或连续测量模式
-----传入参数    e_mode:工作模式
-----输出参数    none
-----返回值      小于0:失败   大于0:成功
******************************************************************************************************************/
s8 cQmc5883l_SetMode(Qmc5883l_Mode_E e_mode)
{
    esp_err_t e_ret;
    
    tQmc5883l.tConfig.eMode = e_mode;
    
    e_ret = qmc5883l_set_mode(&tQmc5883l.tDev, e_mode);
    if(e_ret != ESP_OK)
    {
        return -1;
    }
    
    return 1;
}

/*****************************************************************************************************************
-----函数功能    检查数据是否就绪
-----说明(备注)  检查数据就绪状态
-----传入参数    none
-----输出参数    none
-----返回值      true:数据就绪  false:数据未就绪
******************************************************************************************************************/
bool bQmc5883l_DataReady(void)
{
    bool b_ready = false;
    
    if(qmc5883l_data_ready(&tQmc5883l.tDev, &b_ready) == ESP_OK)
    {
        return b_ready;
    }
    
    return false;
}

/*****************************************************************************************************************
-----函数功能    读取原始数据
-----说明(备注)  读取磁场原始值
-----传入参数    p_data:数据指针
-----输出参数    none
-----返回值      小于0:失败   大于0:成功
******************************************************************************************************************/
s8 cQmc5883l_ReadRawData(Qmc5883l_RawData_T *p_data)
{
    esp_err_t e_ret;
    
    if(p_data == NULL)
    {
        return -1;
    }
    
    e_ret = qmc5883l_get_raw_data(&tQmc5883l.tDev, p_data);
    if(e_ret != ESP_OK)
    {
        return -2;
    }
    
    return 1;
}

/*****************************************************************************************************************
-----函数功能    读取物理数据
-----说明(备注)  读取带单位转换的数据(毫高斯)
-----传入参数    p_data:数据指针
-----输出参数    none
-----返回值      小于0:失败   大于0:成功
******************************************************************************************************************/
s8 cQmc5883l_ReadData(Qmc5883l_Data_T *p_data)
{
    esp_err_t e_ret;
    
    if(p_data == NULL)
    {
        return -1;
    }
    
    e_ret = qmc5883l_get_data(&tQmc5883l.tDev, p_data);
    if(e_ret != ESP_OK)
    {
        return -2;
    }
    
    // 更新全局数据
    tQmc5883l.tData = *p_data;
    
    return 1;
}

/*****************************************************************************************************************
-----函数功能    获取磁航向角
-----说明(备注)  计算磁航向角
-----传入参数    none
-----输出参数    none
-----返回值      磁航向角(度)
******************************************************************************************************************/
float fQmc5883l_GetHeading(void)
{
    Qmc5883l_Data_T t_data;
    float f_heading;
    
    if(cQmc5883l_ReadData(&t_data) > 0)
    {
        // 计算磁航向角 (度)
        f_heading = atan2f(t_data.y, t_data.x) * 180.0f / 3.14159265f;
        if(f_heading < 0.0f)
        {
            f_heading += 360.0f;
        }
        return f_heading;
    }
    
    return 0.0f;
}