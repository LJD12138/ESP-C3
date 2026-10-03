#ifndef MD_BMS_REC_DATA_PROC_H
#define MD_BMS_REC_DATA_PROC_H

#include "board_config.h"

#if(boardBMS_IFACE)
#include "main.h"
#include "Baiku/baiku_proto.h"

s8 c_bms_rec_proc_data(BaikuProtoRx_t* proto);

#endif  //boardBMS_IFACE

#endif  //MD_BMS_REC_DATA_PROC_H
