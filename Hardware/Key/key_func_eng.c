/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\Key
 * File    : key_func_eng.c
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : 工程模式按键处理实现文件 (表驱动优化)
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "key_func_eng.h"

#if (boardENG_MODE_EN)
#include "Key/key_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Sys/sys_queue_task_eng.h"
#include "function.h"

#if (boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  /* boardBUZ_EN */

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#include "MD_Display/user_ui/eng_mode_ui.h"
#endif  /* boardDISPLAY_EN */

//****************************************************Macros********************************************************************//
typedef void (*fnEngAction_T)(void);

typedef struct
{
	const uint8_t		*pPattern;			/* 匹配序列指针 */
	uint8_t				ucLen;				/* 序列长度 */
	fnEngAction_T		pfAction;			/* 对应动作回调 */
	const char			*pLog;				/* 打印日志 */
}EngKeyActionItem_T;

//****************************************************Parameter Initialization**************************************************//
static const EngKeyActionItem_T s_t_eng_action_tbl[] =
{
	/* 当工程模式启用时在此扩展对应按键匹配项 */
	{ NULL, 0, NULL, NULL },
};

#define			ENG_KEY_ACTION_NUM						(sizeof(s_t_eng_action_tbl) / sizeof(s_t_eng_action_tbl[0]))

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 工程模式按键分发处理函数
 * 说明(备注)  : 表驱动匹配序列，直接路由至 LVGL UI 界面并提示蜂鸣
 * 传入参数    : p_uc_key_tri_type_buff: 事件序列缓冲区指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_key_func_eng(uint8_t *p_uc_key_tri_type_buff)
{
	for (uint8_t i = 0; i < ENG_KEY_ACTION_NUM; i++)
	{
		if (s_t_eng_action_tbl[i].pPattern == NULL)
			continue;

		if (bFun_DataCompare(p_uc_key_tri_type_buff, (u8 *)s_t_eng_action_tbl[i].pPattern, s_t_eng_action_tbl[i].ucLen))
		{
			if (s_t_eng_action_tbl[i].pfAction != NULL)
				s_t_eng_action_tbl[i].pfAction();

			#if (boardBUZ_EN)
			bBuz_Tweet(SHORT_1);
			#endif  /* boardBUZ_EN */

			if (uPrint.tFlag.bKeyTask && s_t_eng_action_tbl[i].pLog != NULL)
				sMyPrint(s_t_eng_action_tbl[i].pLog);

			break;
		}
	}
}

#endif  /* boardENG_MODE_EN */
