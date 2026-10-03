#ifndef MD_BMS_PROT_FRAME_H_
#define MD_BMS_PROT_FRAME_H_

#include "board_config.h"

#if(boardBMS_IFACE)
#include "main.h"
#include "Baiku/baiku_proto.h"

extern			BaikuProtoRx_t 								*tpBmsProtoRx;

s8 c_bms_cs_get_param(u8 num);
s8 c_bms_cs_switch(TaskInParam_U u_in_param);
s8 c_bms_cs_set_cali(u8 num);
s8 c_bms_cs_get_app_info(u16 num);
s8 c_bms_cs_req_updata(u8 num);
s8 c_bms_cs_req_chg(void);

bool bBms_SendProtInit(void);
bool bBms_RecProtInit(void);

#endif  //boardBMS_IFACE

#endif  //MD_BMS_PROT_FRAME_H_
