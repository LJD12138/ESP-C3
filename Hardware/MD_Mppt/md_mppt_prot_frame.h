#ifndef MD_MPPT_PROT_FRAME_H_
#define MD_MPPT_PROT_FRAME_H_

#include "board_config.h"

#if(boardMPPT_IFACE)
#include "main.h"
#include "BaiKu/baiku_proto.h"


extern			BaikuProtoTx_t 						*tpMpptProtoTx;
extern 			BaikuProtoRx_t						*tpMpptProtoRx;

s8 c_dcac_cs_switch(TaskInParam_U u_in_param);
s8 c_mppt_cs_get_param(u8 num);
s8 c_mppt_cs_set_pwr(u16 pwr);

bool bMppt_SendProtInit(void);
bool bMppt_RecProtInit(void);

#endif  //boardMPPT_IFACE

#endif  //MD_MPPT_PROT_FRAME_H_
