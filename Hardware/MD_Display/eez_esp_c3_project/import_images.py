#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
将 Image/ 目录中根据 Docs/home_ui_card_design_v4.2.html 提取的图片素材同步导入 esp_c3.eez-project。

功能特性:
1. 完整覆盖《home_ui_card_design_v4.2》设计规范中的 72 张位图素材:
   - 8 张静态图标 (bpp 32):
     * 2 张传感器图标: ic_temp (20x20), ic_humi (20x20)
     * 6 张应用图标: ic_app_settings, ic_app_clock, ic_app_weather,
                     ic_app_network, ic_app_about, ic_app_theme (24x24)
   - 32 张白天天气序列帧 (4 态 x 8 帧, 48x48, bpp 16):
     * anim_sunny_00 ~ anim_sunny_07 (sunny/)
     * anim_cloudy_00 ~ anim_cloudy_07 (cloudy/)
     * anim_overcast_00 ~ anim_overcast_07 (overcast/)
     * anim_rain_00 ~ anim_rain_07 (rain/)
   - 32 张夜晚天气序列帧 (新增 4 态 x 8 帧, 48x48, bpp 16):
     * anim_night_sunny_00 ~ anim_night_sunny_07 (night_sunny/)
     * anim_night_cloudy_00 ~ anim_night_cloudy_07 (night_cloudy/)
     * anim_night_overcast_00 ~ anim_night_overcast_07 (night_overcast/)
     * anim_night_rain_00 ~ anim_night_rain_07 (night_rain/)
2. 保持工程中既有 40 张位图的 objID 不变，避免既有页面与控件引用失效;
3. 自动生成新位图的唯一 objID (UUID v4);
4. 清除旧版本冗余位图 (如临时测试的 anim_rain_08~11 与 ic_app_*_box);
5. 自动备份原工程为 esp_c3.eez-project.bak。

运行方式: 在 eez_esp_c3_project 目录下执行:
    python import_images.py
"""

import os
import csv
import json
import uuid
import base64
import shutil

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
IMG_DIR = os.path.join(BASE_DIR, "Image")
PROJ_PATH = os.path.join(BASE_DIR, "esp_c3.eez-project")
BACKUP_PATH = os.path.join(BASE_DIR, "esp_c3.eez-project.bak")
MANIFEST_PATH = os.path.join(IMG_DIR, "_manifest.csv")

# 规范位图导入顺序 (静态图标 -> 白天四态 -> 夜晚四态)
ORDER = [
    "ic_temp",
    "ic_humi",
    "ic_app_settings",
    "ic_app_clock",
    "ic_app_weather",
    "ic_app_network",
    "ic_app_about",
    "ic_app_theme",
]

WEATHER_GROUPS = [
    "anim_sunny",
    "anim_cloudy",
    "anim_overcast",
    "anim_rain",
    "anim_night_sunny",
    "anim_night_cloudy",
    "anim_night_overcast",
    "anim_night_rain",
]

for prefix in WEATHER_GROUPS:
    for i in range(8):
        ORDER.append(f"{prefix}_{i:02d}")


def img_to_data_uri(img_path: str) -> str:
    """读取图片二进制并转为 base64 data URI"""
    with open(img_path, "rb") as f:
        b64 = base64.b64encode(f.read()).decode("ascii")
    return "data:image/png;base64," + b64


def main():
    if not os.path.exists(PROJ_PATH):
        print(f"[错误] 未找到工程文件: {PROJ_PATH}")
        return

    if not os.path.exists(MANIFEST_PATH):
        print(f"[错误] 未找到素材清单: {MANIFEST_PATH}")
        return

    # 1. 读取既有工程并建立 name -> bitmap 索引
    with open(PROJ_PATH, "r", encoding="utf-8") as f:
        proj = json.load(f)

    existing_bitmaps = {b["name"]: b for b in proj.get("bitmaps", [])}

    # 2. 读取素材清单
    with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
        manifest_rows = list(csv.reader(f))[1:]

    # 建立素材名 -> (相对路径, 类型, 尺寸) 映射
    manifest_map = {}
    for r in manifest_rows:
        rel = r[6]
        if rel.endswith(".gif"):
            continue
        bmp_name = os.path.splitext(os.path.basename(rel))[0]
        manifest_map[bmp_name] = {
            "rel_path": rel,
            "type_str": r[3],
            "dimension": r[4],
        }

    # 3. 按规范顺序构建位图列表
    new_bitmaps = []
    updated_names = []
    added_names = []
    missing_names = []

    for name in ORDER:
        if name not in manifest_map:
            missing_names.append(name)
            continue

        info = manifest_map[name]
        full_path = os.path.join(IMG_DIR, info["rel_path"])
        if not os.path.exists(full_path):
            missing_names.append(name)
            continue

        uri = img_to_data_uri(full_path)
        bpp = 32 if info["type_str"] == "静态" else 16

        # 复用已有 objID 或创建新 objID
        if name in existing_bitmaps:
            obj_id = existing_bitmaps[name]["objID"]
            updated_names.append(name)
        else:
            obj_id = str(uuid.uuid4())
            added_names.append(name)

        new_bitmaps.append({
            "objID": obj_id,
            "name": name,
            "image": uri,
            "bpp": bpp,
            "style": "default",
        })

    # 统计被清理的冗余位图
    removed_names = [name for name in existing_bitmaps if name not in ORDER]

    # 4. 备份原工程
    shutil.copy2(PROJ_PATH, BACKUP_PATH)
    print(f"[备份] 已将原工程备份至: {BACKUP_PATH}")

    # 5. 写回工程
    proj["bitmaps"] = new_bitmaps
    with open(PROJ_PATH, "w", encoding="utf-8") as f:
        json.dump(proj, f, ensure_ascii=False, indent=2)

    print(f"[成功] 已同步更新 EEZ 工程: {PROJ_PATH}")
    print(f"       位图总数: {len(new_bitmaps)} 张 (静态图标 8 + 白天动效 32 + 夜晚动效 32)")
    print(f"       更新已有位图: {len(updated_names)} 张")
    print(f"       新增夜晚位图: {len(added_names)} 张")
    if removed_names:
        print(f"       清理冗余位图: {len(removed_names)} 张 ({', '.join(removed_names)})")
    if missing_names:
        print(f"       [警告] 缺失素材: {', '.join(missing_names)}")


if __name__ == "__main__":
    main()
