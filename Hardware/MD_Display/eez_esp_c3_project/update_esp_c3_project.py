#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
根据《ESP-C3 主页 UI 设计方案 v2.0》自动合成 Image 资源并完善 esp_c3.eez-project 工程
运行方式: 在 eez_esp_c3_project 目录下执行 `python update_esp_c3_project.py`
"""

import os
import json
import uuid
import math
import shutil
import base64
from io import BytesIO
from PIL import Image

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
IMG_DIR = os.path.join(BASE_DIR, "Image")
PROJ_PATH = os.path.join(BASE_DIR, "esp_c3.eez-project")
STATE_PATH = os.path.join(BASE_DIR, "esp_c3.eez-project-ui-state")

CARD_BG_RGBA = (0x26, 0x2B, 0x33, 255)  # 卡片背景色 #262B33


def new_id():
    return str(uuid.uuid4())


def img_to_data_uri(img_path):
    with open(img_path, "rb") as f:
        b64 = base64.b64encode(f.read()).decode("ascii")
    return f"data:image/png;base64,{b64}"


def shift_image(im, dx, dy):
    """亚像素双线性平移图层"""
    return im.transform(
        im.size,
        Image.AFFINE,
        (1, 0, -dx, 0, 1, -dy),
        resample=Image.BICUBIC,
    )


def prepare_all_images():
    """
    1. 复制/规范化 2 张传感器图标 (20x20) 与 6 张应用图标 (24x24)
    2. 使用 Image/layer_*.png 合成 #262B33 垫底的 4 组 x 8 帧 (48x48) 天气序列帧
    """
    os.makedirs(IMG_DIR, exist_ok=True)
    bitmap_entries = []

    # 1. 静态图标映射 (源文件名 -> 方案规范文件名与 EEZ Bitmap 名称)
    static_map = [
        ("icon_temp_20x20.png", "ic_temp.png", "ic_temp"),
        ("icon_humi_20x20.png", "ic_humi.png", "ic_humi"),
        ("app_settings_24x24.png", "ic_app_settings.png", "ic_app_settings"),
        ("app_clock_24x24.png", "ic_app_clock.png", "ic_app_clock"),
        ("app_weather_24x24.png", "ic_app_weather.png", "ic_app_weather"),
        ("app_network_24x24.png", "ic_app_network.png", "ic_app_network"),
        ("app_about_24x24.png", "ic_app_about.png", "ic_app_about"),
        ("app_theme_24x24.png", "ic_app_theme.png", "ic_app_theme"),
    ]

    for src_name, dst_name, bmp_name in static_map:
        src_path = os.path.join(IMG_DIR, src_name)
        dst_path = os.path.join(IMG_DIR, dst_name)
        if os.path.exists(src_path) and not os.path.exists(dst_path):
            shutil.copyfile(src_path, dst_path)
        target_path = dst_path if os.path.exists(dst_path) else src_path
        bitmap_entries.append({
            "objID": new_id(),
            "name": bmp_name,
            "image": img_to_data_uri(target_path),
            "bpp": 32,
            "style": "default"
        })

    # 2. 合成四态天气序列帧 (8帧/态, 48x48, #262B33 不透明底色消除锯齿并省去 Alpha 混合)
    sunny_core = Image.open(os.path.join(IMG_DIR, "layer_sunny_core_48x48.png")).convert("RGBA")
    sunny_rays = Image.open(os.path.join(IMG_DIR, "layer_sunny_rays_48x48.png")).convert("RGBA")
    cloudy_sun = Image.open(os.path.join(IMG_DIR, "layer_cloudy_sun_48x48.png")).convert("RGBA")
    cloudy_cloud = Image.open(os.path.join(IMG_DIR, "layer_cloudy_cloud_48x48.png")).convert("RGBA")
    overcast_back = Image.open(os.path.join(IMG_DIR, "layer_overcast_back_48x48.png")).convert("RGBA")
    overcast_front = Image.open(os.path.join(IMG_DIR, "layer_overcast_front_48x48.png")).convert("RGBA")
    rain_cloud = Image.open(os.path.join(IMG_DIR, "layer_rain_cloud_48x48.png")).convert("RGBA")
    rain_drops = Image.open(os.path.join(IMG_DIR, "layer_rain_drops_48x48.png")).convert("RGBA")

    # (a) 晴天 8 帧: 45° 对称旋转, 每帧旋转 45/8 = 5.625°
    for i in range(8):
        canvas = Image.new("RGBA", (48, 48), CARD_BG_RGBA)
        angle = -(i * (45.0 / 8.0))
        rotated_rays = sunny_rays.rotate(angle, resample=Image.BICUBIC)
        canvas.alpha_composite(rotated_rays)
        canvas.alpha_composite(sunny_core)
        out_name = f"anim_sunny_{i:02d}"
        out_path = os.path.join(IMG_DIR, f"{out_name}.png")
        canvas.convert("RGB").save(out_path, "PNG")
        bitmap_entries.append({
            "objID": new_id(),
            "name": out_name,
            "image": img_to_data_uri(out_path),
            "bpp": 16,
            "style": "default"
        })

    # (b) 多云 8 帧: 太阳右上浮动 + 云层左下错位往复 (配合 lv_anim playback)
    for i in range(8):
        t = i / 7.0
        canvas = Image.new("RGBA", (48, 48), CARD_BG_RGBA)
        sun_shifted = shift_image(cloudy_sun, 2.0 * t, -2.0 * t)
        cloud_shifted = shift_image(cloudy_cloud, -1.5 * t, 1.0 * t)
        canvas.alpha_composite(sun_shifted)
        canvas.alpha_composite(cloud_shifted)
        out_name = f"anim_cloudy_{i:02d}"
        out_path = os.path.join(IMG_DIR, f"{out_name}.png")
        canvas.convert("RGB").save(out_path, "PNG")
        bitmap_entries.append({
            "objID": new_id(),
            "name": out_name,
            "image": img_to_data_uri(out_path),
            "bpp": 16,
            "style": "default"
        })

    # (c) 阴天 8 帧: 前后双云水平反向漂移 (配合 lv_anim playback)
    for i in range(8):
        t = i / 7.0
        canvas = Image.new("RGBA", (48, 48), CARD_BG_RGBA)
        back_shifted = shift_image(overcast_back, 1.5 * t, 0.0)
        front_shifted = shift_image(overcast_front, -2.0 * t, 0.0)
        canvas.alpha_composite(back_shifted)
        canvas.alpha_composite(front_shifted)
        out_name = f"anim_overcast_{i:02d}"
        out_path = os.path.join(IMG_DIR, f"{out_name}.png")
        canvas.convert("RGB").save(out_path, "PNG")
        bitmap_entries.append({
            "objID": new_id(),
            "name": out_name,
            "image": img_to_data_uri(out_path),
            "bpp": 16,
            "style": "default"
        })

    # (d) 雨天 8 帧: 三道雨滴错峰下落循环
    for i in range(8):
        phase = i / 8.0
        canvas = Image.new("RGBA", (48, 48), CARD_BG_RGBA)
        canvas.alpha_composite(rain_cloud)
        dy = -2.0 + 6.0 * phase
        drops_shifted = shift_image(rain_drops, 0.0, dy)
        canvas.alpha_composite(drops_shifted)
        out_name = f"anim_rain_{i:02d}"
        out_path = os.path.join(IMG_DIR, f"{out_name}.png")
        canvas.convert("RGB").save(out_path, "PNG")
        bitmap_entries.append({
            "objID": new_id(),
            "name": out_name,
            "image": img_to_data_uri(out_path),
            "bpp": 16,
            "style": "default"
        })

    return bitmap_entries


def make_base_widget(w_type, identifier, left, top, width, height, local_def=None, use_style="default", flags="", clickable=False):
    w = {
        "objID": new_id(),
        "type": w_type,
        "left": left,
        "top": top,
        "width": width,
        "height": height,
        "customInputs": [],
        "customOutputs": [],
        "style": {
            "objID": new_id(),
            "useStyle": use_style,
            "conditionalStyles": [],
            "childStyles": []
        },
        "timeline": [],
        "eventHandlers": [],
        "leftUnit": "px",
        "topUnit": "px",
        "widthUnit": "px",
        "heightUnit": "px",
        "children": [],
        "widgetFlags": flags,
        "hiddenFlagType": "literal",
        "clickableFlagType": "literal",
        "flagScrollbarMode": "OFF",
        "flagScrollDirection": "",
        "scrollSnapX": "",
        "scrollSnapY": "",
        "checkedStateType": "literal",
        "disabledStateType": "literal",
        "states": "",
        "localStyles": {
            "objID": new_id()
        },
        "group": "",
        "groupIndex": 0
    }
    if identifier:
        w["identifier"] = identifier
    if clickable:
        w["clickableFlag"] = True
    if local_def:
        w["localStyles"]["definition"] = local_def
    return w


def make_panel(identifier, left, top, width, height, local_def=None, use_style="default", clickable=False):
    flags = "CLICK_FOCUSABLE|GESTURE_BUBBLE|PRESS_LOCK|SNAPPABLE"
    if clickable:
        flags = "CLICKABLE|" + flags
    return make_base_widget("LVGLPanelWidget", identifier, left, top, width, height, local_def, use_style, flags, clickable)


def make_label(identifier, left, top, width, height, text, is_var=False, preview=None, local_def=None, use_style="default"):
    w = make_base_widget(
        "LVGLLabelWidget",
        identifier,
        left,
        top,
        width,
        height,
        local_def,
        use_style,
        "CLICK_FOCUSABLE|GESTURE_BUBBLE|PRESS_LOCK|SNAPPABLE",
        False
    )
    w["text"] = text
    w["textType"] = "expression" if is_var else "literal"
    w["longMode"] = "CLIP"
    w["recolor"] = False
    w["useStaticText"] = not is_var
    if preview is not None:
        w["previewValue"] = preview
    elif not is_var:
        w["previewValue"] = text
    return w


def make_image(identifier, left, top, width, height, bmp_name):
    w = make_base_widget(
        "LVGLImageWidget",
        identifier,
        left,
        top,
        width,
        height,
        None,
        "default",
        "GESTURE_BUBBLE|SNAPPABLE",
        False
    )
    w["image"] = bmp_name
    w["imageType"] = "literal"
    w["pivotX"] = 0
    w["pivotY"] = 0
    w["angle"] = 0
    w["zoom"] = 256
    w["scale"] = 256
    w["innerAlign"] = "CENTER"
    return w


def build_main_work_page():
    """根据设计方案第 3 节构建 Main_Work 页面完整控件树"""
    # 1. 顶部导航栏 nav_bar (0, 0, 320x32)
    nav_bar = make_panel(
        "nav_bar", 0, 0, 320, 32,
        local_def={
            "MAIN": {
                "DEFAULT": {
                    "bg_color": "#262B33",
                    "bg_opa": 217,
                    "border_color": "#343A44",
                    "border_width": 1,
                    "border_side": "BOTTOM",
                    "radius": 0,
                    "pad_top": 0,
                    "pad_bottom": 0,
                    "pad_left": 0,
                    "pad_right": 0
                }
            }
        }
    )
    nav_time = make_label(
        "nav_time", 12, 6, 56, 20,
        text="home_nav_time", is_var=True, preview="12:34",
        local_def={
            "MAIN": {
                "DEFAULT": {
                    "text_color": "#FFFFFF",
                    "text_font": "home_cn_16",
                    "text_align": "LEFT"
                }
            }
        }
    )
    nav_date = make_label(
        "nav_date", 68, 8, 72, 16,
        text="10/14 周三", is_var=False,
        local_def={
            "MAIN": {
                "DEFAULT": {
                    "text_color": "#9CA3AF",
                    "text_font": "home_cn_12",
                    "text_align": "LEFT"
                }
            }
        }
    )
    nav_bar["children"].extend([nav_time, nav_date])

    # 5 根信号格 sig_bar_0 ~ sig_bar_4
    sig_specs = [
        ("sig_bar_0", 278, 21, 4, 4, "#FFFFFF"),
        ("sig_bar_1", 284, 17, 4, 8, "#FFFFFF"),
        ("sig_bar_2", 290, 13, 4, 12, "#FFFFFF"),
        ("sig_bar_3", 296, 9, 4, 16, "#FFFFFF"),
        ("sig_bar_4", 302, 5, 4, 20, "#4B5563"),
    ]
    for s_id, sx, sy, sw, sh, scol in sig_specs:
        bar = make_panel(
            s_id, sx, sy, sw, sh,
            local_def={
                "MAIN": {
                    "DEFAULT": {
                        "bg_color": scol,
                        "bg_opa": 255,
                        "border_width": 0,
                        "radius": 1,
                        "pad_top": 0,
                        "pad_bottom": 0,
                        "pad_left": 0,
                        "pad_right": 0
                    }
                }
            }
        )
        nav_bar["children"].append(bar)

    # 2. 主页1 卡片构建 (挂于 tile_env)
    # ① 室内温度卡片 card_temp (12, 8, 144x102)
    card_temp = make_panel("card_temp", 12, 8, 144, 102, use_style="st_home_card")
    card_temp["children"] = [
        make_panel("temp_accent", 0, 14, 3, 16, local_def={
            "MAIN": {"DEFAULT": {"bg_color": "#FFB74D", "bg_opa": 255, "border_width": 0, "radius": 2}}
        }),
        make_image("temp_icon", 12, 12, 20, 20, "ic_temp"),
        make_label("temp_title", 36, 13, 56, 18, "室内温度", is_var=False, local_def={
            "MAIN": {"DEFAULT": {"text_color": "#9CA3AF", "text_font": "home_cn_12"}}
        }),
        make_label("temp_badge", 96, 13, 38, 16, "home_temp_desc", is_var=True, preview="舒适", local_def={
            "MAIN": {
                "DEFAULT": {
                    "text_color": "#FFB74D",
                    "text_font": "home_cn_12",
                    "text_align": "CENTER",
                    "bg_color": "#3D3226",
                    "bg_opa": 255,
                    "radius": 4
                }
            }
        }),
        make_label("temp_val", 12, 40, 82, 36, "home_temp_val", is_var=True, preview="25.5", local_def={
            "MAIN": {"DEFAULT": {"text_color": "#FFFFFF", "text_font": "home_num_32", "text_align": "LEFT"}}
        }),
        make_label("temp_unit", 96, 52, 32, 18, "°C", is_var=False, local_def={
            "MAIN": {"DEFAULT": {"text_color": "#9CA3AF", "text_font": "home_cn_12"}}
        }),
        make_label("temp_range", 12, 78, 120, 16, "适宜区间 22~26°C", is_var=False, local_def={
            "MAIN": {"DEFAULT": {"text_color": "#6B7280", "text_font": "home_cn_12"}}
        }),
    ]

    # ② 室内湿度卡片 card_humi (164, 8, 144x102)
    card_humi = make_panel("card_humi", 164, 8, 144, 102, use_style="st_home_card")
    card_humi["children"] = [
        make_panel("humi_accent", 0, 14, 3, 16, local_def={
            "MAIN": {"DEFAULT": {"bg_color": "#4FC3F7", "bg_opa": 255, "border_width": 0, "radius": 2}}
        }),
        make_image("humi_icon", 12, 12, 20, 20, "ic_humi"),
        make_label("humi_title", 36, 13, 56, 18, "室内湿度", is_var=False, local_def={
            "MAIN": {"DEFAULT": {"text_color": "#9CA3AF", "text_font": "home_cn_12"}}
        }),
        make_label("humi_badge", 96, 13, 38, 16, "home_humi_desc", is_var=True, preview="适宜", local_def={
            "MAIN": {
                "DEFAULT": {
                    "text_color": "#4FC3F7",
                    "text_font": "home_cn_12",
                    "text_align": "CENTER",
                    "bg_color": "#223847",
                    "bg_opa": 255,
                    "radius": 4
                }
            }
        }),
        make_label("humi_val", 12, 40, 64, 36, "home_humi_val", is_var=True, preview="45", local_def={
            "MAIN": {"DEFAULT": {"text_color": "#FFFFFF", "text_font": "home_num_32", "text_align": "LEFT"}}
        }),
        make_label("humi_unit", 78, 52, 40, 18, "%RH", is_var=False, local_def={
            "MAIN": {"DEFAULT": {"text_color": "#9CA3AF", "text_font": "home_cn_12"}}
        }),
        make_label("humi_range", 12, 78, 120, 16, "适宜区间 40~60%", is_var=False, local_def={
            "MAIN": {"DEFAULT": {"text_color": "#6B7280", "text_font": "home_cn_12"}}
        }),
    ]

    # ③ 天气信息条卡片 card_weather (12, 118, 296x72)
    card_weather = make_panel("card_weather", 12, 118, 296, 72, use_style="st_home_cell", clickable=True)
    card_weather["children"] = [
        make_image("weather_anim_img", 12, 12, 48, 48, "anim_sunny_00"),
        make_label("weather_text", 72, 14, 110, 22, "home_weather_text", is_var=True, preview="晴", local_def={
            "MAIN": {"DEFAULT": {"text_color": "#FFFFFF", "text_font": "home_cn_16", "text_align": "LEFT"}}
        }),
        make_label("env_net_time", 72, 40, 120, 18, "home_net_time", is_var=True, preview="12:34:56", local_def={
            "MAIN": {"DEFAULT": {"text_color": "#9CA3AF", "text_font": "home_cn_12", "text_align": "LEFT"}}
        }),
        make_panel("net_sync_dot", 216, 32, 6, 6, local_def={
            "MAIN": {"DEFAULT": {"bg_color": "#4ADE80", "bg_opa": 255, "border_width": 0, "radius": 3}}
        }),
        make_label("net_sync_text", 226, 27, 56, 16, "home_net_sync", is_var=True, preview="已同步", local_def={
            "MAIN": {"DEFAULT": {"text_color": "#9CA3AF", "text_font": "home_cn_12", "text_align": "LEFT"}}
        }),
    ]

    tile_env = make_panel(
        "tile_env", 0, 0, 320, 208,
        local_def={
            "MAIN": {
                "DEFAULT": {
                    "bg_color": "#20242B",
                    "bg_opa": 255,
                    "border_width": 0,
                    "radius": 0,
                    "pad_top": 0,
                    "pad_bottom": 0,
                    "pad_left": 0,
                    "pad_right": 0
                }
            }
        }
    )
    tile_env["children"] = [card_temp, card_humi, card_weather]

    # 3. 主页2 —— 3x2 应用网格 (挂于 tile_grid)
    tile_grid = make_panel(
        "tile_grid", 320, 0, 320, 208,
        local_def={
            "MAIN": {
                "DEFAULT": {
                    "bg_color": "#20242B",
                    "bg_opa": 255,
                    "border_width": 0,
                    "radius": 0,
                    "pad_top": 0,
                    "pad_bottom": 0,
                    "pad_left": 0,
                    "pad_right": 0
                }
            }
        }
    )
    app_specs = [
        (0, 20,  12,  "ic_app_settings", "设置"),
        (1, 120, 12,  "ic_app_clock",    "时钟"),
        (2, 220, 12,  "ic_app_weather",  "天气"),
        (3, 20,  104, "ic_app_network",  "网络"),
        (4, 120, 104, "ic_app_about",    "关于"),
        (5, 220, 104, "ic_app_theme",    "主题"),
    ]
    for idx, cx, cy, ic_name, title in app_specs:
        cell = make_panel(f"app_cell_{idx}", cx, cy, 80, 80, use_style="st_home_cell", clickable=True)
        box = make_panel(
            f"app_box_{idx}", 18, 8, 44, 44,
            local_def={
                "MAIN": {
                    "DEFAULT": {
                        "bg_color": "#313844",
                        "bg_opa": 255,
                        "border_width": 0,
                        "radius": 10,
                        "pad_top": 0,
                        "pad_bottom": 0,
                        "pad_left": 0,
                        "pad_right": 0
                    }
                }
            }
        )
        box["children"] = [make_image(f"app_ic_{idx}", 10, 10, 24, 24, ic_name)]
        lbl = make_label(
            f"app_name_{idx}", 0, 57, 80, 16, title, is_var=False,
            local_def={
                "MAIN": {
                    "DEFAULT": {
                        "text_color": "#E3E6EA",
                        "text_font": "home_cn_12",
                        "text_align": "CENTER"
                    }
                }
            }
        )
        cell["children"] = [box, lbl]
        tile_grid["children"].append(cell)

    # 4. Tileview 容器 home_tileview (0, 32, 320x208) 改用 LVGLPanelWidget + SCROLL_ONE
    home_tileview = make_base_widget(
        "LVGLPanelWidget",
        "home_tileview",
        0, 32, 320, 208,
        local_def={
            "MAIN": {
                "DEFAULT": {
                    "bg_color": "#20242B",
                    "bg_opa": 255,
                    "border_width": 0,
                    "radius": 0,
                    "pad_top": 0,
                    "pad_bottom": 0,
                    "pad_left": 0,
                    "pad_right": 0
                }
            }
        },
        flags="CLICKABLE|CLICK_FOCUSABLE|GESTURE_BUBBLE|PRESS_LOCK|SCROLLABLE|SCROLL_ELASTIC|SCROLL_MOMENTUM|SCROLL_ONE|SNAPPABLE",
        clickable=True
    )
    home_tileview["flagScrollbarMode"] = "OFF"
    home_tileview["flagScrollDirection"] = "HOR"
    home_tileview["scrollSnapX"] = "CENTER"
    home_tileview["children"] = [tile_env, tile_grid]

    # 5. 底部悬浮指示点 page_dot_0 / page_dot_1 (Z-order 在 home_tileview 之上, y=230)
    page_dot_0 = make_panel(
        "page_dot_0", 149, 230, 12, 6,
        local_def={
            "MAIN": {
                "DEFAULT": {
                    "bg_color": "#E3E6EA",
                    "bg_opa": 255,
                    "border_width": 0,
                    "radius": 3
                }
            }
        },
        clickable=True
    )
    page_dot_1 = make_panel(
        "page_dot_1", 165, 230, 6, 6,
        local_def={
            "MAIN": {
                "DEFAULT": {
                    "bg_color": "#5A6068",
                    "bg_opa": 255,
                    "border_width": 0,
                    "radius": 3
                }
            }
        },
        clickable=True
    )

    # 6. Screen 根节点 (320x240)
    screen_widget = make_base_widget(
        "LVGLScreenWidget",
        None,
        0, 0, 320, 240,
        local_def={
            "MAIN": {
                "DEFAULT": {
                    "bg_color": "#1B1E23",
                    "bg_opa": 255
                }
            }
        },
        flags="CLICKABLE|PRESS_LOCK|CLICK_FOCUSABLE|GESTURE_BUBBLE|SNAPPABLE",
        clickable=True
    )
    screen_widget["children"] = [nav_bar, home_tileview, page_dot_0, page_dot_1]

    return {
        "objID": "4b40eba7-b31a-468a-b887-11dfd0b6637f",
        "components": [screen_widget],
        "connectionLines": [],
        "localVariables": [],
        "componentGroups": [],
        "userProperties": [],
        "name": "Main_Work",
        "left": 0,
        "top": 0,
        "width": 320,
        "height": 240,
        "createAtStart": True,
        "deleteOnScreenUnload": False
    }


def main():
    with open(PROJ_PATH, "r", encoding="utf-8") as f:
        proj = json.load(f)

    # 1. 更新 8 个 Native 全局变量 (第 3.5 节)
    var_specs = [
        ("home_nav_time",     "string", "--:--"),
        ("home_temp_val",     "string", "--.-"),
        ("home_temp_desc",    "string", "--"),
        ("home_humi_val",     "string", "--"),
        ("home_humi_desc",    "string", "--"),
        ("home_weather_text", "string", "--"),
        ("home_net_time",     "string", "--:--:--"),
        ("home_net_sync",     "string", "未同步"),
    ]
    proj["variables"]["globalVariables"] = [
        {
            "objID": new_id(),
            "name": name,
            "type": vtype,
            "defaultValue": json.dumps(def_val, ensure_ascii=False) if vtype == "string" else def_val,
            "description": "",
            "native": True
        }
        for name, vtype, def_val in var_specs
    ]


    # 2. 更新共享样式 st_home_card 与 st_home_cell (第 2.1 & 3.4 节)
    proj["lvglStyles"]["styles"] = [
        {
            "objID": new_id(),
            "name": "st_home_card",
            "definition": {
                "MAIN": {
                    "DEFAULT": {
                        "bg_color": "#262B33",
                        "bg_opa": 255,
                        "border_color": "#3A414C",
                        "border_width": 1,
                        "radius": 12,
                        "shadow_width": 0,
                        "pad_top": 0,
                        "pad_bottom": 0,
                        "pad_left": 0,
                        "pad_right": 0
                    }
                }
            }
        },
        {
            "objID": new_id(),
            "name": "st_home_cell",
            "definition": {
                "MAIN": {
                    "DEFAULT": {
                        "bg_color": "#262B33",
                        "bg_opa": 255,
                        "border_color": "#3A414C",
                        "border_width": 1,
                        "radius": 12,
                        "shadow_width": 0,
                        "pad_top": 0,
                        "pad_bottom": 0,
                        "pad_left": 0,
                        "pad_right": 0
                    },
                    "PRESSED": {
                        "bg_color": "#303742",
                        "bg_opa": 255,
                        "border_color": "#4B5563",
                        "border_width": 1
                    }
                }
            }
        }
    ]

    # 3. 增补 3 套子集字体声明 (保留原 BarlowCondensed-Regular-66)
    existing_fonts = [f for f in proj.get("fonts", []) if f.get("name") == "BarlowCondensed-Regular-66"]
    base_embedded = existing_fonts[0].get("embeddedFontFile", "") if existing_fonts else ""
    new_fonts = [
        {
            "objID": new_id(),
            "name": "home_num_32",
            "renderingEngine": "LVGL",
            "source": {
                "objID": new_id(),
                "filePath": "fonts\\BarlowCondensed-Regular.ttf",
                "size": 32
            },
            "embeddedFontFile": base_embedded,
            "bpp": 4,
            "threshold": 128,
            "height": 33,
            "ascent": 26,
            "descent": 7,
            "glyphs": [],
            "lvglRanges": "",
            "lvglSymbols": "0123456789.-",
            "lvglAdditionalSources": [],
            "lvglGlyphs": {
                "encodings": [],
                "symbols": "0123456789.-"
            }
        },
        {
            "objID": new_id(),
            "name": "home_cn_16",
            "renderingEngine": "LVGL",
            "source": {
                "objID": new_id(),
                "filePath": "fonts\\PingFangSC-Regular.ttf",
                "size": 16
            },
            "bpp": 4,
            "threshold": 128,
            "height": 18,
            "ascent": 14,
            "descent": 4,
            "glyphs": [],
            "lvglRanges": "32-126",
            "lvglSymbols": "晴多云阴小大雨雪雾·°",
            "lvglAdditionalSources": [],
            "lvglGlyphs": {
                "encodings": [{"from": 32, "to": 126}],
                "symbols": "晴多云阴小大雨雪雾·°"
            }
        },
        {
            "objID": new_id(),
            "name": "home_cn_12",
            "renderingEngine": "LVGL",
            "source": {
                "objID": new_id(),
                "filePath": "fonts\\PingFangSC-Regular.ttf",
                "size": 12
            },
            "bpp": 4,
            "threshold": 128,
            "height": 14,
            "ascent": 11,
            "descent": 3,
            "glyphs": [],
            "lvglRanges": "32-126",
            "lvglSymbols": "°设置时钟天气网络关于主题室内温度湿度体感舒适适宜偏热冷炎润潮干已未同步更新周日一二三四五六区间",
            "lvglAdditionalSources": [],
            "lvglGlyphs": {
                "encodings": [{"from": 32, "to": 126}],
                "symbols": "°设置时钟天气网络关于主题室内温度湿度体感舒适适宜偏热冷炎润潮干已未同步更新周日一二三四五六区间"
            }
        }
    ]
    proj["fonts"] = existing_fonts + new_fonts

    # 4. 合成并嵌入全部 40 张静态图标与天气序列帧
    proj["bitmaps"] = prepare_all_images()

    # 5. 替换 userPages[0] (Main_Work)，保留其余 4 个页面
    proj["userPages"][0] = build_main_work_page()

    with open(PROJ_PATH, "w", encoding="utf-8") as f:
        json.dump(proj, f, ensure_ascii=False, indent=2)

    print(f"[OK] 已成功更新 {PROJ_PATH}")
    print(f"     - 全局变量: {len(proj['variables']['globalVariables'])} 个")
    print(f"     - 共享样式: {len(proj['lvglStyles']['styles'])} 个")
    print(f"     - 字体数量: {len(proj['fonts'])} 套")
    print(f"     - 图片/序列帧: {len(proj['bitmaps'])} 张")


if __name__ == "__main__":
    main()