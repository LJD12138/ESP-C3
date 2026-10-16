import json
import shutil
import sys
from pathlib import Path

PROJECT_FILE = sys.argv[1] if len(sys.argv) > 1 else "esp_c3.eez-project"

# True  = 640x240 双页同屏编辑模式（在 EEZ 画布中同时并排显示 主页1 + 主页2）
# False = 320x240 标准实机导出模式（Build 导出 C 代码烧录前请改为 False 运行一次）
SIDE_BY_SIDE_EDIT_MODE = True


def fix_project(file_path: str):
    path = Path(file_path)
    if not path.exists():
        print(f"未找到文件: {path.resolve()}")
        return

    backup_path = path.with_suffix(path.suffix + ".bak")
    shutil.copy2(path, backup_path)
    print(f"已备份原工程至: {backup_path.name}")

    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)

    target_w = 640 if SIDE_BY_SIDE_EDIT_MODE else 320

    # 1. 设置工程总分辨率宽度
    data["settings"]["general"]["displayWidth"] = target_w
    data["settings"]["general"]["displayHeight"] = 240

    # 2. 设置 Main_Work 页面及根屏幕 LVGLScreenWidget 的宽度为 target_w (640 或 320)
    for page in data.get("userPages", []):
        if page.get("name") == "Main_Work":
            page["width"] = target_w
            page["height"] = 240
            screen = page["components"][0]
            screen["width"] = target_w
            screen["height"] = 240
            screen["localStyles"]["definition"]["MAIN"]["DEFAULT"] = {
                "bg_color": "#0D1117",
                "bg_opa": 255
            }

    # 卡片通用局部样式（直接写入每个卡片的 localStyles，清零默认 Padding）
    card_local_style_main = {
        "bg_color": "#262B33",
        "bg_opa": 255,
        "border_color": "#3A4454",
        "border_width": 1,
        "radius": 12,
        "pad_top": 0,
        "pad_bottom": 0,
        "pad_left": 0,
        "pad_right": 0,
        "shadow_width": 0
    }
    card_local_style_pressed = {
        "bg_color": "#303742",
        "border_color": "#4B5563"
    }

    # 3. 控件坐标与局部样式修正表
    widget_patches = {
        "nav_bar": {
            "left": 0, "top": 0, "width": 320, "height": 32,
            "style_main": {
                "bg_color": "#161B22", "bg_opa": 255,
                "border_color": "#30363D", "border_width": 1, "border_side": "BOTTOM",
                "radius": 0, "pad_top": 0, "pad_bottom": 0, "pad_left": 0, "pad_right": 0
            }
        },
        "nav_time": {
            "left": 12, "top": 7, "width": 52, "height": 18,
            "style_main": {"text_color": "#FFFFFF", "text_font": "home_cn_16", "text_align": "LEFT"}
        },
        "nav_date": {
            "left": 64, "top": 9, "width": 72, "height": 16,
            "style_main": {"text_color": "#B0B8C4", "text_font": "home_cn_12", "text_align": "LEFT"}
        },
        # 关键：将 home_tileview 宽度同步设为 target_w (640)，解除 320px 裁剪
        "home_tileview": {
            "left": 0, "top": 32, "width": target_w, "height": 208,
            "style_main": {
                "bg_color": "#0D1117", "bg_opa": 255,
                "border_width": 0, "radius": 0,
                "pad_top": 0, "pad_bottom": 0, "pad_left": 0, "pad_right": 0
            }
        },
        "tile_env": {
            "left": 0, "top": 0, "width": 320, "height": 208,
            "add_flags": ["SCROLL_CHAIN_HOR"],
            "style_main": {
                "bg_color": "#0D1117", "bg_opa": 255,
                "border_width": 0, "radius": 0,
                "pad_top": 0, "pad_bottom": 0, "pad_left": 0, "pad_right": 0
            }
        },
        "tile_grid": {
            "left": 320, "top": 0, "width": 320, "height": 208,
            "add_flags": ["SCROLL_CHAIN_HOR"],
            "style_main": {
                "bg_color": "#0D1117", "bg_opa": 255,
                "border_width": 0, "radius": 0,
                "pad_top": 0, "pad_bottom": 0, "pad_left": 0, "pad_right": 0
            }
        },
        # 温度卡片及子控件（直接写入 card_local_style_main 清零 Padding）
        "card_temp": {
            "left": 12, "top": 8, "width": 144, "height": 102,
            "add_flags": ["SCROLL_CHAIN_HOR"],
            "style_main": card_local_style_main
        },
        "temp_accent": {"left": 0, "top": 14, "width": 3, "height": 16},
        "temp_icon":   {"left": 12, "top": 12, "width": 20, "height": 20},
        "temp_title":  {
            "left": 36, "top": 14, "width": 54, "height": 16,
            "style_main": {"text_color": "#B0B8C4", "text_font": "home_cn_12"}
        },
        "temp_badge":  {
            "left": 94, "top": 13, "width": 38, "height": 18,
            "style_main": {
                "text_color": "#FFB74D", "text_font": "home_cn_12", "text_align": "CENTER",
                "bg_color": "#3D3226", "bg_opa": 255, "radius": 4,
                "pad_top": 2, "pad_bottom": 0, "pad_left": 0, "pad_right": 0
            }
        },
        "temp_val":    {"left": 12, "top": 38, "width": 62, "height": 36},
        "temp_unit":   {
            "left": 74, "top": 52, "width": 28, "height": 16,
            "style_main": {"text_color": "#B0B8C4", "text_font": "home_cn_12"}
        },
        "temp_range":  {
            "left": 12, "top": 78, "width": 124, "height": 16,
            "style_main": {"text_color": "#8B95A5", "text_font": "home_cn_12"}
        },
        # 湿度卡片及子控件（直接写入 card_local_style_main 清零 Padding）
        "card_humi": {
            "left": 164, "top": 8, "width": 144, "height": 102,
            "add_flags": ["SCROLL_CHAIN_HOR"],
            "style_main": card_local_style_main
        },
        "humi_accent": {"left": 0, "top": 14, "width": 3, "height": 16},
        "humi_icon":   {"left": 12, "top": 12, "width": 20, "height": 20},
        "humi_title":  {
            "left": 36, "top": 14, "width": 54, "height": 16,
            "style_main": {"text_color": "#B0B8C4", "text_font": "home_cn_12"}
        },
        "humi_badge":  {
            "left": 94, "top": 13, "width": 38, "height": 18,
            "style_main": {
                "text_color": "#4FC3F7", "text_font": "home_cn_12", "text_align": "CENTER",
                "bg_color": "#223847", "bg_opa": 255, "radius": 4,
                "pad_top": 2, "pad_bottom": 0, "pad_left": 0, "pad_right": 0
            }
        },
        "humi_val":    {"left": 12, "top": 38, "width": 40, "height": 36},
        "humi_unit":   {
            "left": 54, "top": 52, "width": 38, "height": 16,
            "style_main": {"text_color": "#B0B8C4", "text_font": "home_cn_12"}
        },
        "humi_range":  {
            "left": 12, "top": 78, "width": 124, "height": 16,
            "style_main": {"text_color": "#8B95A5", "text_font": "home_cn_12"}
        },
        # 天气卡片及子控件（直接写入 card_local_style_main 清零 Padding）
        "card_weather": {
            "left": 12, "top": 118, "width": 296, "height": 72,
            "add_flags": ["SCROLL_CHAIN_HOR"],
            "style_main": card_local_style_main,
            "style_pressed": card_local_style_pressed
        },
        "weather_anim_img": {"left": 12, "top": 12, "width": 48, "height": 48},
        "weather_text":     {"left": 72, "top": 14, "width": 110, "height": 22},
        "env_net_time":     {
            "left": 72, "top": 42, "width": 120, "height": 16,
            "style_main": {"text_color": "#B0B8C4", "text_font": "home_cn_12", "text_align": "LEFT"}
        },
        "net_sync_dot":     {"left": 216, "top": 33, "width": 6, "height": 6},
        "net_sync_text":    {
            "left": 226, "top": 29, "width": 56, "height": 16,
            "style_main": {"text_color": "#E3E6EA", "text_font": "home_cn_12", "text_align": "LEFT"}
        },
    }

    # 为主页2 的 6 个应用格子统一写入 localStyles（清零 Padding + 设置圆角卡片背景）
    for i in range(6):
        widget_patches[f"app_cell_{i}"] = {
            "add_flags": ["SCROLL_CHAIN_HOR"],
            "style_main": card_local_style_main,
            "style_pressed": card_local_style_pressed
        }

    def traverse_and_patch(node):
        if isinstance(node, dict):
            ident = node.get("identifier")
            if ident in widget_patches:
                patch = widget_patches[ident]
                for k in ("left", "top", "width", "height"):
                    if k in patch:
                        node[k] = patch[k]
                if "add_flags" in patch:
                    flags = [f for f in node.get("widgetFlags", "").split("|") if f]
                    for nf in patch["add_flags"]:
                        if nf not in flags:
                            flags.append(nf)
                    node["widgetFlags"] = "|".join(flags)
                if "style_main" in patch:
                    local_def = node.setdefault("localStyles", {}).setdefault("definition", {})
                    main_part = local_def.setdefault("MAIN", {})
                    main_part.setdefault("DEFAULT", {}).update(patch["style_main"])
                    if "style_pressed" in patch:
                        main_part.setdefault("PRESSED", {}).update(patch["style_pressed"])

            for v in node.values():
                traverse_and_patch(v)
        elif isinstance(node, list):
            for item in node:
                traverse_and_patch(item)

    traverse_and_patch(data.get("userPages", []))

    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)

    mode_str = "640x240 双页并排预览模式" if SIDE_BY_SIDE_EDIT_MODE else "320x240 标准实机模式"
    print(f"修复完成！当前模式: [{mode_str}] -> 已更新: {path.name}")


if __name__ == "__main__":
    fix_project(PROJECT_FILE)