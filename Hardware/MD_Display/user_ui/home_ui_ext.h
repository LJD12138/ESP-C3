/***********************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display\user_ui
 * File    : home_ui_ext.h
 * Date    : 2026-10-15
 * Author  : LJD(291483914@qq.com)
 * Desc    : Display 主页扩展控制逻辑 (Tileview切换/天气动效/信号与状态更新)
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef HOME_UI_EXT_H_
#define HOME_UI_EXT_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_data.h"

//****************************************************Macros********************************************************************//
#define			homeuiTILE_COUNT						2		/* Tile 数量 (Tile 0: 环境, Tile 1: 应用) */
#define			homeuiANIM_FRAME_MS						150		/* 天气动画每帧间隔 ms */
#define			homeuiSWIPE_ANIM_MS						250		/* 逻辑切换动画时长 ms */

//****************************************************Types*********************************************************************//

//****************************************************Globals*******************************************************************//

//****************************************************Extern********************************************************************//
void     vHomeUi_Init(void);
void     vHomeUi_Update(const DispDataSnapshot_T *p_data);
void     vHomeUi_AnimaEnter(void);
void     vHomeUi_AnimaStop(void);
void     vHomeUi_ManualTick(void);

bool     bHomeUi_SetTile(uint8_t uc_idx, bool b_anim);
uint8_t  ucHomeUi_GetTile(void);
void     vHomeUi_RequestTile(uint8_t uc_idx);
void     vHomeUi_PollRequest(void);

#endif  /* boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* HOME_UI_EXT_H_ */
