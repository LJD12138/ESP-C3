/***********************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display
 * File    : md_display_api.h
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示对外 API 头文件：分辨率宏、初始化/页面加载/UI 刷新及状态接口
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_DISPLAY_API_H
#define MD_DISPLAY_API_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "board_config.h"
#include "MD_Display/md_display_iface.h"

#if (boardDISPLAY_EN)

//****************************************************Macros********************************************************************//
#define			DISP_HOR_RES							dispTFT_WIDTH
#define			DISP_VER_RES							dispTFT_HEIGHT

//****************************************************Types*********************************************************************//

//****************************************************Globals*******************************************************************//

//****************************************************Extern********************************************************************//
struct   DispDataSnapshot_T;

void     vDisp_Init(void);
void     vDisp_LoadScreen(int screen_id);
void     vDisp_ReqUiRefresh(void);
void     vDisp_UiRefresh(void);
void     vDisp_UiPump(void);
bool     bDisp_IsReady(void);
void     vDisp_UpdateDevParam(const struct DispDataSnapshot_T *p_snapshot);
void     vDisp_ClearScreen(uint16_t us_color);

#endif  /* boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_DISPLAY_API_H */
