# ESP-C3 主页 UI 设计方案 v2.0（320×240 横屏 · EEZ Studio 主导 + 序列帧 Animation）

| 项 | 值 |
|---|---|
| 文档类型 | UI 设计规格书（参数级，供 AI 与工程师直接实现） |
| 目标硬件 | ST7789 320×240 SPI 屏（`dispTFT_WIDTH/HEIGHT`）+ FT6336U 电容触摸（`lv_port_indev.c` 已具备） |
| UI 框架 | LVGL 9.4（`managed_components/lvgl__lvgl`） |
| 调度架构 | UniDisplay（`Middlewares/UniDisplay` + `Hardware/MD_Display`） |
| 页面载体 | **复用既有 `PAGE_ID_WORK`（`md_display_page_work.c`）**，**不新增页面** |
| **交付形态** | **EEZ Studio 工程全面主导（SOP-A 主路径）**：承担全部布局、共享样式（含 Pressed 按压态）、子集字体、静态图标、天气序列帧图集与 Native 变量绑定；`vars.c` 手写数据桥；`user_ui/` 仅保留最薄胶水层（Tile 逻辑切页、信号格显隐、LVGL `lv_anim` 序列帧与入场动画触发） |
| EEZ 工程 | `Hardware/MD_Display/eez_esp_c3_project/esp_c3.eez-project`，**改造既有 Screen `main_work`** |
| 设计日期 | 2026-10-14（v2.0 优化版） |

> **为什么复用 WORK 页而非新增页面**：`G_tPageWork` 已在 `b_task_param_init()` 注册、`DS_WORK` 已路由至 `PAGE_ID_WORK`、`ui_init()` 默认加载 `SCREEN_ID_MAIN_WORK`、故障页（`md_display_page_fault.c`）以 `main_work` 为底屏——WORK 页本就是工程的主页。复用方案零枚举、零注册、零路由改动，只改 EEZ 工程内的 Screen 内容与页面回调内部逻辑。

---

## 1. 总体规格

### 1.1 基本参数

| 参数 | 值 | 说明 |
|---|---|---|
| 分辨率 | 320(H) × 240(V) 横屏 | RGB565 |
| 页面载体 | EEZ Screen `main_work`（仍生成 `SCREEN_ID_MAIN_WORK` 与 `objects.*`） | 单屏承载双 tile + 底部指示点 |
| 页面结构 | 固定导航栏（32px）+ `Tileview`（208px，双 Tile）+ 底部悬浮指示点 | 主页1（tile 0）/ 主页2（tile 1） |
| 交互触发 | A. 手势滑动（tileview 原生） B. 指示点点击 / 按键逻辑切换 | 统一 250ms ease-out 平滑过渡 |
| 天气动效方案 | **GIF 拆分为多张序列帧 PNG → EEZ Images 导出 → `lv_anim` 序列帧播放** | 规避 EEZ 不支持 GIF 限制，完整还原四态天气动效 |
| 页面帧周期 | `usRefreshMs = 33` | 维持 `G_tPageWork` 现值；触摸滑动由 `dispINDEV_POLL_PERIOD_MS=16` 快泵保证流畅 |
| 数据模式 | 帧快照消费（`DispDataSnapshot_T`）+ EEZ Native 变量数据桥 | 渲染链只读 `p_data`，与数据更新解耦 |

### 1.2 布局总骨架（320×240 像素严格对齐）
(0,0)   ┌──────────────────────────────────────┐ (320,0)
│ 12:34  10/14 周三               ▂▄▆█ │ ← 导航信息栏 y=0..31（高32px）
(0,31)  ├──────────────────────────────────────┤ ← 1px 底边框分隔线（#343A44）
│  ┌─────────────┐    ┌─────────────┐  │
│  │ 室内温度 舒适│    │ 室内湿度 适宜│  │ ← 温湿度双卡 y=40..141（tile内 y=8, 144×102）
│  │ 25.5 °C     │    │ 45 %RH      │  │
│  └─────────────┘    └─────────────┘  │
│  ┌────────────────────────────────┐  │
│  │ [48×48序列帧] 晴  12:34:56 已同步│  │ ← 天气卡片 y=150..221（tile内 y=118, 296×72）
│  └────────────────────────────────┘  │
│                 ━━ •                 │ ← 翻页指示点 y=230（与卡片底边留出 8px 呼吸边距）
(0,240) └──────────────────────────────────────┘

- **上栏（`y = 0..31`）**：导航信息栏，高 32px，直接挂在 Screen 根对象下，不随 Tile 滑动；
- **下栏（`y = 32..239`）**：主内容区高 208px，为 `home_tileview` 窗口，两 Tile 横向排列；
- **前景指示点（`y = 230`）**：直接挂在 Screen 根对象最上层（Z-order 位于 `home_tileview` 之上），胶囊态 `12×6px` + 圆点态 `6×6px`，间距 `4px`，居中于 `x = 160`。

### 1.3 职责分工（EEZ 最大化接管 / user_ui 极简辅助）

| 功能模块 | 归属 | 实现方式 |
|---|---|---|
| 全部布局、卡片、色条、单位静态文本、底部指示点 | **EEZ Studio 工程** | SOP-A 主路径直接搭建 |
| 应用网格 6 格圆角卡片 + 6 枚应用图标 + 按压变色（`PRESSED`） | **EEZ Studio 工程** | EEZ Image 控件 + 共享样式 `st_home_cell`（含 Default/Pressed 双态） |
| 字体子集化（仅 3 套字体）与全部图片/序列帧编译打包 | **EEZ Studio 工程** | Build 自动生成 `eez_ui/images.c` 与 `ui_font_*.c` |
| 动态文本（导航时间/温湿度纯数值/体感文案/网络时间/天气/同步状态） | **EEZ Native 变量 + `vars.c`** | `set_var_xxx` → `ui_tick()` 自动比对刷新 |
| 天气四态序列帧轮播 & 卡片入场渐显上移动画 | **LVGL 原生 `lv_anim_t`**（`user_ui` 封装） | 操作 EEZ 生成的 `objects.weather_anim_img` 切换帧索引，滑动期间自动挂起 |
| 信号格 5 档颜色/显隐、同步圆点颜色、底部指示点胶囊态切换 | **user_ui** | 操作 `objects.sig_bar_i`、`objects.net_sync_dot`、`objects.page_dot_i`（带静态缓存比对） |
| tileview 逻辑切页、按键切页、跨任务请求防抖 | **user_ui** | `lv_tileview_set_tile_by_index(objects.home_tileview, ...)` |

---

## 2. 全局设计参数

### 2.1 颜色表（EEZ Studio 样式表配置）

| 语义 | 颜色值 | 用途 |
|---|---|---|
| 根背景 | `#1B1E23` | Screen `main_work` 背景 |
| 内容页背景 | `#20242B` | `tile_env` / `tile_grid` 背景 |
| 导航栏背景 / 底边框 | `#262B33`（85% 不透明） / `#343A44` | 顶部状态栏与 y=31 分隔线 |
| 卡片 / 应用格默认背景 | `#262B33` | 温湿度卡、天气卡、主页2 应用格 Default 态 |
| 卡片 / 应用格按压背景 | `#303742` | 天气卡、主页2 应用格 `PRESSED` 态 |
| 卡片 / 应用格边框 | `#3A414C`（1px，圆角 12） | 统一 12px 圆角规范，禁用阴影（Shadow） |
| 应用图标内底座背景 | `#313844`（圆角 10，无边框） | 去除内层多余边框套娃 |
| 主文字 / 数值文字 | `#FFFFFF` | 导航时间、温湿度主数值、天气名称 |
| 次要文字 / 单位文字 | `#9CA3AF` | 卡片标题、`°C` / `%RH` 单位、网络时间 |
| 辅助说明文字 | `#6B7280` | 适宜区间提示文字 |
| 温度主题色 / 徽章底色 | `#FFB74D` / `#3D3226` | 温度左侧色条、温度计图标、右上角体感文字 |
| 湿度主题色 / 徽章底色 | `#4FC3F7` / `#223847` | 湿度左侧色条、水滴图标、右上角体感文字 |
| 状态点亮色 / 熄灭色 | `#4ADE80`（绿） / `#5A6068`（灰） | 同步状态圆点、信号熄灭格、未激活翻页点 |
| 翻页指示点激活色 | `#E3E6EA` | 当前页胶囊指示点 |

### 2.2 字体表（精简为 3 套子集字体，节省 Flash）

> **优化说明**：由于将 `°C` 和 `%RH` 拆分为独立的 `12px` 静态 Label，温湿度数值得以统一使用 `32px` 字体（无需单独生成 40px 与 28px 两套数字字库）。

| 字体名（EEZ） | 字号 | 字符集范围 | 用途 |
|---|---|---|---|
| `home_num_32` | 32px | `0123456789.-`（仅 12 个字符） | 室内温度数值、室内湿度数值 |
| `home_cn_16` | 16px | ASCII + `晴多云阴小大雨雪雾` | 顶部导航时间、天气主文字 |
| `home_cn_12` | 12px | ASCII + `°` + `设置时钟天气网络关于主题室内温度湿度体感舒适适宜偏热冷炎润潮干已未同步更新周日一二三四五六区间` | 单位（`°C`/`%RH`）、卡片标题、体感徽章、区间提示、网络时间、应用名称 |

### 2.3 图片与序列帧资源清单（`eez_esp_c3_project/Image/`）

> **导出格式规范**：
> - **静态小图标（传感器 & 应用图标）**：导出为 `CF_TRUE_COLOR_ALPHA`（RGB565A8）或带底座 `CF_TRUE_COLOR`（RGB565）；
> - **天气动效序列帧（48×48）**：直接以卡片背景色 `#262B33` 垫底导出为 **不带透明通道的 `RGB565`**，不仅彻底消除 GIF 透明锯齿杂边，而且每帧体积从 `6.9 KB` 降至 `4.6 KB`，LVGL 刷屏免去 Alpha 混合计算！

| 类别 | 文件命名规范 | 尺寸 | 数量 | 帧间隔 / 说明 |
|---|---|---|---|---|
| 传感器图标 | `ic_temp.png` / `ic_humi.png` | 20×20 | 2 张 | 暖橙 `#FFB74D` 温度计 / 浅蓝 `#4FC3F7` 水滴 |
| 主页2 应用图标 | `ic_app_settings.png`、`ic_app_clock.png`、`ic_app_weather.png`、`ic_app_network.png`、`ic_app_about.png`、`ic_app_theme.png` | 24×24 | 6 张 | 对应设置、时钟、天气、网络、关于、主题 |
| 晴天序列帧 | `anim_sunny_00.png` ~ `anim_sunny_07.png` | 48×48 | 8 帧 | 太阳光芒具有 45° 对称性，8 帧旋转 45° 循环等效于 64 帧转 360°，每帧 90ms |
| 多云序列帧 | `anim_cloudy_00.png` ~ `anim_cloudy_07.png` | 48×48 | 8 帧 | 太阳右上浮动 + 云层左下错位往复，每帧 150ms（周期 1.2s 或往复 `LV_ANIM_PLAYBACK`） |
| 阴天序列帧 | `anim_overcast_00.png` ~ `anim_overcast_07.png` | 48×48 | 8 帧 | 前后双云水平反向漂移，每帧 160ms（支持正反往复播放 `playback`） |
| 雨天序列帧 | `anim_rain_00.png` ~ `anim_rain_07.png` | 48×48 | 8 帧 | 三道雨滴错峰下落渐隐循环，每帧 100ms（周期 800ms） |

> **Flash 占用核算**：4 组天气 × 8 帧 × `48×48×2B` = `147.4 KB`；加上 8 张静态小图标约 `9 KB`，总计仅约 **156 KB**。

### 2.4 通用常量（`user_ui/home_ui_ext.h`）

| 宏 | 值 | 说明 |
|---|---|---|
| `homeuiTILE_COUNT` | 2 | tile 数量（主页1 + 主页2） |
| `homeuiAPP_COUNT` | 6 | 应用格子数量 |
| `homeuiSWIPE_ANIM_MS` | 250 | 逻辑切页平滑过渡时长 |
| `homeuiANIM_ENTER_MS` | 360 | 单卡片入场动画时长（三卡 stagger 120ms，与 HTML 预览严格一致） |
| `homeuiWEATHER_FRAME_CNT` | 8 | 每种天气状态的序列帧帧数 |

---

## 3. EEZ Studio 控件树规格（Screen `main_work`）

### 3.1 顶部导航栏（Screen 直接子对象，屏幕绝对坐标 `y = 0..31`）

| 对象名 | EEZ 控件 | 坐标 (x,y) | 尺寸 | 样式要点 | 动态来源 |
|---|---|---|---|---|---|
| `nav_bar` | Container | (0, 0) | 320×32 | 背景 `#262B33`、底边框 1px `#343A44` | 静态 |
| `nav_time` | Label | (12, 0) | 56×32 | `home_cn_16`、`#FFFFFF`、左对齐垂直居中 | 绑定变量 `home_nav_time` |
| `sig_bar_0` | Object | (278, 21) | 4×4 | 圆角 1；点亮 `#FFFFFF` / 熄灭 `#4B5563` | user_ui 设色或显隐 |
| `sig_bar_1` | Object | (284, 17) | 4×8 | 同上 | user_ui 设色或显隐 |
| `sig_bar_2` | Object | (290, 13) | 4×12 | 同上 | user_ui 设色或显隐 |
| `sig_bar_3` | Object | (296, 9) | 4×16 | 同上 | user_ui 设色或显隐 |
| `sig_bar_4` | Object | (302, 5) | 4×20 | 同上 | user_ui 设色或显隐 |

### 3.2 Tileview 容器与底部翻页指示点（Screen 直接子对象）

| 对象名 | EEZ 控件 | 坐标 (x,y) | 尺寸 | 样式要点 | 说明 |
|---|---|---|---|---|---|
| `home_tileview` | Tileview | (0, 32) | 320×208 | 水平滚动（`LV_DIR_HOR`），隐藏原生滚动条 | 承载两个子 Tile |
| `tile_env` | TileViewTile | col 0, row 0 | 320×208 | 背景 `#20242B` | 主页1（环境与天气） |
| `tile_grid` | TileViewTile | col 1, row 0 | 320×208 | 背景 `#20242B` | 主页2（3×2 应用网格） |
| `page_dot_0` | Object | (149, 230) | 12×6 | 圆角 3，背景 `#E3E6EA` | 挂在 Screen 根层，随切页在 12×6 与 6×6 间切换 |
| `page_dot_1` | Object | (165, 230) | 6×6 | 圆角 3，背景 `#5A6068` | 与 `page_dot_0` 保持 4px 间距，总宽 22px 居中于 X=160 |

### 3.3 主页1 —— 环境与天气页（`tile_env` 子对象，坐标相对 `tile_env` 左上角）

> **布局优化要点**：温湿度卡片高度设为 `102px`（`y = 8..109`），天气卡片置于 `y = 118..189`（高 `72px`），距离 `y = 198`（即屏幕绝对坐标 `y = 230`）的翻页指示点留出 **8px 安全边距**，彻底解决旧版仅差 2px 的拥挤感。

**① 温度卡片 `card_temp`**（tile 内坐标 `(12, 8)`，尺寸 `144×102`）：

| 对象名 | EEZ 控件 | 坐标(卡内) | 尺寸 | 样式要点 | 动态来源 |
|---|---|---|---|---|---|
| `card_temp` | Panel | (12, 8) | 144×102 | 背景 `#262B33`、边框 1px `#3A414C`、圆角 12 | 入场动画对象 |
| `temp_accent` | Object | (0, 14) | 3×16 | 贴左边缘竖条，背景 `#FFB74D`、右圆角 2 | 静态 |
| `temp_icon` | Image | (12, 12) | 20×20 | `ic_temp.png`（暖橙色） | 静态 |
| `temp_title` | Label | (36, 13) | 56×18 | `home_cn_12`、`#9CA3AF`、静态文本「室内温度」 | 静态 |
| `temp_badge` | Label | (96, 13) | 38×16 | `home_cn_12`、`#FFB74D`、背景 `#3D3226`、圆角 4、居中 | 绑定变量 `home_temp_desc` |
| `temp_val` | Label | (12, 40) | 82×36 | `home_num_32`、`#FFFFFF`、左对齐 | 绑定变量 `home_temp_val`（仅数值） |
| `temp_unit` | Label | (96, 52) | 32×18 | `home_cn_12`、`#9CA3AF`、静态文本「°C」 | **EEZ 静态文本** |
| `temp_range` | Label | (12, 78) | 120×16 | `home_cn_12`、`#6B7280`、静态文本「适宜区间 22~26°C」 | 静态 |

**② 湿度卡片 `card_humi`**（tile 内坐标 `(164, 8)`，尺寸 `144×102`）：

| 对象名 | EEZ 控件 | 坐标(卡内) | 尺寸 | 样式要点 | 动态来源 |
|---|---|---|---|---|---|
| `card_humi` | Panel | (164, 8) | 144×102 | 背景 `#262B33`、边框 1px `#3A414C`、圆角 12 | 入场动画对象 |
| `humi_accent` | Object | (0, 14) | 3×16 | 贴左边缘竖条，背景 `#4FC3F7`、右圆角 2 | 静态 |
| `humi_icon` | Image | (12, 12) | 20×20 | `ic_humi.png`（浅蓝色） | 静态 |
| `humi_title` | Label | (36, 13) | 56×18 | `home_cn_12`、`#9CA3AF`、静态文本「室内湿度」 | 静态 |
| `humi_badge` | Label | (96, 13) | 38×16 | `home_cn_12`、`#4FC3F7`、背景 `#223847`、圆角 4、居中 | 绑定变量 `home_humi_desc` |
| `humi_val` | Label | (12, 40) | 64×36 | `home_num_32`、`#FFFFFF`、左对齐 | 绑定变量 `home_humi_val`（仅数值） |
| `humi_unit` | Label | (78, 52) | 40×18 | `home_cn_12`、`#9CA3AF`、静态文本「%RH」 | **EEZ 静态文本** |
| `humi_range` | Label | (12, 78) | 120×16 | `home_cn_12`、`#6B7280`、静态文本「适宜区间 40~60%」 | 静态 |

**③ 天气信息条卡片 `card_weather`**（tile 内坐标 `(12, 118)`，尺寸 `296×72`）：

| 对象名 | EEZ 控件 | 坐标(卡内) | 尺寸 | 样式要点 | 动态来源 |
|---|---|---|---|---|---|
| `card_weather` | Panel | (12, 118) | 296×72 | 背景 `#262B33`、边框 1px `#3A414C`、圆角 12 | 入场动画对象 |
| `weather_anim_img` | Image | (12, 12) | 48×48 | 默认指向 `anim_sunny_00.png` | **单控件承载全部四态序列帧动画** |
| `weather_text` | Label | (72, 14) | 110×22 | `home_cn_16`、`#FFFFFF` | 绑定变量 `home_weather_text` |
| `env_net_time` | Label | (72, 40) | 120×18 | `home_cn_12`、`#9CA3AF` | 绑定变量 `home_net_time` |
| `net_sync_dot` | Object | (216, 32) | 6×6 | 圆角 3；已同步 `#4ADE80` / 未同步 `#5A6068` | user_ui 设色 |
| `net_sync_text` | Label | (226, 27) | 56×16 | `home_cn_12`、`#9CA3AF`、左对齐 | 绑定变量 `home_net_sync` |

### 3.4 主页2 —— 3×2 应用网格（`tile_grid` 子对象，EEZ 全权生成图标与按压态）

> **纵坐标优化**：第 1 行置于 `y = 12`，第 2 行置于 `y = 104`（底边位于 `y = 184`，距离 `y = 198` 的底部指示点留出 `14px` 净空，不再遮挡指示点）。
> **样式配置（EEZ 共享样式 `st_home_cell`）**：
> - `DEFAULT` 状态：背景 `#262B33`、边框 1px `#3A414C`、圆角 `12px`；
> - `PRESSED` 状态：背景 `#303742`、边框 1px `#4B5563`（触摸按下时 LVGL 自动呈现按压反馈，无需手写代码）。

| 格子对象名 | 坐标 (tile内) | 尺寸 | 内部底座 `app_box_i` | 内部图标 `app_ic_i` (24×24) | 内部名称 `app_name_i` |
|---|---|---|---|---|---|
| `app_cell_0` | (20, 12) | 80×80 | (18, 8) 44×44 `#313844` 圆角10 | (10, 10) `ic_app_settings.png` | (0, 57) 80×16 「设置」 |
| `app_cell_1` | (120, 12) | 80×80 | (18, 8) 44×44 `#313844` 圆角10 | (10, 10) `ic_app_clock.png` | (0, 57) 80×16 「时钟」 |
| `app_cell_2` | (220, 12) | 80×80 | (18, 8) 44×44 `#313844` 圆角10 | (10, 10) `ic_app_weather.png` | (0, 57) 80×16 「天气」 |
| `app_cell_3` | (20, 104) | 80×80 | (18, 8) 44×44 `#313844` 圆角10 | (10, 10) `ic_app_network.png` | (0, 57) 80×16 「网络」 |
| `app_cell_4` | (120, 104) | 80×80 | (18, 8) 44×44 `#313844` 圆角10 | (10, 10) `ic_app_about.png` | (0, 57) 80×16 「关于」 |
| `app_cell_5` | (220, 104) | 80×80 | (18, 8) 44×44 `#313844` 圆角10 | (10, 10) `ic_app_theme.png` | (0, 57) 80×16 「主题」 |

### 3.5 Native 变量定义（EEZ Studio 变量表，共 8 个字符串变量）

| 变量名 | 类型 | 默认初始值 | 绑定控件 | 格式示例（不含静态单位） |
|---|---|---|---|---|
| `home_nav_time` | string | `--:--` | `nav_time` | `"12:34"` |
| `home_temp_val` | string | `--.-` | `temp_val` | `"25.5"`（单位 `°C` 为独立静态 Label） |
| `home_temp_desc` | string | `--` | `temp_badge` | `"舒适"` / `"偏冷"` / `"偏热"` / `"炎热"` |
| `home_humi_val` | string | `--` | `humi_val` | `"45"`（单位 `%RH` 为独立静态 Label） |
| `home_humi_desc` | string | `--` | `humi_badge` | `"适宜"` / `"偏干"` / `"潮湿"` |
| `home_weather_text` | string | `--` | `weather_text` | `"晴"` / `"多云"` / `"阴"` / `"雨"` |
| `home_net_time` | string | `--:--:--` | `env_net_time` | `"12:34:56"` |
| `home_net_sync` | string | `未同步` | `net_sync_text` | `"已同步"` / `"未同步"` |

---

## 4. 序列帧动画与切页控制实现（`user_ui/home_ui_ext.c`）

### 4.1 序列帧表与 `lv_anim_t` 驱动设计

EEZ Build 后会在 `eez_ui/images.h` 中导出所有序列帧的 `lv_img_dsc_t` 结构体指针。在 `home_ui_ext.c` 中定义二维帧表并使用 LVGL 标准 `lv_anim_t` 驱动（或者在 `vHomeUi_ManualTick` 中按毫秒取模切帧，两者皆可，以下展示 **LVGL 标准 Animation 方式**）：

```c
/* 四态天气序列帧查表 (8帧/态, 由 EEZ Build 生成于 images.h) */
static const lv_img_dsc_t *const s_aptWeatherFrames[WEATHER_MAX][homeuiWEATHER_FRAME_CNT] = {
    [WEATHER_SUNNY]    = { &img_anim_sunny_00,    ..., &img_anim_sunny_07    },
    [WEATHER_CLOUDY]   = { &img_anim_cloudy_00,   ..., &img_anim_cloudy_07   },
    [WEATHER_OVERCAST] = { &img_anim_overcast_00, ..., &img_anim_overcast_07 },
    [WEATHER_RAIN]     = { &img_anim_rain_00,     ..., &img_anim_rain_07     },
};

/* 各天气状态的单轮周期与是否往复播放 (多云/阴天为正弦往复运动, 开启 playback 帧数翻倍更丝滑) */
typedef struct {
    uint16_t usDurationMs;
    bool     bPlayback;
} WeatherAnimCfg_T;

static const WeatherAnimCfg_T s_atWeatherAnimCfg[WEATHER_MAX] = {
    [WEATHER_SUNNY]    = { 720,  false }, /* 45° 对称循环, 720ms×8 = 5760ms/圈 */
    [WEATHER_CLOUDY]   = { 2000, true  }, /* 去程 2s + 返程 2s = 4s 周期 */
    [WEATHER_OVERCAST] = { 2500, true  }, /* 去程 2.5s + 返程 2.5s = 5s 周期 */
    [WEATHER_RAIN]     = { 1200, false }, /* 1.2s 循环下落 */
};

/* lv_anim 帧切换执行回调 */
static void prv_vWeatherAnimExecCb(void *p_var, int32_t l_frame_idx)
{
    /* 性能优化：若 Tileview 正在滑动或当前处于主页2 (tile 1)，跳过切帧避免无效重绘 */
    if (lv_obj_is_scrolling(objects.home_tileview) || (ucHomeUi_GetTile() != 0)) {
        return;
    }
    if ((s_eCurWeather < WEATHER_MAX) && (l_frame_idx < homeuiWEATHER_FRAME_CNT)) {
        lv_image_set_src((lv_obj_t *)p_var, s_aptWeatherFrames[s_eCurWeather][l_frame_idx]);
    }
}