#ifndef MD_DCAC_PROT_FRAME_H_
#define MD_DCAC_PROT_FRAME_H_

#include "board_config.h"

#if(boardDCAC_IFACE)
#include "main.h"
#include "BaiKu/baiku_proto.h"

#if(boardUSE_OS)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#endif  //boardUSE_OS

extern			BaikuProtoTx_t 						    *tpDcacProtoTx;
extern			BaikuProtoRx_t 						    *tpDcacProtoRx;
extern			SemaphoreHandle_t 						dcacSemaphoreMutex;

s8 c_dcac_cs_switch(TaskInParam_U u_in_param);
s8 c_dcac_cs_get_param(u8 num);
bool b_dcac_cs_set_total_chg_pwr(u16 pwr);
bool b_dcac_cs_init(void);
bool b_dcac_cs_set_para_in_pwr(u16 pwr);

bool bDcac_SendProtInit(void);
bool bDcac_RecProtInit(void);

#endif  //boardDCAC_IFACE

#endif  //MD_DCAC_PROT_FRAME_H_
