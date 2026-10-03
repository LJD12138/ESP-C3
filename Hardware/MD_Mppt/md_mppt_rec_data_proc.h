#ifndef MD_MPPT_REC_DATA_PROC_H
#define MD_MPPT_REC_DATA_PROC_H

#include "board_config.h"

#if(boardMPPT_IFACE)
#include "main.h"
#include "Baiku/baiku_proto.h"

s8 c_mppt_rec_proc_data(BaikuProtoRx_t* proto);

#endif  //boardMPPT_IFACE

#endif  //MD_MPPT_REC_DATA_PROC_H
