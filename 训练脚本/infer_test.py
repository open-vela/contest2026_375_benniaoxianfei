#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
infer_test.py —— 用 INT8 模型验证单张手势图片
================================================
用法:
    python infer_test.py <图片路径> [图片路径2 ...]

示例:
    python infer_test.py 测试图_数字1.jpg
    python infer_test.py 测试图_谢谢.jpg 测试图_数字1.jpg

说明:
    - 用训练出的 gesture_cnn_int8.tflite 推理
    - 图片任意大小/格式（自动缩放 128x128）
    - 输出: 预测类别 + 置信度 + 各类概率
    - 和板端 TFLite Micro 走完全相同的量化流程
"""

import sys
import os
import numpy as np
import tensorflow as tf

sys.stdout.reconfigure(encoding="utf-8")  # Windows 控制台中文显示

BASE = os.path.dirname(os.path.abspath(__file__))
MODEL = os.path.join(BASE, "模型", "gesture_cnn_int8.tflite")
LABELS = os.path.join(BASE, "模型", "gesture_labels.txt")
IMG_SIZE = 128


def load_labels(path):
    with open(path, encoding="utf-8") as f:
        return [line.strip() for line in f if line.strip()]


def load_image(path):
    """读图（支持中文路径）→ 128x128 RGB uint8"""
    with open(path, "rb") as f:
        data = f.read()
    img = tf.image.decode_image(data, channels=3)
    img = tf.image.resize(img, [IMG_SIZE, IMG_SIZE])
    return tf.cast(img, tf.uint8).numpy()


def predict(interpreter, img, labels):
    in_t = interpreter.get_input_details()[0]
    out_t = interpreter.get_output_details()[0]

    # 输入量化（与板端一致；按张量实际类型 UINT8/INT8）
    scale, zp = in_t["quantization_parameters"]["scales"][0], \
                in_t["quantization_parameters"]["zero_points"][0]
    q = img.astype(np.float32) / scale + zp
    if in_t["dtype"] == np.uint8:
        q = np.clip(np.round(q), 0, 255).astype(np.uint8)
    else:
        q = np.clip(np.round(q), -128, 127).astype(np.int8)

    interpreter.set_tensor(in_t["index"], q[None, ...])
    interpreter.invoke()

    out = interpreter.get_tensor(out_t["index"])[0]
    o_scale, o_zp = out_t["quantization_parameters"]["scales"][0], \
                    out_t["quantization_parameters"]["zero_points"][0]
    probs = (out.astype(np.float32) - o_zp) * o_scale
    probs = np.clip(probs, 0, None)
    total = probs.sum()
    if total > 0:
        probs = probs / total

    best = int(np.argmax(probs))
    return best, probs


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    labels = load_labels(LABELS)
    with open(MODEL, "rb") as f:
        model_content = f.read()
    interp = tf.lite.Interpreter(model_content=model_content)
    interp.allocate_tensors()

    print(f"模型: {os.path.basename(MODEL)}  类别: {labels}")
    print("=" * 50)

    for path in sys.argv[1:]:
        if not os.path.exists(path):
            print(f"❌ 找不到: {path}")
            continue
        img = load_image(path)
        best, probs = predict(interp, img, labels)
        print(f"\n📷 {os.path.basename(path)}")
        print(f"   → 预测: **{labels[best]}**  置信度 {probs[best]*100:.1f}%")
        for i, p in enumerate(probs):
            bar = "#" * int(p * 30)
            print(f"      {labels[i]:<8} {p*100:5.1f}%  {bar}")


if __name__ == "__main__":
    main()
