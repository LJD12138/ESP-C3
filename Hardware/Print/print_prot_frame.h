#ifndef MD_PRINT_PROT_FRAME_H_
#define MD_PRINT_PROT_FRAME_H_

#include "board_config.h"

#if(boardPRINT_IFACE)
#include "main.h"
#include "Baiku/baiku_proto.h"

extern 			vu8 										uc_next_cmd;
extern 			BaikuProtoRx_t 								*tpPrintProtoRx;
extern 			BaikuProtoTx_t 								*tpPrintProtoTx;

bool b_cycle_relay_data(void);
bool b_relay02_switch_result(uint8_t* data);
bool b_relay08_param(void);
bool b_relay0A_bat_param(void);
bool b_relay0C_dcac_param(void);
bool b_relay0E_mppt_param(void);
bool b_relay10_usb_param(void);
bool b_relay12_dc_param(void);
bool b_relay14_sysinfo_param(void);
bool b_relay40_set_chg_pwr(BaikuProtoRx_t* proto);
bool b_relay44_cali(BaikuProtoRx_t* proto);
bool b_relay45_cali(u16 temp);
bool b_relay80_get_mem_param(BaikuProtoRx_t* proto);
bool b_relay82_write_mem_info(BaikuProtoRx_t* proto);
bool b_relay84_set_print_state(BaikuProtoRx_t* proto);
bool b_relay86_get_print_state(BaikuProtoRx_t* proto);
bool b_relay88_sys_set(BaikuProtoRx_t* proto);
bool b_relay_bms_app_info(u16 len);

bool bPrint_SendProtInit(void);
bool bPrint_RecProtInit(void);

#endif  //boardPRINT_IFACE

#endif  //MD_PRINT_PROT_FRAME_H_
