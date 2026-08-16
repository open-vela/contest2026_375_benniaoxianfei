#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gen_model_data.py —— 从 INT8 tflite 生成板端 C 数组 model_data.cc
=================================================================
用法:
    python gen_model_data.py [模型文件]
    # 默认: 模型/gesture_cnn_qat_int8.tflite（若不存在则用 gesture_cnn_int8.tflite）

输出: app/sign_translate/model_data.cc
     alignas(16) const unsigned char g_sign_model[] = {...};
     const unsigned int g_sign_model_len = ...;
"""

import os
import sys

sys.stdout.reconfigure(encoding="utf-8")

BASE = os.path.dirname(os.path.abspath(__file__))
MODEL_DIR = os.path.join(BASE, "模型")
OUT = os.path.join(BASE, "..", "app", "sign_translate", "model_data.cc")

candidates = [
    os.path.join(MODEL_DIR, "gesture_cnn_qat_int8.tflite"),
    os.path.join(MODEL_DIR, "gesture_cnn_int8.tflite"),
]
if len(sys.argv) > 1:
    candidates.insert(0, sys.argv[1])

model_path = next((p for p in candidates if os.path.exists(p)), None)
if model_path is None:
    print("❌ 找不到模型:", candidates)
    sys.exit(1)

with open(model_path, "rb") as f:
    data = f.read()

print(f"模型: {os.path.basename(model_path)}  ({len(data)} bytes = {len(data)/1024:.1f} KB)")

lines = []
lines.append("/****************************************************************************")
lines.append(" * model_data.cc —— INT8 模型 C 数组（由 gen_model_data.py 自动生成）")
lines.append(" * 来源: %s" % os.path.basename(model_path))
lines.append(" ****************************************************************************/")
lines.append("")
lines.append("#include <cstdint>")
lines.append("")
lines.append("alignas(16) const unsigned char g_sign_model[] = {")
for i in range(0, len(data), 12):
    chunk = data[i:i + 12]
    hexs = ", ".join("0x%02x" % b for b in chunk)
    lines.append("  %s," % hexs)
lines.append("};")
lines.append("")
lines.append("const unsigned int g_sign_model_len = %d;" % len(data))
lines.append("")

os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))

print(f"✅ 已生成: {OUT}")
print(f"   共 {len(lines)} 行, {len(data)} 字节")
