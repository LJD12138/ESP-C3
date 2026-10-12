/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\Key
 * File    : key_func.c
 * Date    : 2026-10-10
 * Author  : LJD(291483914@qq.com)
 * Desc    : 按键功能业务动作分发处理实现文件 (表驱动优化方案)
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Key/key_func.h"

#if (boardKEY_EN)
#include "Key/key_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "function.h"

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  /* boardDISPLAY_EN */

#if (boardENG_MODE_EN)
#include "Key/key_func_eng.h"
#endif  /* boardENG_MODE_EN */

//****************************************************Macros********************************************************************//
typedef void (*fnKeyAction_T)(void);

/* 适用系统状态掩码 */
#define			KEY_MASK_ANY							0xFFFFFFFF
#define			KEY_MASK_WORK							(1U << DS_WORK)
#define			KEY_MASK_ERR							(1U << DS_ERR)
#define			KEY_MASK_NORMAL							(KEY_MASK_WORK | KEY_MASK_ERR)

//****************************************************Parameter Initialization**************************************************//
/* 动作映射表项 */
typedef struct
{
	const uint8_t		*pSeq;				/* 触发序列特征数组指针 */
	uint8_t				ucSeqLen;			/* 序列长度 (字节数) */
	uint32_t			ulStateMask;		/* 适用的工作状态掩码 */
	fnKeyAction_T		pfAction;			/* 业务处理函数指针 */
	const char			*pLog;				/* 调试打印日志 */
}KeyActionItem_T;

/* 编译期保证 (1U << eDevState) 移位安全 */
typedef char __ds_shift_safe[(DS_WORK < 32 && DS_ERR < 32) ? 1 : -1];

/* 长按 开关机 */
static const u8 s_uc_key_tri_sys_on_off_buff[2] = { KTE_POWER_LONG, KTE_FUN_NULL };
/* 连击 开关机/系统充放保护 */
static const u8 s_uc_key_tri_sys_prote_on_off_buff[4] = { KTE_POWER_LONG, KTE_POWER_LONG, KTE_POWER_LONG, KTE_FUN_NULL };

#if (boardDISPLAY_EN)
/* 单击 切换屏幕 */
static const u8 s_uc_key_tri_screen_buff[2] = { KTE_POWER_SHORT, KTE_FUN_NULL };
#endif  /* boardDISPLAY_EN */

//****************************************************Function Declaration******************************************************//
static void v_act_sys_on_off(void);
static void v_act_sys_prote(void);

#if (boardDISPLAY_EN)
static void v_act_screen_switch(void);
#endif  /* boardDISPLAY_EN */

static bool b_key_dispatch(const KeyActionItem_T *p_table, uint8_t uc_num, u8 *p_buff);


/* 跨状态动作表: 判序最高, 先于工程模式判断 */
static const KeyActionItem_T s_t_key_action_global[] =
{
	/* 序列特征                                   长度                                       有效系统状态       回调处理函数               日志 */
	{s_uc_key_tri_sys_on_off_buff,              sizeof(s_uc_key_tri_sys_on_off_buff),      KEY_MASK_ANY,    v_act_sys_on_off,         "开关机"},
};
#define			KEY_ACTION_GLOBAL_NUM					(sizeof(s_t_key_action_global) / sizeof(s_t_key_action_global[0]))

/* 工作态动作表: 仅在 DS_WORK / DS_ERR 生效 */
static const KeyActionItem_T s_t_key_action_work[] =
{
	{s_uc_key_tri_sys_prote_on_off_buff,         sizeof(s_uc_key_tri_sys_prote_on_off_buff),  KEY_MASK_NORMAL, v_act_sys_prote,          "系统充放保护"},

	#if (boardDISPLAY_EN)
	{s_uc_key_tri_screen_buff,                   sizeof(s_uc_key_tri_screen_buff),            KEY_MASK_NORMAL, v_act_screen_switch,      "切换屏幕"},
	#endif  /* boardDISPLAY_EN */
};

#define KEY_ACTION_WORK_NUM (sizeof(s_t_key_action_work) / sizeof(s_t_key_action_work[0]))


/***********************************************************************************************************************
 * 函数功能    : 按键功能处理函数
 * 说明(备注)  : 查表分发按键事件序列
 * 传入参数    : p_uc_key_tri_type_buff: 事件序列缓冲区指针
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void vKey_ProcKeyFunc(u8 *p_uc_key_tri_type_buff)
{
	/* 第一优先级: 跨状态动作 (原逻辑中"开关机"优先于工程模式判断, 保持一致) */
	if (b_key_dispatch(s_t_key_action_global, KEY_ACTION_GLOBAL_NUM, p_uc_key_tri_type_buff) == true)
	{
		vKey_ParamInit();
		return;
	}

	#if (boardENG_MODE_EN)
	/* 第二优先级: 工程模式处理 */
	if (tSysInfo.eDevState == DS_ENG_MODE)
	{
		v_key_func_eng(p_uc_key_tri_type_buff);
		vKey_ParamInit();
		return;
	}
	#endif  /* boardENG_MODE_EN */

	/* 第三优先级: 工作态/错误态动作 */
	b_key_dispatch(s_t_key_action_work, KEY_ACTION_WORK_NUM, p_uc_key_tri_type_buff);

	vKey_ParamInit();
}

/***********************************************************************************************************************
 * 函数功能    : 系统开关机动作
 * 说明(备注)  : 触发系统开关机状态切换
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
static void v_act_sys_on_off(void)
{
	cSys_Switch(SO_KEY, ST_NULL, false);
}

/***********************************************************************************************************************
 * 函数功能    : 系统充放保护动作
 * 说明(备注)  : 触发强制关闭保护标志，长鸣并关机
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
static void v_act_sys_prote(void)
{
	bSys_SetPerm(SPO_FORCE_CLOSE, true);
	cSys_Switch(SO_KEY, ST_OFF, false);
}

#if (boardDISPLAY_EN)
/***********************************************************************************************************************
 * 函数功能    : 切换屏幕动作
 * 说明(备注)  : 切换主显示屏页面
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_act_screen_switch(void)
{
	// bDisp_ChangeScreen(ID_MAIN_0);
}
#endif  /* boardDISPLAY_EN */

/***********************************************************************************************************************
 * 函数功能    : 按键动作查表分发内核
 * 说明(备注)  : 遍历动作表比对当前系统状态掩码与事件序列特征
 * 传入参数    : p_table: 动作表指针, uc_num: 表项数量, p_buff: 事件序列缓冲区指针
 * 输出参数    : 无
 * 返回值      : bool: true 命中并执行动作, false 未命中
 ***********************************************************************************************************************/
static bool b_key_dispatch(const KeyActionItem_T *p_table, uint8_t uc_num, u8 *p_buff)
{
	uint8_t i;
	uint32_t ul_curr_state_mask = (1U << tSysInfo.eDevState);

	for (i = 0; i < uc_num; i++)
	{
		/* 1. 校验当前系统工作状态是否匹配该按键动作 */
		if ((p_table[i].ulStateMask & ul_curr_state_mask) != 0)
		{
			/* 2. 比对按键序列特征 */
			if (bFun_DataCompare(p_buff, (u8 *)p_table[i].pSeq, p_table[i].ucSeqLen))
			{
				if (p_table[i].pfAction != NULL)
					p_table[i].pfAction();

				if (uPrint.tFlag.bKeyTask && p_table[i].pLog != NULL)
					sMyPrint("Key_Task:%s\r\n", p_table[i].pLog);

				return true;
			}
		}
	}
	return false;
}

#endif  /* boardKEY_EN */

