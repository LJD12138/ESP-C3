#ifndef MD_DCAC_REC_DATA_PROC_H
#define MD_DCAC_REC_DATA_PROC_H

#include "board_config.h"

#if(boardDCAC_IFACE)
#include "main.h"
#include "Baiku/baiku_proto.h"

s8 c_dcac_rec_proc_data(BaikuProtoRx_t* proto);

#endif  //boardDCAC_IFACE

#endif  //MD_DCAC_REC_DATA_PROC_H
