#ifndef FLASH_ESP32_H_
#define FLASH_ESP32_H_

#include "main.h"
 
bool bFlash_Esp32EraseSector(uint32_t StarAddr,uint32_t EndAddr);
bool bFlash_Esp32Write8Bit(uint32_t WriteAddr,const uint8_t *wData,uint32_t wNum);
void vFlash_Esp32Read8Bit(uint32_t ReadAddr,uint8_t *rData,uint32_t rNum);
#endif
