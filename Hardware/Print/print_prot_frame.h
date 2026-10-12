/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_prot_frame.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : Print通信协议帧组包与解析接口声明
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef __PRINT_PROT_FRAME_H
#define __PRINT_PROT_FRAME_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardPRINT_IFACE)
#include "Baiku/baiku_proto.h"

//****************************************************Macros********************************************************************//

//****************************************************Types*********************************************************************//

//****************************************************Globals*******************************************************************//
extern vu8              uc_next_cmd;
extern BaikuProtoRx_t   *tpPrintProtoRx;
extern BaikuProtoTx_t   *tpPrintProtoTx;

//****************************************************Extern********************************************************************//
s8 c_cycle_relay_data(void);
s8 c_relay02_switch_result(uint8_t* data);
s8 c_relay08_param(void);

bool bPrint_SendProtInit(void);
bool bPrint_RecProtInit(void);

#endif  /* boardPRINT_IFACE */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __PRINT_PROT_FRAME_H */
