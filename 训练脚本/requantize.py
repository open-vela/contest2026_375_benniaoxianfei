#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""重新量化 INT8（用更充分的校准集，修复量化失真）"""
import os
import sys

import numpy as np
import tensorflow as tf
from tensorflow import keras

sys.stdout.reconfigure(encoding="utf-8")
os.environ["TF_CPP_MIN_LOG_LEVEL"] = "3"

BASE = r"E:\Desktop\Openvela\数据集"
OUT = r"E:\Desktop\Openvela\训练脚本\模型"
IMG = 128

model = keras.models.load_model(os.path.join(OUT, "gesture_cnn.h5"))

# 校准集：验证集全部 + 训练集抽样（覆盖更多多样性）
# 注意：校准输入范围必须与运行时推理一致 —— 喂 [0,255] 原始像素（不归一化）
def rep_data():
    for src, batches in [(os.path.join(BASE, "val"), 12),
                         (os.path.join(BASE, "train"), 6)]:
        ds = keras.utils.image_dataset_from_directory(
            src, image_size=(IMG, IMG), batch_size=32,
            label_mode=None, shuffle=True, seed=1)
        # 不除以 255：量化器据此计算输入 scale/zero_point，
        # 与板端/infer_test 直接喂 0-255 像素一致
        for x in ds.take(batches):
            yield [tf.cast(x, tf.float32)]

converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = rep_data
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.uint8
converter.inference_output_type = tf.uint8
tflite_int8 = converter.convert()

out_path = os.path.join(OUT, "gesture_cnn_int8.tflite")
with open(out_path, "wb") as f:
    f.write(tflite_int8)
sz = os.path.getsize(out_path)
print(f"重新量化完成: gesture_cnn_int8.tflite  {sz} bytes ({sz/1024:.1f} KB)")
