/*******************************************************************************************************************************
 * Project : ESP-C3
 * Module  : ESP-C3\Hardware\MD_Display\user_ui
 * File    : home_ui_ext.c
 * Date    : 2026-10-15
 * Author  : LJD(291483914@qq.com)
 * Desc    : Display 主页扩展控制逻辑实现 (Tileview横滑切换/天气动态逐帧动效/信号与数据刷新)
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include <stdio.h>
#include <string.h>

#include "home_ui_ext.h"

#if (boardDISPLAY_EN)
#include "lvgl.h"
#include "eez_ui/screens.h"
#include "eez_ui/images.h"
#include "eez_ui/vars.h"

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//
static volatile uint8_t s_uc_req_tile       = 0xFF;
static bool             s_b_home_anim_active = false;
static uint32_t         s_ul_last_anim_tick  = 0;
static uint8_t          s_uc_anim_frame_idx  = 0;
static Weather_E        s_e_current_weather  = WEATHER_SUNNY;

/* 晴天 8 帧动效序列 */
static const lv_img_dsc_t * const s_ap_sunny_frames[8] = {
	&img_anim_sunny_00, &img_anim_sunny_01, &img_anim_sunny_02, &img_anim_sunny_03,
	&img_anim_sunny_04, &img_anim_sunny_05, &img_anim_sunny_06, &img_anim_sunny_07
};

/* 多云 8 帧动效序列 */
static const lv_img_dsc_t * const s_ap_cloudy_frames[8] = {
	&img_anim_cloudy_00, &img_anim_cloudy_01, &img_anim_cloudy_02, &img_anim_cloudy_03,
	&img_anim_cloudy_04, &img_anim_cloudy_05, &img_anim_cloudy_06, &img_anim_cloudy_07
};

/* 阴天 8 帧动效序列 */
static const lv_img_dsc_t * const s_ap_overcast_frames[8] = {
	&img_anim_overcast_00, &img_anim_overcast_01, &img_anim_overcast_02, &img_anim_overcast_03,
	&img_anim_overcast_04, &img_anim_overcast_05, &img_anim_overcast_06, &img_anim_overcast_07
};

/* 雨天 8 帧动效序列 */
static const lv_img_dsc_t * const s_ap_rain_frames[8] = {
	&img_anim_rain_00, &img_anim_rain_01, &img_anim_rain_02, &img_anim_rain_03,
	&img_anim_rain_04, &img_anim_rain_05, &img_anim_rain_06, &img_anim_rain_07
};

/* 晴夜 8 帧动效序列 */
static const lv_img_dsc_t * const s_ap_night_sunny_frames[8] = {
	&img_anim_night_sunny_00, &img_anim_night_sunny_01, &img_anim_night_sunny_02, &img_anim_night_sunny_03,
	&img_anim_night_sunny_04, &img_anim_night_sunny_05, &img_anim_night_sunny_06, &img_anim_night_sunny_07
};

/* 多云夜 8 帧动效序列 */
static const lv_img_dsc_t * const s_ap_night_cloudy_frames[8] = {
	&img_anim_night_cloudy_00, &img_anim_night_cloudy_01, &img_anim_night_cloudy_02, &img_anim_night_cloudy_03,
	&img_anim_night_cloudy_04, &img_anim_night_cloudy_05, &img_anim_night_cloudy_06, &img_anim_night_cloudy_07
};

/* 阴夜 8 帧动效序列 */
static const lv_img_dsc_t * const s_ap_night_overcast_frames[8] = {
	&img_anim_night_overcast_00, &img_anim_night_overcast_01, &img_anim_night_overcast_02, &img_anim_night_overcast_03,
	&img_anim_night_overcast_04, &img_anim_night_overcast_05, &img_anim_night_overcast_06, &img_anim_night_overcast_07
};

/* 雨夜 8 帧动效序列 */
static const lv_img_dsc_t * const s_ap_night_rain_frames[8] = {
	&img_anim_night_rain_00, &img_anim_night_rain_01, &img_anim_night_rain_02, &img_anim_night_rain_03,
	&img_anim_night_rain_04, &img_anim_night_rain_05, &img_anim_night_rain_06, &img_anim_night_rain_07
};

//****************************************************Function Declaration******************************************************//
static void prv_vUpdateSignalBars(uint8_t uc_level);
static void prv_vUpdatePageDots(void);

/***********************************************************************************************************************
 * 函数功能    : 主页 UI 扩展组件初始化
 * 说明(备注)  : 重置状态机与帧计数
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vHomeUi_Init(void)
{
	s_uc_req_tile        = 0xFF;
	s_b_home_anim_active = false;
	s_ul_last_anim_tick  = 0;
	s_uc_anim_frame_idx  = 0;
	s_e_current_weather  = WEATHER_SUNNY;
}

/***********************************************************************************************************************
 * 函数功能    : 启动主页动态效果
 * 说明(备注)  : 进页时调用，开启天气逐帧更新
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vHomeUi_AnimaEnter(void)
{
	s_b_home_anim_active = true;
	s_ul_last_anim_tick  = lv_tick_get();
	s_uc_anim_frame_idx  = 0;
}

/***********************************************************************************************************************
 * 函数功能    : 停止主页动态效果
 * 说明(备注)  : 退页时调用，清除活动标志，停止动画计算以实现零开销
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vHomeUi_AnimaStop(void)
{
	s_b_home_anim_active = false;
}

/***********************************************************************************************************************
 * 函数功能    : 主页渲染时基驱动 (单上下文 ManualTick)
 * 说明(备注)  : 在 vDisp_UiRefresh() 渲染出口调用，推进天气逐帧动画
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vHomeUi_ManualTick(void)
{
	if (!s_b_home_anim_active || (objects.weather_anim_img == NULL))
		return;

	uint32_t ul_now = lv_tick_get();
	if ((ul_now - s_ul_last_anim_tick) >= homeuiANIM_FRAME_MS)
	{
		s_ul_last_anim_tick = ul_now;
		s_uc_anim_frame_idx = (s_uc_anim_frame_idx + 1U) % 8U;

		const lv_img_dsc_t *p_img = NULL;
		switch (s_e_current_weather)
		{
			case WEATHER_CLOUDY:
				p_img = s_ap_cloudy_frames[s_uc_anim_frame_idx];
				break;

			case WEATHER_OVERCAST:
				p_img = s_ap_overcast_frames[s_uc_anim_frame_idx];
				break;

			case WEATHER_RAIN:
				p_img = s_ap_rain_frames[s_uc_anim_frame_idx];
				break;

			case WEATHER_NIGHT_SUNNY:
				p_img = s_ap_night_sunny_frames[s_uc_anim_frame_idx];
				break;

			case WEATHER_NIGHT_CLOUDY:
				p_img = s_ap_night_cloudy_frames[s_uc_anim_frame_idx];
				break;

			case WEATHER_NIGHT_OVERCAST:
				p_img = s_ap_night_overcast_frames[s_uc_anim_frame_idx];
				break;

			case WEATHER_NIGHT_RAIN:
				p_img = s_ap_night_rain_frames[s_uc_anim_frame_idx];
				break;

			case WEATHER_SUNNY:
			default:
				p_img = s_ap_sunny_frames[s_uc_anim_frame_idx];
				break;
		}

		if (p_img != NULL)
			lv_image_set_src(objects.weather_anim_img, p_img);
	}
}

/***********************************************************************************************************************
 * 函数功能    : 更新主页所有展示数据
 * 说明(备注)  : 只读消费帧快照 p_data，格式化各字段并驱动变量桥与控件状态
 * 传入参数    : p_data: 显示数据快照指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vHomeUi_Update(const DispDataSnapshot_T *p_data)
{
	if (p_data == NULL)
		return;

	char sa_buf[32];

	/*----------------1. 室内温度更新----------------*/
	if (p_data->sTempX10 != 0x7FFF)
	{
		if (p_data->sTempX10 < 0)
			snprintf(sa_buf, sizeof(sa_buf), "-%d.%d", (-p_data->sTempX10) / 10, (-p_data->sTempX10) % 10);
		else
			snprintf(sa_buf, sizeof(sa_buf), "%d.%d", p_data->sTempX10 / 10, p_data->sTempX10 % 10);

		set_var_home_temp_val(sa_buf);

		/* 体感描述 */
		if (p_data->sTempX10 < 180)
			set_var_home_temp_desc("偏冷");
		else if (p_data->sTempX10 <= 260)
			set_var_home_temp_desc("舒适");
		else if (p_data->sTempX10 < 300)
			set_var_home_temp_desc("偏热");
		else
			set_var_home_temp_desc("炎热");
	}
	else
	{
		set_var_home_temp_val("--.-");
		set_var_home_temp_desc("--");
	}

	/*----------------2. 室内湿度更新----------------*/
	if (p_data->ucHumidity <= 100)
	{
		snprintf(sa_buf, sizeof(sa_buf), "%u", p_data->ucHumidity);
		set_var_home_humi_val(sa_buf);

		if (p_data->ucHumidity < 40)
			set_var_home_humi_desc("偏干");
		else if (p_data->ucHumidity <= 70)
			set_var_home_humi_desc("舒适");
		else
			set_var_home_humi_desc("潮湿");
	}
	else
	{
		set_var_home_humi_val("--");
		set_var_home_humi_desc("--");
	}

	/*----------------3. 导航时间与网络时钟----------------*/
	static const char * const s_sa_wday_str[7] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};

	if (p_data->bNetSynced && (p_data->ucLocalHour != 0xFF))
	{
		snprintf(sa_buf, sizeof(sa_buf), "%02u:%02u", p_data->ucLocalHour, p_data->ucLocalMin);
		set_var_home_nav_time(sa_buf);

		snprintf(sa_buf, sizeof(sa_buf), "%02u:%02u:%02u", p_data->ucLocalHour, p_data->ucLocalMin, p_data->ucLocalSec);
		set_var_home_net_time(sa_buf);

		set_var_home_net_sync("已同步");

		if (objects.net_sync_dot != NULL)
			lv_obj_set_style_bg_color(objects.net_sync_dot, lv_color_hex(0x4ade80), LV_PART_MAIN | LV_STATE_DEFAULT);

		if ((objects.obj0__nav_date != NULL) && (p_data->ucLocalMon > 0))
		{
			snprintf(sa_buf, sizeof(sa_buf), "%02u/%02u %s", p_data->ucLocalMon, p_data->ucLocalDay, s_sa_wday_str[p_data->ucLocalWday % 7]);
			lv_label_set_text(objects.obj0__nav_date, sa_buf);
		}
	}
	else
	{
		set_var_home_nav_time("--:--");
		set_var_home_net_time("--:--:--");
		set_var_home_net_sync("未同步");

		if (objects.net_sync_dot != NULL)
			lv_obj_set_style_bg_color(objects.net_sync_dot, lv_color_hex(0x5a6068), LV_PART_MAIN | LV_STATE_DEFAULT);

		if (objects.obj0__nav_date != NULL)
			lv_label_set_text(objects.obj0__nav_date, "--/-- --");
	}

	/*----------------4. 天气信息与动画模式更新----------------*/
	if (p_data->bWeatherValid)
	{
		if (p_data->saWeatherDesc[0] != '\0')
		{
			if (p_data->sWeatherTempX10 != 0x7FFF)
				snprintf(sa_buf, sizeof(sa_buf), "%s %d°C", p_data->saWeatherDesc, p_data->sWeatherTempX10 / 10);
			else
				snprintf(sa_buf, sizeof(sa_buf), "%s", p_data->saWeatherDesc);
		}
		else
		{
			snprintf(sa_buf, sizeof(sa_buf), "%d°C", p_data->sWeatherTempX10 / 10);
		}

		set_var_home_weather_text(sa_buf);
		s_e_current_weather = (Weather_E)p_data->eWeather;
		if (s_e_current_weather >= WEATHER_MAX)
			s_e_current_weather = WEATHER_SUNNY;
	}
	else
	{
		set_var_home_weather_text("--");
		s_e_current_weather = WEATHER_SUNNY;
	}

	/*----------------5. 信号格数显隐更新----------------*/
	prv_vUpdateSignalBars(p_data->ucSignalLevel);

	/*----------------6. 页面指示圆点更新----------------*/
	prv_vUpdatePageDots();
}

/***********************************************************************************************************************
 * 函数功能    : 更新 WiFi 5 格信号柱条显示
 * 传入参数    : uc_level: 信号格数 (0~5)
 * 返回值      : 无
 ************************************************************************************************************************/
static void prv_vUpdateSignalBars(uint8_t uc_level)
{
	static uint8_t s_uc_cached_level = 0xFF;
	if (s_uc_cached_level == uc_level)
		return;

	s_uc_cached_level = uc_level;

	lv_obj_t * const ap_bars[5] = {
		objects.obj0__wifi_icom__sig_bar_0,
		objects.obj0__wifi_icom__sig_bar_1,
		objects.obj0__wifi_icom__sig_bar_2,
		objects.obj0__wifi_icom__sig_bar_3,
		objects.obj0__wifi_icom__sig_bar_4
	};

	for (uint8_t i = 0; i < 5; i++)
	{
		if (ap_bars[i] != NULL)
		{
			lv_color_t color = (i < uc_level) ? lv_color_hex(0xffffff) : lv_color_hex(0x4b5563);
			lv_obj_set_style_bg_color(ap_bars[i], color, LV_PART_MAIN | LV_STATE_DEFAULT);
		}
	}
}

/***********************************************************************************************************************
 * 函数功能    : 更新底部页面指示圆点
 * 传入参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void prv_vUpdatePageDots(void)
{
	static uint8_t s_uc_cached_tile = 0xFF;
	uint8_t cur_tile = ucHomeUi_GetTile();

	if (s_uc_cached_tile == cur_tile)
		return;

	s_uc_cached_tile = cur_tile;

	if (cur_tile == 0)
	{
		if (objects.obj1__page_dot_0 != NULL)
		{
			lv_obj_set_size(objects.obj1__page_dot_0, 12, 6);
			lv_obj_set_style_bg_color(objects.obj1__page_dot_0, lv_color_hex(0xe3e6ea), LV_PART_MAIN | LV_STATE_DEFAULT);
		}
		if (objects.obj1__page_dot_1 != NULL)
		{
			lv_obj_set_size(objects.obj1__page_dot_1, 6, 6);
			lv_obj_set_style_bg_color(objects.obj1__page_dot_1, lv_color_hex(0x5a6068), LV_PART_MAIN | LV_STATE_DEFAULT);
		}
	}
	else
	{
		if (objects.obj1__page_dot_0 != NULL)
		{
			lv_obj_set_size(objects.obj1__page_dot_0, 6, 6);
			lv_obj_set_style_bg_color(objects.obj1__page_dot_0, lv_color_hex(0x5a6068), LV_PART_MAIN | LV_STATE_DEFAULT);
		}
		if (objects.obj1__page_dot_1 != NULL)
		{
			lv_obj_set_size(objects.obj1__page_dot_1, 12, 6);
			lv_obj_set_style_bg_color(objects.obj1__page_dot_1, lv_color_hex(0xe3e6ea), LV_PART_MAIN | LV_STATE_DEFAULT);
		}
	}
}

/***********************************************************************************************************************
 * 函数功能    : 切换当前工作页 Tile
 * 传入参数    : uc_idx: 目标 Tile 索引 (0: 环境卡片, 1: 应用网格), b_anim: 是否启用平滑动效
 * 返回值      : bool: true 成功触发切换, false 失败 (正在滚动或参数非法)
 ************************************************************************************************************************/
bool bHomeUi_SetTile(uint8_t uc_idx, bool b_anim)
{
	if ((objects.home_tileview == NULL) || (uc_idx >= homeuiTILE_COUNT))
		return false;

	if (lv_obj_is_scrolling(objects.home_tileview))
		return false;

	lv_obj_scroll_to_x(objects.home_tileview, (int32_t)uc_idx * 320, b_anim ? LV_ANIM_ON : LV_ANIM_OFF);
	prv_vUpdatePageDots();
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 获取当前活动 Tile 索引
 * 传入参数    : 无
 * 返回值      : uint8_t: 当前 Tile 索引 (0 或 1)
 ************************************************************************************************************************/
uint8_t ucHomeUi_GetTile(void)
{
	if (objects.home_tileview == NULL)
		return 0;

	int32_t scroll_x = lv_obj_get_scroll_x(objects.home_tileview);
	return (scroll_x >= 160) ? 1 : 0;
}

/***********************************************************************************************************************
 * 函数功能    : 跨任务请求切换 Tile (线程安全)
 * 传入参数    : uc_idx: 目标 Tile 索引
 * 返回值      : 无
 ************************************************************************************************************************/
void vHomeUi_RequestTile(uint8_t uc_idx)
{
	s_uc_req_tile = uc_idx;
}

/***********************************************************************************************************************
 * 函数功能    : 轮询跨任务切页请求
 * 说明(备注)  : 仅在显示任务帧更新入口调用
 * 传入参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vHomeUi_PollRequest(void)
{
	uint8_t req = s_uc_req_tile;
	if (req != 0xFF)
	{
		s_uc_req_tile = 0xFF;
		if (req < homeuiTILE_COUNT)
			bHomeUi_SetTile(req, true);
	}
}

#endif  /* boardDISPLAY_EN */
