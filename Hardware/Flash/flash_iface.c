#include "Flash/flash_iface.h"
#include "Print/print_task.h"

#if(boardUSE_SFUD)
#include <sfud.h>
#elif(boardIC_TYPE == boardIC_GD32F30X)
#include "Flash/flash_gd32.h"
#elif(boardIC_TYPE == boardIC_STM32H7XX)
#include "Flash/flash_stm32.h"
#elif(boardIC_TYPE == boardIC_ESP32C61)
#include "Flash/flash_esp32.h"
#include "esp_partition.h"
#include "esp_log.h"
#endif

//------------------------------------变量常量定义------------------------------------
Flash_T  tFlash;

#if(boardUSE_SFUD)
const sfud_flash *flash = NULL; //获取设备结构体
#ifdef SFUD_USING_QSPI
const sfud_flash *qspi_flash = NULL; //获取设备结构体
#endif  //SFUD_USING_QSPI
#endif  //boardUSE_SFUD

#if(boardIC_TYPE == boardIC_ESP32C61)
const esp_partition_t *tpPartition = NULL;
#endif

/*****************************************************************************************************************
-----函数功能    Flash接口初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
bool bFlash_IfaceInit(void)
{
    #if(boardIC_TYPE == boardIC_ESP32C61)
    tpPartition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "storage");

    if(tpPartition == NULL)
        return false;
    #endif

	#if(boardUSE_SFUD)
	if(sfud_init() == SFUD_SUCCESS)
	{
		//IC1 Flash初始化
		flash = sfud_get_device_table() + 0;  //获取设备信息
		
		//IC2 QSPI初始化
		#ifdef SFUD_USING_QSPI
		sfud_qspi_fast_read_enable(sfud_get_device(SFUD_W25Q_DEVICE_INDEX1), 4);
		qspi_flash = sfud_get_device(SFUD_W25Q_DEVICE_INDEX1);
		#endif
		return true;
	}
	else
		return false;
	#else
	return true;
	#endif  //boardUSE_SFUD
}

/*****************************************************************************************************************
-----函数功能    flash 擦除函数
-----说明(备注)  none
-----传入参数    start_addr:开始地址    end_addr:结束地址
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
s8 cFlash_EraseSector(u32 start_addr,u32 end_addr)
{
	#if(boardIC_TYPE == boardIC_GD32F30X)
	if(bFlash_Gd32EraseSector(star_addr, end_addr) == false)
		return -1;
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	#if(boardUSE_SFUD)
	if(flash == NULL)
		return -1;
	
	if(sfud_erase(flash,star_addr, end_addr) != SFUD_SUCCESS)
		return -2;
	#else
	
	#endif  //boardUSE_SFUD
    #elif(boardIC_TYPE == boardIC_ESP32C61)
    if(tpPartition->address > start_addr)
        return -1;

    u32 erase_size = end_addr - start_addr;

    if(erase_size > tpPartition->size)
        return -2;

    if(esp_partition_erase_range(tpPartition, (start_addr - tpPartition->address), erase_size) != ESP_OK)
    {
        return -3;
    }
        
	#endif  //boardIC_TYPE
	
	
	return 1;
}

/*****************************************************************************************************************
-----函数功能    flash 写入函数,按8位写入
-----说明(备注)  none
-----传入参数    start_addr:开始地址    data:数据    len:字节长度
-----输出参数    none
-----返回值      0:写入成功    反之失败
*****************************************************************************************************************/
s8 cFlash_Write8BitData(u32 start_addr, u8* data, u32 len)
{
	if(data == NULL || len == 0)
		return 0;
	
	
	#if(boardIC_TYPE == boardIC_GD32F30X)
	if(bFlash_Gd32Write32Bit(start_addr,(u32*)data,len) == false)
		return -1;
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	#if(boardUSE_SFUD)
	if(flash == NULL)
		return -1;
	
	if(sfud_write(flash, start_addr, len, data) != SFUD_SUCCESS)
		return -2;
	#else
//	#error "错误,当前STMH7 不带内部Flash操作";
	#endif  //boardUSE_SFUD
    #elif(boardIC_TYPE == boardIC_ESP32C61)
    if(tpPartition->address > start_addr)
        return -1;

    if(esp_partition_write(tpPartition, (start_addr - tpPartition->address), data, len) != ESP_OK)
    {
        return -2;
    }
        
	#endif  //boardIC_TYPE
	

	if(uPrint.tFlag.bOperFlash)
	{
		sMyPrint("\r\n 写入的数据 长度= %d ",len);
		for(int i = 0; i < len; i++)
		{
			sMyPrint("%x ",data[i]);
		}
		sMyPrint("\r\n");
	}
	
	return 1;
}

/*****************************************************************************************************************
-----函数功能    flash 读取函数,按8位读取
-----说明(备注)  none
-----传入参数    start_addr:开始地址    data:存放数据buff的指针   len:字节长度
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
s8 cFlash_Read8BitData(u32 start_addr, u8* data, u32 len)
{
	if(data == NULL || len == 0)
		return 0;
	
	#if(boardIC_TYPE == boardIC_GD32F30X)
	vFlash_Gd32Read8Bit(start_addr, data, len);
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	#if(boardUSE_SFUD)
	if(flash == NULL)
		return -1;
	
	if(sfud_read(flash, start_addr, len, data) != SFUD_SUCCESS)
		return -1;
	#else
	
	#endif  //boardUSE_SFUD
    #elif(boardIC_TYPE == boardIC_ESP32C61)

    if(tpPartition->address > start_addr)
        return -1;

    if(esp_partition_read(tpPartition, (start_addr - tpPartition->address), data, len) != ESP_OK)
    {
        return -2;
    }
        
	#endif  //boardIC_TYPE
	

	
	if(uPrint.tFlag.bOperFlash)
	{
		sMyPrint("\r\n读取的数据 长度= %d ",len);
		for(int i = 0; i < len; i++)
		{
			sMyPrint("%x ",data[i]);
		}
		sMyPrint("\r\n");
	}
	
	return 1;
}

#if(FLASH_DEBUG)
//#include "systick.h"
/*****************************************************************************************************************
-----函数功能    flash测试Demo
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void sfud_qspi_test_demo(void);
void sfud_spi_test_demo(void);

void vFlash_ReadWriteTest(void)
{
    /*
    * This example uses the tpPartition table from ../tpPartitions_example.csv. For reference, its contents are as follows:
    *
    *  nvs,        data, nvs,      0x9000,  0x6000,
    *  phy_init,   data, phy,      0xf000,  0x1000,
    *  factory,    app,  factory,  0x10000, 1M,
    *  storage,    data, ,             , 0x40000,
    */
    // static const char *TAG = "example";
    // Find the tpPartition map in the tpPartition table
    static const char store_data[] = "ESP-IDF tpPartition Memory Map Example";

    // Prepare data to be read later using the mapped address
    esp_partition_erase_range(tpPartition, 0, tpPartition->size);
    esp_partition_write(tpPartition, 0, store_data, sizeof(store_data));
    sMyPrint("Written sample data to tpPartition: %s", store_data);

    // const void *map_ptr;
    // esp_partition_mmap_handle_t map_handle;

    // Map the Partition to data memory
    // ESP_ERROR_CHECK(esp_partition_mmap(tpPartition, 0, tpPartition->size, ESP_PARTITION_MMAP_DATA, &map_ptr, &map_handle));
    // sMyPrint("Mapped tpPartition to data memory address %p", map_ptr);

    // Read back the written verification data using the mapped memory pointer
    char read_data[sizeof(store_data)];
    esp_partition_read(tpPartition, 0, read_data, sizeof(read_data));
    sMyPrint("Read sample data from tpPartition using mapped memory: %s", (char*) read_data);

    assert(strcmp(store_data, read_data) == 0);
    sMyPrint("Data matches");

    // Unmap mapped memory
    // esp_partition_munmap(map_handle);
    // sMyPrint("Unmapped tpPartition from data memory");

    sMyPrint("Example end");

    sMyPrint("address%x, size%d",tpPartition->address, tpPartition->size);


	sfud_spi_test_demo();
//	sfud_qspi_test_demo();
}

void sfud_spi_test_demo(void)
{
	s8 ret = 0;
	u16 size = 512;
	uint32_t addr = tpPartition->address;
	__ALIGNED(4) u8 uc_w_buf[512] = {0};
	__ALIGNED(4) u8 uc_r_buf[512] = {0};
	
	// const sfud_flash *flash = sfud_get_device(SFUD_W25Q_DEVICE_INDEX);
	
	/* prepare write data */
    for (int i = 0; i < size; i++)
    {
		if(i >= 256)
			uc_w_buf[i] = 512 - i - 1;
		else
			uc_w_buf[i] = i;
    }
    /* erase test */
    ret = cFlash_EraseSector(addr, (addr + tpPartition->size));
    if (ret > 0)
        sMyPrint("擦除完成.地址0x%08X.大小%zu\r\n", addr, size);
    else
    {
        sMyPrint("擦除失败.\r\n");
        return;
    }
    /* write test */
    u32 write_offset = 0;
    ret = cFlash_Write8BitData(addr, &uc_w_buf[write_offset], 200); addr +=200; write_offset +=200;
	ret = cFlash_Write8BitData(addr, &uc_w_buf[write_offset], 200); addr +=200; write_offset +=200;
	ret = cFlash_Write8BitData(addr, &uc_w_buf[write_offset], 112); addr = tpPartition->address;
    if (ret > 0)
        sMyPrint("Flash写入完成.地址0x%08X.大小%zu.\r\n",  addr, size);
    else
    {
        sMyPrint("Flash写入失败.\r\n");
        return;
    }
    /* read test */
    ret = cFlash_Read8BitData(addr, uc_r_buf, size);
    if (ret > 0)
    {
        sMyPrint("Flash读取完成.地址0x%08X,大小%zu. :\r\n", addr, size);
        sMyPrint("Offset (h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");
        for (int i = 0; i < size; i++)
        {
            if (i % 16 == 0)
            {
                sMyPrint("[%08X] ", addr + i);
            }
            sMyPrint("%02X ", uc_r_buf[i]);
            if (((i + 1) % 16 == 0) || i == size - 1)
            {
                sMyPrint("\r\n");
//				vSys_MsDelay(2);
//				vTaskDelay(2);
				
            }
        }
        sMyPrint("\r\n");
    }
    else
        sMyPrint("Flash读取失败.\r\n");
    
    /* data check */
    for (int i = 0; i < size; i++)
    {
        if (uc_r_buf[i] != uc_w_buf[i])
        {
            sMyPrint("数据比较失败.\r\n");
            return;
        }
    }
    sMyPrint("数据比较成功.\r\n");
}

#ifdef SFUD_USING_QSPI
void sfud_qspi_test_demo(void)
{
	u16 size = 512;
	uint32_t addr = 0;
	u8 uc_w_buf[512] = {0};
	u8 uc_r_buf[512] = {0};
	
    sfud_err ret = SFUD_SUCCESS;
    const sfud_flash *flash = sfud_get_device(SFUD_W25Q_DEVICE_INDEX1);

    /* prepare write data */
    for (int i = 0; i < size; i++)
    {
		if(i >= 256)
			uc_w_buf[i] = 512 - i - 1;
		else
			uc_w_buf[i] = i;
    }
    /* erase test */
    ret = sfud_erase(flash, addr, size);
    if (ret == SFUD_SUCCESS)
        sMyPrint("擦除%s完成.地址0x%08X.大小%zu\r\n", flash->name, addr, size);
    else
    {
        sMyPrint("擦除%s失败.\r\n", flash->name);
        return;
    }
    /* write test */
    ret = sfud_write(flash, addr, 200, &uc_w_buf[addr]); addr +=200;
	ret = sfud_write(flash, addr, 200, &uc_w_buf[addr]); addr +=200;
	ret = sfud_write(flash, addr, 112, &uc_w_buf[addr]); addr = 0;
    if (ret == SFUD_SUCCESS)
        sMyPrint("%s  Flash写入完成.地址0x%08X.大小%zu.\r\n", flash->name, addr, size);
    else
    {
        sMyPrint("%s  Flash写入失败.\r\n", flash->name);
        return;
    }
    /* read test */
    ret = sfud_read(flash, addr, size, uc_r_buf);
    if (ret == SFUD_SUCCESS)
    {
        sMyPrint("%s  Flash读取完成.地址0x%08X,大小%zu. :\r\n", flash->name, addr, size);
        sMyPrint("Offset (h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\r\n");
        for (int i = 0; i < size; i++)
        {
            if (i % 16 == 0)
            {
                sMyPrint("[%08X] ", addr + i);
            }
            sMyPrint("%02X ", uc_r_buf[i]);
            if (((i + 1) % 16 == 0) || i == size - 1)
            {
                sMyPrint("\r\n");
				vSys_MsDelay(2);
            }
        }
        sMyPrint("\r\n");
    }
    else
        sMyPrint("%s  Flash读取失败.\r\n", flash->name);
    
    /* data check */
    for (int i = 0; i < size; i++)
    {
        if (uc_r_buf[i] != uc_w_buf[i])
        {
            sMyPrint("数据比较失败.\r\n");
            return;
        }
    }
    sMyPrint("数据比较成功.\r\n");
}
#endif  //SFUD_USING_QSPI
#endif  //FLASH_DEBUG
