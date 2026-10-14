# ESP-C3 主页 UI 设计方案（320×240 横屏 · EEZ Studio 生成为主）

| 项 | 值 |
|---|---|
| 文档类型 | UI 设计规格书（参数级，供 AI 直接实现） |
| 目标硬件 | ST7789 320×240 SPI 屏（`dispTFT_WIDTH/HEIGHT`）+ FT6336U 电容触摸（`lv_port_indev.c` 已具备） |
| UI 框架 | LVGL 9.4（`managed_components/lvgl__lvgl`） |
| 调度架构 | UniDisplay（`Middlewares/UniDisplay` + `Hardware/MD_Display`） |
| 页面载体 | **复用既有 `PAGE_ID_WORK`（`md_display_page_work.c`）**，**不新增页面** |
| **交付形态** | **EEZ Studio 工程内重新设计既有 Screen `main_work` → Build 生成 `eez_ui/`（SOP-A 主路径）**；`vars.c` 手写数据桥；`user_ui/` 仅承担 EEZ Flow 不可实现项（tileview 逻辑切换、信号格/天气图标显隐，SOP-B 辅助路径） |
| EEZ 工程 | `Hardware/MD_Display/eez_esp_c3_project/esp_c3.eez-project`，**改造既有 Screen `main_work`** |
| 设计日期 | 2026-10-14 |

> **为什么复用 WORK 页而非新增页面**：`G_tPageWork` 已在 `b_task_param_init()` 注册、`DS_WORK` 已路由至 `PAGE_ID_WORK`、`ui_init()` 默认加载 `SCREEN_ID_MAIN_WORK`、故障页（`md_display_page_fault.c`）以 `main_work` 为底屏——WORK 页本就是工程的主页。新增 `PAGE_ID_HOME` 会造成双主页语义冗余、留下一页死代码，且需改 `uni_disp_page.h` 枚举 + 注册 + 路由共 3 处。复用方案这些全部为零改动，只改 EEZ 工程内的 Screen 内容与页面回调内部逻辑。

---

## 1. 总体规格

### 1.1 基本参数

| 参数 | 值 | 说明 |
|---|---|---|
| 分辨率 | 320(H) × 240(V) 横屏 | RGB565 |
| 页面载体 | EEZ Screen `main_work`（**重新设计**，仍生成 `SCREEN_ID_MAIN_WORK` 与 `objects.*`） | 单屏承载双 tile |
| 页面结构 | 固定导航栏 + `Tileview`（`home_tileview`）双 Tile | 主页1（tile 0）/ 主页2（tile 1） |
| 交互触发 | A. 手势滑动（tileview 原生）　B. 逻辑切换（user_ui API / 按键事件） | 两种方式统一 250ms 平滑过渡 |
| 过渡动画 | 250ms，ease-out 曲线 | 常量 `homeuiSWIPE_ANIM_MS` |
| 页面帧周期 | `usRefreshMs = 33` | 维持 `G_tPageWork` 现值；滚动/触摸渲染由显示任务 `dispINDEV_POLL_PERIOD_MS=16` 快泵保证流畅 |
| 数据模式 | 帧快照消费（`DispDataSnapshot_T`）+ EEZ Native 变量数据桥 | 渲染链只读 `p_data`，与数据更新解耦 |

### 1.2 布局总骨架（所有页面统一）

```
(0,0)   ┌──────────────────────────────────────┐ (320,0)
        │ 12:34                        ▂▄▆█ │ ← 导航信息栏 y=0..31（高32px）
(0,31)  ├──────────────────────────────────────┤ ← 1px 分隔线（边界清晰）
        │                                      │
        │      ┌──────── tileview 内容区 ────────┐│
        │      │  y=32..239（高208px，严格裁剪）  ││
        │      └──────────────────────────────┘│
        │       主页1 ⇄ 主页2（水平滑动切换）      │
(0,240) └──────────────────────────────────────┘
```

- 上栏：导航信息栏，固定 32px（= 240 / 7.5），直接挂在 Screen 根，所有 Tile 共享，不随滑动移动；
- 下栏：主内容区 208px，为 `home_tileview` 窗口，两 Tile 横向排列；
- 分栏边界：y=31 处 1px 分隔线，导航栏与内容区**无重叠**（tileview 严格裁剪于 y≥32）。

### 1.3 职责分工（EEZ 为主 / user_ui 为辅）

| 功能 | 归属 | 依据 |
|---|---|---|
| 全部布局、控件、样式、字体、图片、静态文本 | **EEZ Studio 工程** | SOP-A 主路径 |
| 动态文本（导航时间/温湿度/网络时间/天气文字） | **EEZ Native 变量绑定 + 手写 `vars.c`** | §9 数据桥：`set_var_xxx` → `ui_tick()` 自动比对刷新 |
| tileview 手势滑动 | EEZ Tileview 控件原生 | 无需代码 |
| tileview 逻辑切换、防抖、跨任务请求 | **user_ui**（操作 `objects.home_tileview`） | LVGL API 调用 |
| 信号格 5 档显隐 | **user_ui**（操作 `objects.sig_bar_i`） | 可见性切换，EEZ Flow 不做 |
| 天气图标四态显隐 | **user_ui**（操作 `objects.weather_icon_i`） | 可见性切换 |
| 应用格子点击（预留，当前无动作） | **user_ui**（`LV_EVENT_CLICKED` 绑定） | LVGL 事件 |
| 页面契约/注册/路由 | **复用 `G_tPageWork`，零注册零路由改动** | 已注册（`b_task_param_init`）已路由（`DS_WORK`） |

### 1.4 交互与动画规格

| 触发方式 | 实现路径 | 说明 |
|---|---|---|
| 手势滑动 | tileview 原生横向滚动 + snap 吸附 | FT6336 触摸，松手后惯性吸附 |
| 逻辑切换（显示任务内） | `bHomeUi_SetTile(idx, anim)` | 内部 `lv_tileview_set_tile_by_index(objects.home_tileview, ...)` |
| 逻辑切换（其它任务） | `vHomeUi_RequestTile(idx)` → 显示任务帧内轮询 | 原子单字节请求寄存器，**不新增 uni_disp_event.h 事件类型** |
| 按键切换 | `G_tPageWork.bOnEvent` 消费 `DISP_EVT_KEY_SHORT` | 短按在 tile0/tile1 间循环切换，返回 true（现透传，改后本页消费） |

- 防抖：滚动进行中（`lv_obj_is_scrolling(objects.home_tileview)` 为 true）拒绝新逻辑请求并返回 false；
- 越界：`idx >= homeuiTILE_COUNT` 返回 false；
- 动画时长：逻辑切换带 `LV_ANIM_ON`；若 LVGL 默认滚动动画与 250ms 目标不符，在 user_ui 层改用 `lv_obj_scroll_to()` + 自建 `lv_anim` 精确控制。

---

## 2. 全局设计参数

### 2.1 颜色表（在 EEZ Studio 样式中填写；建议建共享样式便于统一调色）

| 语义 | 值 | 用途 |
|---|---|---|
| 根背景 | `#1B1E23` | Screen `main_work` 背景 |
| 内容页背景 | `#20242B` | 两个 Tile 背景 |
| 导航栏背景 | `#333333`，不透明度 70% | 深灰半透明 |
| 导航分隔线 | `#414750` | y=31 处 1px |
| 主文字/导航文字/信号点亮 | `#FFFFFF` | 时间、信号点亮格 |
| 次要文字 | `#9CA3AF` | 网络时间 |
| 温度数值 | `#FFB74D` | 暖橙 |
| 湿度数值 | `#4FC3F7` | 浅蓝 |
| 天气文字/图标 | `#E3E6EA` | |
| 主页1 分隔线 | `#3A414C` | |
| 应用格子背景 / 边框 | `#262B33` / `#3A414C`（1px，圆角 6） | |
| 图标占位区背景 | `#333B47`（圆角 6） | |
| 应用名称文字 | `#C9CFD8` | |
| 信号熄灭格 | `#5A6068` | |

> 性能备注：RGB565 + 30 行局部缓冲下半透明导航栏渲染开销可忽略；若实测帧率不足，降级为不透明 `#2B2E33`（在 EEZ 样式中改一处）。

### 2.2 字体表（EEZ Studio 字体编辑器创建，子集化，随 Build 生成 `eez_ui/ui_font_home_*.c`）

| 字体名（EEZ） | 字号 | 字符集 | 用途 |
|---|---|---|---|
| `home_num_40` | 40px | ASCII 0x20~0x7E + `°`(U+00B0) | 温度值 |
| `home_num_28` | 28px | ASCII | 湿度值 |
| `home_cn_16` | 16px | ASCII + `晴多云阴雨` | 导航时间、天气文字 |
| `home_cn_12` | 12px | ASCII + `设置时钟天气网络关于主题室内温度湿度体感舒适偏热冷炎润潮偏干已未同步` | 应用名称、卡片标题、体感文案、网络时间、同步状态 |

### 2.3 图片资源（放 `eez_project/Image/`，Build 生成至 `eez_ui/images.c`）

| 文件名 | 尺寸 | 风格 | 用途 |
|---|---|---|---|
| `ic_temp.png` | 20×20 | 白色单色线性，温度计 | 温度卡标题图标 |
| `ic_humi.png` | 20×20 | 白色单色线性，水滴 | 湿度卡标题图标 |
| `weather_sunny_core.png` | 48×48 | 白色单色，太阳本体（圆心+内芒） | 晴天静态层 |
| `weather_sunny_rays.png` | 48×48 | 白色单色，外圈光芒环（user_ui 旋转层） | 晴天动画层 |
| `weather_cloudy.png` | 48×48 | 白色单色，云+局部日光 | 多云 |
| `weather_overcast.png` | 48×48 | 白色单色，实心云 | 阴 |
| `weather_rain.png` | 48×48 | 白色单色，云+雨丝 | 雨（user_ui 浮动层） |

> 图片格式建议：EEZ 导出选**带 alpha 的索引色（I8+A8 或 A8 抠底）**，省 flash 且软渲染快；全部为小尺寸单色图，无 RAM 压力。

### 2.4 通用常量（`user_ui/home_ui_ext.h`）

| 宏 | 值 | 说明 |
|---|---|---|
| `homeuiTILE_COUNT` | 2 | tile 数量（扩展加页时递增） |
| `homeuiAPP_COUNT` | 6 | 应用格子数量 |
| `homeuiSWIPE_ANIM_MS` | 250 | 逻辑切换动画时长 |
| `homeuiANIM_ENTER_MS` | 600 | 页面入场动画总时长（三卡 stagger 120ms） |
| `homeuiANIM_RAYS_PERIOD_MS` | 6000 | 晴天光芒旋转一圈时长 |
| `homeuiANIM_FLOAT_PERIOD_MS` | 1200 | 雨天图标浮动往复周期（幅度 ±2px） |

---

## 3. EEZ Studio 设计规格（Screen `main_work` 重新设计，widget 树）

### 3.0 设计流程（SOP-A）

1. 打开 `eez_esp_c3_project/esp_c3.eez-project`，**清空既有 Screen `main_work` 的旧控件**（旧储能参数控件随之移除），保留 Screen 本体（名字不变 → `SCREEN_ID_MAIN_WORK`/`objects.main_work` 引用不变，故障页底屏、`ui_init()` 默认加载均不受影响）；
2. 新建 4 个子集字体（§2.2）与 4 张天气图标（§2.3）；
3. 新建 5 个 Native 变量（§3.6）；
4. 按下文 widget 树搭建控件，Label 的 text 绑定对应 Native 变量；
5. EEZ Studio **Build** → 生成 `eez_ui/`（`screens.c` 更新 `main_work`；`vars.h` 含变量声明）；
6. 手写新建 `eez_ui/vars.c`（当前工程尚无此文件；Native 变量实现唯一手写例外，不在 `.eez-project-build` 覆盖清单内）；
7. 修改 `md_display_page_work.c` 回调逻辑 + 新建 `user_ui/home_ui_ext.c/.h`，编译（见 §6）。

> 注：导航栏/格子样式建议在 EEZ 样式区建共享样式（如 `st_home_nav`、`st_home_cell`），调色只改样式。控件重复量大的应用网格可用 Node 脚本生成工程结构（架构参考 HS800 `gen_eng_mode.js` 模式）。

### 3.1 导航栏区（Screen 直接子对象，屏幕绝对坐标）

| 对象名 | EEZ 控件 | 坐标 (x,y) | 尺寸 | 样式要点 | 动态来源 |
|---|---|---|---|---|---|
| `nav_bar` | Container | (0, 0) | 320×32 | 背景 `#333333`、不透明度 70%、底边框 1px `#414750` | 静态 |
| `nav_time` | Label | (10, 0) | 100×32 | `home_cn_16`、`#FFFFFF`、左对齐垂直居中 | 绑定变量 `home_nav_time` |
| `sig_bar_0` | Object(矩形) | (280, 22) | 4×4 | 背景 `#FFFFFF` | user_ui 显隐 |
| `sig_bar_1` | Object(矩形) | (286, 18) | 4×8 | 背景 `#FFFFFF` | user_ui 显隐 |
| `sig_bar_2` | Object(矩形) | (292, 14) | 4×12 | 背景 `#FFFFFF` | user_ui 显隐 |
| `sig_bar_3` | Object(矩形) | (298, 10) | 4×16 | 背景 `#FFFFFF` | user_ui 显隐 |
| `sig_bar_4` | Object(矩形) | (304, 6) | 4×20 | 背景 `#FFFFFF` | user_ui 显隐 |

- 信号柱条：宽 4px、间距 2px、底部对齐于 y=26，高度 4/8/12/16/20 递增；
- 电平语义：`ucSignalLevel = N`（0~5）→ 前 N 根显示，其余隐藏（`lv_obj_add_flag(..., LV_OBJ_FLAG_HIDDEN)`）；**0 = 无信号（全隐藏）**；
- 分隔线由 `nav_bar` 底边框实现（或另建 `nav_line` 对象 (0,31) 320×1）。

### 3.2 Tileview 容器（Screen 直接子对象）

| 对象名 | EEZ 控件 | 坐标 (x,y) | 尺寸 | 说明 |
|---|---|---|---|---|
| `home_tileview` | Tileview | (0, 32) | 320×208 | 水平方向（`LV_DIR_HOR`），手势/吸附原生支持 |
| `tile_env` | TileViewTile (col 0, row 0) | — | 320×208 | 主页1，背景 `#20242B` |
| `tile_grid` | TileViewTile (col 1, row 0) | — | 320×208 | 主页2，背景 `#20242B` |

> **坐标基准**：Tile 内子对象坐标为 tile 相对坐标 = 屏幕绝对坐标 y − 32（下文 Tile 子对象表已按 tile 相对坐标给出）。

### 3.3 主页1 —— 环境页（`tile_env` 子对象，卡片化设计，tile 相对坐标）

布局：**温湿度双卡片**（左右并排）+ **底部天气信息条卡片**。三卡片统一视觉规范：背景 `#262B33`、圆角 12、边框 1px `#3A414C`；**不用 shadow**（软渲染开销大，用边框+色条表达层次）。

**① 温度卡片 `card_temp`**（tile 内 (12, 10)，144×108）：

| 对象名 | EEZ 控件 | 坐标(卡内) | 尺寸 | 样式要点 | 动态来源 |
|---|---|---|---|---|---|
| `card_temp` | Panel | — | 144×108 | 统一卡片视觉规范 | 入场动画对象 |
| `temp_accent` | Object | (12, 10) | 32×4 | 背景 `#FFB74D`、圆角 2（卡片识别色条） | 静态 |
| `temp_icon` | Image | (12, 20) | 20×20 | `ic_temp.png` | 静态 |
| `temp_title` | Label | (40, 21) | 92×18 | `home_cn_12`、`#9CA3AF`、静态文本「室内温度」 | 静态 |
| `temp_val` | Label | (12, 46) | 120×42 | `home_num_40`、`#FFFFFF`、左对齐 | 绑定变量 `home_temp_val` |
| `temp_desc` | Label | (12, 88) | 120×16 | `home_cn_12`、`#9CA3AF` | 绑定变量 `home_temp_desc` |

**② 湿度卡片 `card_humi`**（tile 内 (164, 10)，144×108，内布局同温度卡）：

| 对象名 | EEZ 控件 | 坐标(卡内) | 尺寸 | 样式要点 | 动态来源 |
|---|---|---|---|---|---|
| `card_humi` | Panel | — | 144×108 | 统一卡片视觉规范 | 入场动画对象 |
| `humi_accent` | Object | (12, 10) | 32×4 | 背景 `#4FC3F7`、圆角 2 | 静态 |
| `humi_icon` | Image | (12, 20) | 20×20 | `ic_humi.png` | 静态 |
| `humi_title` | Label | (40, 21) | 92×18 | `home_cn_12`、`#9CA3AF`、「室内湿度」 | 静态 |
| `humi_val` | Label | (12, 46) | 120×42 | `home_num_28`、`#FFFFFF`、左对齐 | 绑定变量 `home_humi_val` |
| `humi_desc` | Label | (12, 88) | 120×16 | `home_cn_12`、`#9CA3AF` | 绑定变量 `home_humi_desc` |

> 字号层级说明：温度 40px / 湿度 28px 为**主次分层设计**（温度为主指标）；湿度若用 40px 则最长串 `100%RH` 溢出，28px 下 7 字符×14px=98px < 120px 恰好安全。

**③ 天气信息条 `card_weather`**（tile 内 (12, 126)，296×70）：

| 对象名 | EEZ 控件 | 坐标(卡内) | 尺寸 | 样式要点 | 动态来源 |
|---|---|---|---|---|---|
| `card_weather` | Panel | — | 296×70 | 统一卡片视觉规范 | 入场动画对象 |
| `weather_icon_0` | Image | (8, 11) | 48×48 | `weather_sunny_core.png`（晴：太阳本体静态层） | user_ui 显隐 |
| `weather_rays` | Image | (8, 11) | 48×48 | `weather_sunny_rays.png`（晴：光芒层） | user_ui 显隐 + 旋转 |
| `weather_icon_1` | Image | (8, 11) | 48×48 | `weather_cloudy.png` | user_ui 显隐 |
| `weather_icon_2` | Image | (8, 11) | 48×48 | `weather_overcast.png` | user_ui 显隐 |
| `weather_icon_3` | Image | (8, 11) | 48×48 | `weather_rain.png` | user_ui 显隐 + 浮动 |
| `weather_text` | Label | (72, 12) | 60×24 | `home_cn_16`、`#FFFFFF` | 绑定变量 `home_weather_text` |
| `env_net_time` | Label | (72, 40) | 120×20 | `home_cn_12`、`#9CA3AF` | 绑定变量 `home_net_time` |
| `net_sync_text` | Label | (196, 30) | 72×16 | `home_cn_12`、`#9CA3AF`、右对齐 | 绑定变量 `home_net_sync` |
| `net_sync_dot` | Object | (272, 31) | 8×8 | 圆形（圆角 4）；同步 `#4ADE80` / 未同步 `#5A6068` | user_ui 设色 |

- 图标组五对象同坐标 (8,11) 叠放：晴 = `icon_0` + `rays` 双图层，多云/阴/雨 = 单图层；运行时仅当前态可见（user_ui 控制）；
- 数值格式（显示任务内格式化，无浮点 printf）：

```c
/* 温度: sTempX10 = 温度×10; 40px 单 label 含单位; |值|越界降级整数格式防溢出 */
if ((p_data->sTempX10 >= 995) || (p_data->sTempX10 <= -95))
	snprintf(sa_buf, sizeof(sa_buf), "%d°C", p_data->sTempX10 / 10);
else if (p_data->sTempX10 < 0)
	snprintf(sa_buf, sizeof(sa_buf), "-%d.%d°C", (-p_data->sTempX10) / 10, (-p_data->sTempX10) % 10);
else
	snprintf(sa_buf, sizeof(sa_buf), "%d.%d°C", p_data->sTempX10 / 10, p_data->sTempX10 % 10);

/* 湿度: 28px 含 %RH */
snprintf(sa_buf, sizeof(sa_buf), "%u%%RH", p_data->ucHumidity);

/* 导航时间 */
snprintf(sa_buf, sizeof(sa_buf), "%02u:%02u", p_data->ucLocalHour, p_data->ucLocalMin);
```

- **体感描述文案规则**（格式化时按阈值选定，写入 `home_temp_desc` / `home_humi_desc`）：

| 温度区间 | 文案 | | 湿度区间 | 文案 |
|---|---|---|---|---|
| < 18.0℃ | 偏冷 | | < 40% | 偏干 |
| 18.0 ~ 26.0℃ | 体感舒适 | | 40 ~ 70% | 舒适 |
| 26.0 ~ 30.0℃ | 偏热 | | > 70% | 潮湿 |
| ≥ 30.0℃ | 炎热 | | | |

- **动画规格**（user_ui `vHomeUi_ManualTick()` 驱动，机制见 §4.3）：

| 动画 | 对象 | 参数 | 触发 |
|---|---|---|---|
| 卡片入场 | `card_temp` / `card_humi` / `card_weather` | 透明度 0→100 + y 偏移 +12→0，单卡 360ms，三卡 stagger 120ms，总 `homeuiANIM_ENTER_MS`=600ms | `vOnEnter` 一次性 |
| 晴天光芒旋转 | `weather_rays` | 0→360° 匀速，`homeuiANIM_RAYS_PERIOD_MS`=6000ms/圈，transform pivot 设中心 (24, 24) | 天气=晴 且页面活动 |
| 雨天轻浮动 | `weather_icon_3` | y = 11 + 2·sin(2πt / `homeuiANIM_FLOAT_PERIOD_MS`)，±2px | 天气=雨 且页面活动 |
| 多云 / 阴 | — | 静态无动画 | — |

- 未同步兜底：导航/网络时间 `--:--`、`--:--:--`，天气图标全隐藏、文本 `--`，同步点灰 `#5A6068`、文案「未同步」。

### 3.4 主页2 —— 应用网格（`tile_grid` 子对象，tile 相对坐标）

> **布局说明（横屏适配，横3竖2）**：内容区 320×208 为横屏宽扁区域，网格取**横向 3 列 × 纵向 2 行**（横3竖2，共 6 格）。原需求「图标区 80×80」在此布局下**精确成立，无参数冲突**：
> - 横向：3×80 = 240，余 80 → 左右边距 20 + 列间距 20×2，间距完全均匀；
> - 纵向：2×80 = 160，余 48 → 上下边距 16 + 行间距 16，间距完全均匀（16+80+16+80+16 = 208 精确填满）。

6 个格子容器（EEZ 控件 Container，背景 `#262B33`、边框 1px `#3A414C`、圆角 6，建议共享样式 `st_home_cell`）：

| 对象名 | 网格位置 | 坐标 (tile内) | 尺寸 | 对应屏幕坐标 |
|---|---|---|---|---|
| `app_cell_0` | 第1行·第1列 | (20, 16) | 80×80 | (20, 48) |
| `app_cell_1` | 第1行·第2列 | (120, 16) | 80×80 | (120, 48) |
| `app_cell_2` | 第1行·第3列 | (220, 16) | 80×80 | (220, 48) |
| `app_cell_3` | 第2行·第1列 | (20, 112) | 80×80 | (20, 144) |
| `app_cell_4` | 第2行·第2列 | (120, 112) | 80×80 | (120, 144) |
| `app_cell_5` | 第2行·第3列 | (220, 112) | 80×80 | (220, 144) |

格内布局（每格 80×80 内相对坐标，格内子对象）：

| 对象名 | EEZ 控件 | 坐标(格内) | 尺寸 | 样式要点 |
|---|---|---|---|---|
| `app_icon_ph_i` | Object | (16, 6) | 48×48 | 背景 `#333B47`、圆角 6、边框 1px `#3A414C`（图标占位框） |
| `app_name_i` | Label | (0, 58) | 80×16 | `home_cn_12`、`#C9CFD8`、居中、静态文本 |

默认 6 格名称（EEZ 设计器直接填写静态文本，按行排布）：第1行 `设置`、`时钟`、`天气`；第2行 `网络`、`关于`、`主题`。当前**不实现点击跳转**，点击回调入口在 user_ui 预留（见 §7）。

### 3.5 EEZ Flow 不可实现项 → user_ui 归属汇总

| 功能 | user_ui 操作对象 | 机制 |
|---|---|---|
| 信号格显隐 | `objects.sig_bar_0..4` | `lv_obj_add/clear_flag(..., LV_OBJ_FLAG_HIDDEN)`，缓存比对 |
| 天气图层显隐（五层） | `objects.weather_icon_0..3` / `objects.weather_rays` | 同上；晴=icon_0+rays，其余单层 |
| 同步点设色 | `objects.net_sync_dot` | `lv_obj_set_style_bg_color(...)`（绿/灰二态，缓存比对） |
| 天气动画（光芒旋转/雨天浮动/入场） | `objects.weather_rays` / `weather_icon_3` / `card_temp` 等 | `vHomeUi_ManualTick()` 状态机（见 §4.3），不依赖 LVGL 定时器 |
| tileview 逻辑切换 | `objects.home_tileview` | `lv_tileview_set_tile_by_index(...)` |
| 防抖判定 | `objects.home_tileview` | `lv_obj_is_scrolling(...)` |
| 应用点击（预留） | `objects.app_cell_0..5` | `lv_obj_add_event_cb(..., LV_EVENT_CLICKED, ...)` |

### 3.6 Native 变量定义（EEZ Studio 中新建）

| 变量名 | 类型 | 初始值 | 绑定控件 |
|---|---|---|---|
| `home_nav_time` | string | `--:--` | `nav_time` |
| `home_temp_val` | string | `--.-°C` | `temp_val` |
| `home_temp_desc` | string | `--` | `temp_desc` |
| `home_humi_val` | string | `--%RH` | `humi_val` |
| `home_humi_desc` | string | `--` | `humi_desc` |
| `home_net_time` | string | `--:--:--` | `env_net_time` |
| `home_weather_text` | string | `--` | `weather_text` |
| `home_net_sync` | string | `未同步` | `net_sync_text` |

数据流（EEZ Studio 数据桥模式，§9）：

```
数据源任务 ──vDisp_DataSetXxx()──► 数据池(临界区)
显示任务 vOnUpdate ──vDisp_DataCaptureSnapshot()──► p_data
  └─ vHomeUi_Update(p_data):
       ├─ snprintf 格式化 → set_var_home_xxx()      [字符串变量桥]
       ├─ objects.sig_bar_i 显隐                      [user_ui 直控]
       └─ objects.weather_icon_i 显隐                 [user_ui 直控]
vDisp_PortUiRefresh 出口: ui_tick() ── EEZ 自动比对变量 ──► 刷新绑定 Label
```

---

## 4. 滑动切换逻辑（user_ui 扩展层，与 UI 设计、数据更新解耦）

### 4.1 切换接口（`user_ui/home_ui_ext.c`）

| 接口 | 允许上下文 | 机制 |
|---|---|---|
| `bool bHomeUi_SetTile(uint8_t uc_idx, bool b_anim)` | 仅显示任务 | `lv_tileview_set_tile_by_index(objects.home_tileview, uc_idx, 0, b_anim ? LV_ANIM_ON : LV_ANIM_OFF)` |
| `uint8_t ucHomeUi_GetTile(void)` | 仅显示任务 | 读当前 tile 索引 |
| `void vHomeUi_RequestTile(uint8_t uc_idx)` | 任意任务 | 原子单字节写请求寄存器（`0xFF` = 无请求） |
| `void vHomeUi_PollRequest(void)` | 仅显示任务（`vOnUpdate` 首步） | 读并清请求寄存器 → 合法则 `bHomeUi_SetTile(idx, true)` |
| `void vHomeUi_Update(const DispDataSnapshot_T *p_data)` | 仅显示任务 | 变量桥 + 显隐更新（NULL 防御 + 缓存比对） |
| `void vHomeUi_AnimaEnter(void)` | 仅显示任务（页面 `vOnEnter`） | 启动动画：置活动标志 + 记录入场状态机起始时戳 |
| `void vHomeUi_AnimaStop(void)` | 仅显示任务（页面 `vOnExit`） | 清活动标志：ManualTick 门槛后直接返回，动画停、零脏区 |
| `void vHomeUi_ManualTick(void)` | 仅显示任务（`vDisp_UiRefresh` 出口） | 时基 `lv_tick_get()`；驱动入场/光芒旋转/雨天浮动（见 §4.3） |

### 4.2 防抖与越界规则

1. `bHomeUi_SetTile` 入口先判 `lv_obj_is_scrolling(objects.home_tileview)`，滚动中返回 false（手势进行期间逻辑请求被拒）；
2. `uc_idx >= homeuiTILE_COUNT` 返回 false；
3. 手势释放由 tileview snap 自动吸附（LVGL 原生 scroll throw），无需额外代码；
4. 滚动期间数据照常刷新（label 更新不受滚动影响）。

### 4.3 动画驱动机制（ManualTick，单上下文零定时器）

架构约定（skill §10-B）：自绘动画**不依赖 LVGL 定时器**，统一由 `vHomeUi_ManualTick()` 在渲染出口驱动，保持 LVGL 单上下文：

```
vDisp_UiRefresh() 渲染出口（md_display_api.c）:
    ui_tick();                     /* 1. EEZ 变量绑定比对 */
    vHomeUi_ManualTick();          /* 2. 主页动画状态机 [新增调用点] */
    lv_timer_handler();            /* 3. LVGL 定时器与重绘 */
```

实现要点：

1. `ManualTick` 内部以 `lv_tick_get()` 取毫秒时基，按 §3.3 动画规格计算各对象目标态；**仅变化才写控件**（旋转角每帧必写，透明度/位移插值仅在动画窗口内写）；
2. 活动门控：`G_bHomeAnimActive == false`（页面退出/非 WORK 页）时函数开头直接 return，不产生任何脏区；
3. 入场动画为一次性状态机：`homeuiANIM_ENTER_MS` 窗口结束后各卡复位为最终态（`LV_OPA_COVER`、y=设计值），窗口外不再触碰控件；
4. 光芒旋转：`lv_obj_set_style_transform_rotate(objects.weather_rays, s_ang_01deg, 0)`；pivot 需设为图层中心 (24, 24)——在 EEZ 工程设 transform pivot，或 user_ui Init 时 `lv_obj_set_style_transform_pivot_x/y(..., 24, ...)`；
5. 雨天浮动：`lv_obj_set_y(objects.weather_icon_3, 11 + offset)`，offset 为 ±2px 查表正弦（整数近似，避免浮点 sin）；
6. 性能预算：旋转层每帧脏区 48×48（RGB565 全帧 DMA 15.4ms 下占比极小）；若实测帧率不足，退化为步进模式（每 750ms 转 45°，8 步/圈）。

---

## 5. 数据模型与更新接口

### 5.1 快照扩展（`md_display_data.h` → `struct DispDataSnapshot_T` 末尾追加）

```c
	/* ---- 主页 UI 扩展字段 (追加于既有字段之后, 不改动既有布局) ---- */
	int16_t				sTempX10;			/* 室内温度×0.1℃ (源: tGxhtc3 换算) */
	uint8_t				ucHumidity;			/* 室内湿度 0~100 %RH */
	uint8_t				ucSignalLevel;		/* 网络信号格数 0~5 */
	uint8_t				ucLocalHour;		/* 导航栏本地时 (系统时钟, 未接入=0xFF) */
	uint8_t				ucLocalMin;
	uint8_t				ucNetHour;			/* 网络同步时 (未同步=0xFF) */
	uint8_t				ucNetMin;
	uint8_t				ucNetSec;
	uint8_t				eWeather;			/* Weather_E */
```

```c
typedef enum
{
	WEATHER_SUNNY = 0,		/* 晴 */
	WEATHER_CLOUDY,			/* 多云 */
	WEATHER_OVERCAST,		/* 阴 */
	WEATHER_RAIN,			/* 雨 */
	WEATHER_MAX
}Weather_E;
```

### 5.2 数据写入 API（`md_display_data.c` 实现，任意任务可调，临界区保护多字节一致性）

| 接口 | 调用方 | 时机 |
|---|---|---|
| `void vDisp_DataSetEnv(int16_t s_temp_x10, uint8_t uc_humi)` | 传感器任务 | GXHTC3 每次采样换算后（0.1℃ / 1%RH） |
| `void vDisp_DataSetNetTime(uint8_t h, uint8_t m, uint8_t s)` | 网络/WiFi 任务 | SNTP 同步成功后周期调用 |
| `void vDisp_DataSetWeather(Weather_E e_weather)` | 网络/WiFi 任务 | 天气接口返回后 |

### 5.3 捕获规则（`vDisp_DataCaptureSnapshot` 内）

| 字段 | 捕获方式 |
|---|---|
| `sTempX10` / `ucHumidity` / 网络时间 / `eWeather` | 临界区读内部数据池（与 5.2 写入对称） |
| `ucSignalLevel` | 临界区外直读 `tWiFiInfo.cRssi`（单读者安全），经下表分级 |
| `ucLocalHour/Min` | 直读系统时钟；未接入时钟源时置 `0xFF`，导航栏显示 `--:--` |

**RSSI → 信号格数分级表：**

| RSSI (dBm) | 格数 |
|---|---|
| ≥ -55 | 5 |
| [-66, -55) | 4 |
| [-77, -66) | 3 |
| [-88, -77) | 2 |
| [-99, -88) | 1 |
| < -99 或未连接（`WIFI_STATE_DISCONNECTED`/`STOPPED`） | 0 |

### 5.4 vars.c 手写实现（`eez_ui/vars.c`，新建文件，EEZ 构建不覆盖）

`vars.h` 的 `get_var_xxx/set_var_xxx` 声明由 EEZ Build 生成；`vars.c` 手写实现，`strncpy` 带边界：

```c
static char s_saHomeNavTime[8] = "--:--";

const char *get_var_home_nav_time(void)
{
	return (s_saHomeNavTime);
}

void set_var_home_nav_time(const char *p_value)
{
	strncpy(s_saHomeNavTime, p_value, sizeof(s_saHomeNavTime) - 1);
	s_saHomeNavTime[sizeof(s_saHomeNavTime) - 1] = '\0';
}

/* home_temp_val[10] / home_humi_val[8] / home_net_time[10] / home_weather_text[8]
   home_temp_desc[12] / home_humi_desc[8] / home_net_sync[8] 同模式 */
```

> 缓冲区尺寸按最长格式预留：`-13°C`(5+1)、`100%RH`(6+1)、`HH:MM:SS`(8+1)、天气文本(2 汉字+1)、体感文案(UTF-8 3 汉字 9+1 取 12)。

---

## 6. 文件结构与页面契约（复用 `G_tPageWork`）

### 6.1 新增/修改文件清单

```
Hardware/MD_Display/
│
├─ eez_esp_c3_project/
│  ├─ esp_c3.eez-project              [修改] 重新设计 Screen main_work / 4 字体 / 4 图片 / 5 Native 变量
│  └─ Image/
│     └─ weather_*.png ×4             [新增]
│
├─ eez_ui/                             [EEZ Build 生成, 只读]
│  ├─ screens.c / screens.h / styles.c / images.c / ui_font_home_*.c   [生成]
│  └─ vars.c                          [手写新建] Native 变量 get/set (唯一手写例外)
│
├─ md_display_page_work.c             [修改] 既有 WORK 页回调适配新主页 (见 6.2)
├─ md_display_api.c                   [修改] vDisp_UiRefresh() 渲染出口插入 vHomeUi_ManualTick() (见 4.3)
├─ md_display_data.c / .h             [修改] 快照扩展字段 + 写入API
└─ user_ui/
   ├─ home_ui_ext.c / .h              [新增] EEZ 扩展层: 切页 API + 信号/天气显隐 + 变量桥
   └─ CMakeLists.txt                  [修改] 追加源文件
```

> **零改动清单**：`uni_disp_page.h`（不加枚举）、`md_display_task.c/.h`（已注册 `G_tPageWork`）、`eDisp_MapDevStateToPage()`（`DS_WORK → PAGE_ID_WORK` 维持）、UniDisplay core/event。
> 红线：`eez_ui/` 除 `vars.c` 外零手改。

### 6.2 既有页面契约适配（`md_display_page_work.c`，`G_tPageWork` 控制块不变）

| 回调 | 现状 → 改后 |
|---|---|
| `vOnEnter` | 保留：`SCREEN_ID_MAIN_WORK` 加载判断、首帧快照 + `lv_refr_now(NULL)`、背光点亮；**删除**：狗 GIF 运行时挂载/恢复逻辑（`s_p_dog_gif` 及相关代码，GIF 资源保留在工程中可日后挂到 tile 作装饰，见 §7）；**新增**：`vHomeUi_Update(&t_snapshot)` 首刷 + `vHomeUi_AnimaEnter()` 启动入场/天气动画（替代原 `vDisp_UpdateDevParam` 空钩子调用） |
| `vOnUpdate` | `vHomeUi_PollRequest()` → `vHomeUi_Update(p_data)`（禁阻塞、禁直读业务全局变量；原 `vDisp_UpdateDevParam` 为空钩子可直接移除调用） |
| `bOnEvent` | 现透传 false → 改为消费 `DISP_EVT_KEY_SHORT`：`bHomeUi_SetTile((ucHomeUi_GetTile() ^ 1), true)` 循环切换并返回 true（消费按键需自行重置息屏倒计时，参照框架约定）；其余事件透传 false |
| `vOnExit` | **新增**：`vHomeUi_AnimaStop()` 停止动画（清活动标志，ManualTick 此后零开销零脏区）；GIF 暂停逻辑删除后无其他自建资源 |
| `usRefreshMs` | 维持 33 不变 |

### 6.3 既有耦合点核查结论（改前已核实）

| 耦合点 | 核查结果 | 影响 |
|---|---|---|
| 故障页 `md_display_page_fault.c` | `vOnEnter` 加载 `SCREEN_ID_MAIN_WORK` 作底屏 | 无需改动，故障弹窗自动叠加在新主页上 |
| `md_display_api.c` | `vDisp_UpdateDevParam()` 为空钩子（无旧控件引用） | 可直接移除页面内调用或保留 |
| `eez_ui/ui.c` | `ui_init()` 默认 `loadScreen(SCREEN_ID_MAIN_WORK)` | Screen 名不变，默认加载路径不受影响 |
| 狗 GIF | `img_dog_gif.c` 为 user_ui 独立资源，页面运行时挂载 | 删除挂载代码即可，资源文件可留作扩展 |

---

## 7. 扩展接口清单（预留模块标注）

| 扩展点 | 位置 | 扩展方式 |
|---|---|---|
| **应用点击跳转** | user_ui 给 `objects.app_cell_i` 绑 `LV_EVENT_CLICKED` | 回调内 `vDisp_RequestPage(PAGE_ID_XXX)`；当前不绑即无动作 |
| **应用图标替换** | EEZ 工程：`app_icon_ph_i` 换 Image 控件 + `Image/` 加图 | 重新 Build 即生效 |
| **应用名称修改** | EEZ 设计器改 `app_name_i` 静态文本（或改绑变量） | 重新 Build |
| **狗 GIF 装饰**（可选） | user_ui 在 `tile_env` 挂载 `img_dog_gif`（资源已在工程） | 复用 `md_display_page_work.c` 原 GIF 挂载模式 |
| **更多动效** | user_ui `vHomeUi_ManualTick()` 状态机加分支 | 数值滚动、卡片呼吸、天气切换过渡等 |
| **天气状态扩充** | EEZ 加图标 + `Weather_E` 加枚举 + user_ui 显隐表加行 | 雪雾等 |
| **页面数扩充** | EEZ 工程加 TileViewTile（col+1）+ `homeuiTILE_COUNT` 递增 | 切页/防抖逻辑自动适配 |
| **页面指示点**（可选） | EEZ 加 2 个小圆点对象，user_ui 随 `bHomeUi_SetTile` 同步显隐 | 位置预留 (156/164, 230) 8×8 |
| **内容从导航栏下滑过**（可选视觉变体） | tileview 扩至全屏 + 导航栏置顶半透明 | 需放开「无重叠」约束，默认不启用 |
| **温湿度数据源** | `vDisp_DataSetEnv` | 已具备 GXHTC3（`bsp_gxhtc3_temp_humi.c`） |
| **网络时间/天气源** | `vDisp_DataSetNetTime` / `vDisp_DataSetWeather` | WiFi 任务（`wifi_task.c`）接入 SNTP/HTTP 天气后调用 |

---

## 8. 实现注意事项与验收清单

### 8.1 注意事项

1. **EEZ Build 会整体覆盖 `eez_ui/`**：一切 UI 改动必须回到 `eez_project/` 重做后重新 Build，禁止直接改生成文件（`vars.c` 除外）；
2. 中文字体必须子集化（§2.2 字符集），`°`(U+00B0) 勿遗漏；
3. 字符串一律 `snprintf`/`strncpy`，禁 `sprintf`/`strcpy`；
4. `set_var_xxx` 每帧调用安全（`ui_tick()` 内 EEZ 自动比对，值不变不刷控件）；信号/天气显隐在 user_ui 侧再做一层静态缓存比对；
5. 页面四回调禁止阻塞（无 `vTaskDelay`/等信号量/耗时循环）；
6. GXHTC3 原始值（`tGxhtc3.usTemp/usHumi`）换算到 0.1℃ / 1%RH 时核对传感器实际缩放；
7. 命名遵循工程规范：导出 `vHomeUi_/bHomeUi_/G_tPage` 前缀、static 小写下划线、宏 `homeui` 前缀；
8. 文件头注释/段落横幅/`#endif` 条件注释对齐 `.agents/rules/c-code-templates.md`；
9. 改造 `md_display_page_work.c` 时同步删除 GIF 相关的 static 变量与 include（`img_dog_gif.h`），避免残留死代码；
10. **动画性能红线**：旋转层每帧 48×48 脏区、入场动画 600ms 窗口内三卡重绘，均在 RGB565 承受力内；`vOnExit` 必须调 `vHomeUi_AnimaStop()`，页面退出/息屏后 ManualTick 不得产生任何脏区；禁用 LVGL shadow/blur 等软渲染重效果，层次用边框+色条表达。

### 8.2 验收清单

- [ ] EEZ Build 后生成内容一致：`SCREEN_ID_MAIN_WORK` 为新主页布局、`objects` 子树命名同 §3；`vars.h` 含 8 个变量声明；
- [ ] 主页1 三卡片布局正确：色条/图标/标题/数值/体感文案坐标与 §3.3 一致，卡内无溢出；
- [ ] 页面进入时三卡片 600ms 入场动画（渐显+上移，stagger 120ms，一次性）；
- [ ] 天气=晴：光芒层持续旋转（6000ms/圈，绕中心）；天气=雨：图标 ±2px 浮动；多云/阴：无动画；
- [ ] 体感文案随温湿度阈值切换正确（偏冷/体感舒适/偏热/炎热、偏干/舒适/潮湿）；
- [ ] 同步点颜色与文案联动（已同步绿点 / 未同步灰点）；
- [ ] 退出页面/息屏后动画停止（无持续脏区，CPU 占用回落）；
- [ ] 手势左右滑切页，松手平滑吸附无跳变；
- [ ] 逻辑切换（按键 / `vHomeUi_RequestTile`）产生 250ms 量级平滑过渡；
- [ ] 滚动进行中新逻辑请求被拒绝，无撕裂/错位；
- [ ] 导航时间、温湿度、网络时间、天气文本变化才刷新（变量比对生效）；
- [ ] 信号 0~5 格显隐正确（0 格全隐藏）；天气四态图标切换正确；
- [ ] 未联网：信号 0 格、网络时间 `--:--:--`、天气 `--`；
- [ ] 主页2 六格占位完整、名称正确、点击无跳转；
- [ ] 故障注入验证：故障弹窗仍以 `main_work` 为底屏正常弹出/清除；
- [ ] 按键短按在两 tile 间切换且息屏倒计时被重置；
- [ ] `eez_ui/` 除 `vars.c` 外零手改；UniDisplay core/event 零改动；`uni_disp_page.h`/`md_display_task.c` 零改动；
- [ ] 编译通过，命名与模板规范符合 `.agents/rules`。
