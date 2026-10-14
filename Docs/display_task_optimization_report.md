# 显示任务全面评估与优化报告

- 日期：2026-10-14
- 对象：ESP-C3 (G02) 显示任务 = UniDisplay 调度核心 + MD_Display 项目件 + EEZ UI + LVGL 9.4
- 基线提交：881982f (v2.0.2) 工作区状态
- 构建环境：ESP-IDF v5.5.2 / esp32c3

---

## 1. 现有实现梳理

### 1.1 架构组成

```
Key/Sys 等任务                    DispTask (prio 4, 栈 6KB)
  │ bDisp_PostEvent / bDisp_Switch   │
  ▼                                  ▼
uni_disp_event.c (环形队列深4)  md_display_task.c (通知+超时等待, 耗时补偿)
                                     │ vDisp_EnginePoll()
                                     ▼
                        uni_disp_core.c 调度引擎
              快照 → 状态路由 → 事件分发 → 切页 → 息屏门控渲染
                 │                                   │
        md_display_data.c                    8 个页面 (四回调契约)
        (快照/路由表/故障轮播)                      │ vDisp_UiRefresh()
                                                  ▼
                                   ui_tick() → lv_timer_handler()
                                                  │
                                   lv_port_disp.c → SPI DMA (esp_lcd ST7789)
```

### 1.2 当前配置基准

| 项目 | 值 |
| :--- | :--- |
| 屏幕 | ST7789 320×240, SPI 40MHz, RGB565_SWAPPED |
| LVGL 渲染模式 | PARTIAL，双缓冲 40 行 ×2 = 51,200 B（运行时 DMA 堆分配） |
| LVGL 内置内存池 | 64 KB 静态 .bss（`work_mem_int`） |
| FreeRTOS 节拍 | 100 Hz（pdMS_TO_TICKS 粒度 10 ms） |
| 框架默认帧周期 | 50 ms；页面声明 20/33/50/100 ms，下限钳位 33 ms |
| 息屏策略 | 180 s 无操作息屏；息屏时任务 2 s 低频心跳 |
| EEZ UI | 5 屏约 10 个控件，无图片资源 |

---

## 2. 多维度量化评估

### 2.1 渲染效率

| 指标 | 优化前基准 | 说明 |
| :--- | :--- | :--- |
| 实际帧周期（33 ms 页） | **40 ms（25 FPS）** | 100 Hz 节拍下 `pdMS_TO_TICKS(33)` 上取整为 4 tick |
| 全屏 DMA 传输耗时 | ≈31 ms | 153,600 B @ 40 MHz，双缓冲可与渲染流水重叠 |
| 耗时补偿粒度 | 10 ms | 帧周期补偿以 tick 计 |
| 关机动画保持时长 | 1.8 s（设计 1.5 s） | 45 帧 × 实际 40 ms |
| 故障轮播周期 | 2.42 s（设计 2.0 s） | 按 33 ms 累加但调用间隔 40 ms |

### 2.2 资源占用（构建实测）

| 指标 | 优化前 |
| :--- | :--- |
| G02.bin | 623,088 B（app 分区余量 80%） |
| DRAM 占用 | 150,356 / 321,296 B（46.8%） |
| 其中 LVGL 静态池 | 65,536 B（占 DRAM 用量 43.6%） |
| 显示 DMA 双缓冲 | 51,200 B（运行时堆） |

LVGL 池对当前 5 屏/10 控件 UI 严重过剩，是最大单项静态内存。

### 2.3 响应速度

| 指标 | 优化前基准 | 说明 |
| :--- | :--- | :--- |
| 事件入队→处理 | ≤10 ms（亮屏） | 任务通知即时唤醒 |
| **跨任务切页请求（息屏时）** | **最长 2,000 ms** | `vDisp_RequestPage` 不写任务通知，等 2 s 心跳 |
| 按键亮屏 | ≤10 ms | key_task 直调 bDisp_Switch（含唤醒） |

### 2.4 兼容性

- LVGL 单上下文遵守良好：其它任务一律事件/请求中转，未发现跨任务直触 LVGL。
- `eez_ui/` 只读约束未被破坏；渲染链只读快照 `p_data`。
- 快照临界区保护 16/32 位字段，低频量单读者直读，符合快照三原则。

### 2.5 可维护性（对照架构红线）

| 红线 | 状态 |
| :--- | :--- |
| eez_ui 只读 | ✅ |
| LVGL 单上下文 | ✅ |
| 页面回调无阻塞 | ✅ |
| **通用件零项目依赖** | ❌ `uni_disp_core.c` 直接 include 4 个项目头，引用 `tSysInfo`/`tAppMemParam`/`vDisp_SetBacklight`/`vDisp_UiRefresh`/`bBuz_Tweet`；port 原语缺 5 个 |
| 快照消费模式 | ✅ |
| 帧周期下限 | ✅（但页面声明 20 ms 与钳位 33 ms 不一致，存在误导） |

### 2.6 用户体验一致性

- ❌ 上电时序：接口层默认开背光 → `vDisp_Init` 关 → boot 页再开，产生**上电闪白屏**。
- ⚠️ 帧率不足导致的动画/进度条偏慢（25 FPS vs 设计 30 FPS）。

### 2.7 问题清单汇总

| # | 严重度 | 问题 | 位置 |
| :--- | :--- | :--- | :--- |
| P0-1 | 高 | FreeRTOS 100 Hz 致帧周期 40 ms、时序全面偏慢 21% | sdkconfig |
| P0-2 | 高 | `vDisp_RequestPage` 不唤醒任务，息屏切页最长 2 s | uni_disp_core.c |
| P1-1 | 中 | core 项目耦合，违反通用件零依赖红线 | uni_disp_core.c |
| P1-2 | 中 | 上电闪白屏 | md_display_iface.c |
| P2-1 | 低 | LVGL 池 64 KB 过剩 | sdkconfig |
| P2-2 | 低 | 页面声明帧周期与钳位值不一致 | work/boot/eng 页 |

---

## 3. 优化方案与实施

| 优先级 | 优化点 | 预期效果 | 状态 |
| :--- | :--- | :--- | :--- |
| P0 | `CONFIG_FREERTOS_HZ` 100→1000 | 帧周期 40→33 ms（25→30 FPS）；关机保持/故障轮播时长回归设计值；全系统定时精度 ×10 | ✅ 已实施 |
| P0 | `vDisp_RequestPage` 增加 `vDisp_PortWakeTask()` | 息屏切页延迟 2,000→≤2 ms | ✅ 已实施 |
| P1 | port 补齐 5 个适配原语，core 去除全部项目 include | 红线合规；core/page/event 与移植母本通用件对齐，移植只改 port | ✅ 已实施 |
| P1 | 移除接口层默认开背光 | 消除上电闪白屏 | ✅ 已实施 |
| P2 | LVGL 池 64→32 KB | 释放 32 KB 片内 DRAM | ✅ 已实施 |
| P2 | 页面 `usRefreshMs` 20→33 显式声明 | 消除配置歧义（行为不变） | ✅ 已实施 |

### 3.1 修改文件清单

| 文件 | 修改内容 |
| :--- | :--- |
| `sdkconfig` / `sdkconfig.defaults` | `CONFIG_FREERTOS_HZ=1000`；`CONFIG_LV_MEM_SIZE_KILOBYTES=32` |
| `Middlewares/UniDisplay/uni_disp_port.h` | 引入项目快照头（工程适配点）；声明 5 个适配原语 |
| `Middlewares/UniDisplay/uni_disp_port.c` | 实现 `vDisp_PortUiRefresh` / `vDisp_PortSetBacklight` / `usDisp_PortGetAutoOffTime` / `bDisp_PortIsSystemReady` / `vDisp_PortBeepOnLightOn` |
| `Middlewares/UniDisplay/uni_disp_core.c` | 删除 4 个项目 include 与 `app_info.h` 等引用，全部改经 port 原语；`vDisp_RequestPage` 增加任务唤醒 |
| `Hardware/MD_Display/md_display_iface.c` | 删除 `vDisp_IfaceInit` 末尾默认开背光 |
| `md_display_page_work/boot/eng.c` | `usRefreshMs` 20→33 |

行为保持：port 原语均为 1:1 转调；蜂鸣原语内部按 `boardBUZ_EN`（当前 0）裁剪，语义与原 `#if` 块一致；就绪判定 `!= 0` 与原 `== 0` 取反逻辑等价。

---

## 4. 优化前后对比（构建实测）

| 指标 | 优化前 | 优化后 | 变化 |
| :--- | :--- | :--- | :--- |
| G02.bin | 623,088 B | 623,168 B | +80 B（+0.013%，原语函数开销） |
| DRAM 占用 | 150,356 B（46.8%） | **117,578 B（36.6%）** | **−32,778 B（−21.8%）** |
| .bss | 74,064 B | 41,296 B | −32,768 B |
| LVGL 池（map 实测） | 0x10000 | 0x8000 | −32 KB |
| 亮屏帧周期（33 ms 页） | 40 ms（25 FPS） | **33 ms（30 FPS）** | −17.5% |
| 息屏跨任务切页延迟 | ≤2,000 ms | **≤2 ms** | −99.9% |
| 关机动画保持 | 1.8 s | 1.5 s（=设计值） | 修复 |
| 故障轮播周期 | 2.42 s | 2.0 s（=设计值） | 修复 |
| 上电闪白屏 | 存在 | 消除 | 修复 |
| 编译警告（本次修改文件） | — | 0 | 全量+定向重编均通过 |

> 运行时帧率/延迟为依时钟参数的推算值：1000 Hz 下 `pdMS_TO_TICKS(33)=33` 精确成立；唤醒经 `xTaskNotifyGive` + `ulTaskNotifyTake`，处理延迟为 1 个 tick（1 ms）量级。建议上机用健康巡检任务（uxTaskGetRunTimeStats）复核 DispTask CPU 占用。

## 5. 兼容性回归分析

| 变更 | 影响面 | 结论 |
| :--- | :--- | :--- |
| FreeRTOS 1000 Hz | 全系统 `pdMS_TO_TICKS`/延时精度变细；tick ISR 频率 ×10（160 MHz C3 开销 <1%）；健康巡检、看门狗、1 s 定时器均按 ms 参数化，语义不变 | 安全 |
| LVGL 池 32 KB | 当前 UI 峰值占用估算 <4 KB（5 屏 10 控件）；余量 8×。**后续 EEZ 引入大图/多字体需实测并按需回提** | 安全（附观察项） |
| RequestPage 唤醒 | 显示任务内自调用产生一次多余通知，无害；跨任务调用即时生效 | 安全 |
| core 去耦合 | 纯转发，无逻辑变化 | 安全 |
| 背光时序 | GPIO 初始化态即熄灭（上拉=灭），页面状态机负责点亮；无任何流程依赖接口层默认点亮 | 安全 |
| 页面 33 ms 声明 | 与原钳位结果一致 | 无行为变化 |

## 6. 遗留观察项（未实施，供后续决策）

1. **背光 PWM 调光**：高/低亮参数（0x88~0x8F）当前仅存参数区未接硬件，BL 为 GPIO 开关；如需调光需改 LEDC PWM 或数字电位器驱动。
2. **息屏时渲染链空转**：息屏期间 EnginePoll 每 2 s 仍抓快照+路由（必要，用于状态沿检测），开销已极低，无需处理。
3. **DispTask 栈水位**：6 KB 栈建议上机经 `get_heapstack_task` 复核一次水位（LVGL 9 + EEZ 深调用链）。
4. **80 行双缓冲上机复核**：本轮已提升为 80 行（见第 7 节），需上机经 `v_print_dma_memory_info` 日志确认 buf2 分配成功（失败自动降级单缓冲，功能不损）。

---

# 第二轮：帧率/渲染效率与触摸响应专项（2026-10-14）

## 7. 优化项与预期收益

| # | 优化点 | 文件 | 预期收益 |
| :- | :- | :- | :- |
| R1 | SPI 时钟 40→80 MHz | md_display_iface.h | 全帧 DMA 30.7→**15.4 ms**；40 MHz 时全帧重绘（30.7 ms DMA+渲染）已逼近 33 ms 帧周期上限，80 MHz 后全屏动画可持续 30 FPS |
| R2 | LVGL 组件单独 `-O2`（全局保持 Debug） | 根 CMakeLists.txt | LVGL 软件渲染吞吐约 1.5~2×；compile_commands 实测 788 个 LVGL 源文件 `-O2` 位于 `-Og` 之后生效，其余 964 个文件保持 `-Og` |
| R3 | 显示缓冲 40→80 行 | lv_port_disp.c | 全帧 DMA 分块 6→3 次，渲染/传输重叠更深；运行时堆 +51.2 KB（上轮已释放 32.8 KB，净占用仍低于优化前） |
| R4 | 帧间插入 16 ms 触摸快泵 `vDisp_UiPump()` | md_display_task.c / md_display_api.c | `lv_timer_handler` 驱动频率 33→16 ms；快泵只跑 LVGL 定时器不跑 EEZ `ui_tick`，页面数据节奏仍由帧周期控制 |
| R5 | indev 采样周期 33→16 ms（`lv_timer_set_period(lv_indev_get_read_timer())`） | lv_port_indev.c | 解除 LVGL 默认 `LV_DEF_REFR_PERIOD` 33 ms 采样上限，与快泵节拍对齐 |

触摸延迟链分析（按键/触摸 → 视觉反馈）：
- 优化前：采样 ≤33 ms + LVGL 处理 + 下一帧渲染 ≤33 ms → 最长 ≈66 ms；
- 优化后：采样 ≤16 ms + 当次泵内即处理渲染 → 最长 ≈33 ms，**响应延迟约减半**。
- 事件即时性不受影响：快泵等待中收到任务通知（按键事件/亮屏）立即跳出进入 EnginePoll 处理。

## 8. 第二轮构建验证（实测）

| 指标 | 第一轮后 | 第二轮后 | 说明 |
| :- | :- | :- | :- |
| G02.bin | 623,168 B | 704,560 B | +81.4 KB（+13.1%，LVGL -O2 代码体积），分区余量 78% |
| DRAM 静态占用 | 117,586 B（36.6%） | 117,586 B（36.6%） | 不变（80 行缓冲为运行时堆分配，+51.2 KB 运行时堆，余量充足） |
| 编译警告（本轮修改文件） | — | 0 | 定向重编验证 |
| 全帧 DMA 耗时 | 30.7 ms | 15.4 ms | 理论值 @80 MHz |
| 触摸采样间隔 | 33 ms | 16 ms | 实测配置生效路径：`lv_indev_get_read_timer` + `lv_timer_set_period` |

## 9. 第二轮兼容性回归分析

| 变更 | 影响面 | 结论 |
| :- | :- | :- |
| SPI 80 MHz | 仅显示总线；ST7789 写时序普遍支持，**若现场排线长出现花屏，回退 40 MHz 一处宏即可** | 安全（附回退预案） |
| LVGL -O2 | 仅 LVGL 组件；不改变 API 行为，断言仍开启（`LV_USE_ASSERT_*`） | 安全 |
| 80 行双缓冲 | 运行时堆多占 51.2 KB；buf2 分配失败自动降级单缓冲（原有容错） | 安全 |
| 触摸快泵 | 全部调用仍在显示任务单上下文；息屏时不泵（保持 0% CPU 心跳）；`S_bForceRefresh`/页面帧周期语义不变 | 安全 |
| indev 16 ms | FT6336 I2C 读取每次约 0.25 ms @400 kHz，泵内读取占空比约 1.6% | 安全 |

## 10. 上机验证清单（建议）

1. `DISP_IFACE` 启动日志确认 DMA Largest ≥ 52 KB（buf2 分配成功）；
2. 全屏切页/动画观察是否仍有撕裂（80 MHz + 3 分块应明显改善）；
3. 触摸按钮跟手性主观对比；
4. 健康巡检复核 DispTask 栈水位与 CPU 占用（快泵后预计 CPU +1~2%）。
