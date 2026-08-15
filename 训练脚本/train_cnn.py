#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
train_cnn.py —— 手语 12 类轻量 CNN 训练 + INT8 量化
=====================================================
用法:
    pip install tensorflow
    python train_cnn.py

输入: 数据集/train/  数据集/val/   (由 prepare_dataset.py 生成)
输出:
    模型/gesture_cnn.h5          浮点模型
    模型/gesture_cnn.tflite      浮点 TFLite
    模型/gesture_cnn_int8.tflite INT8 量化 TFLite（烧进板子的目标）
    模型/gesture_labels.txt      类别顺序

目标: 模型 < 300KB, 准确率 >= 90%, 板端推理 < 300ms
"""

import os
import numpy as np
import tensorflow as tf
from tensorflow import keras

BASE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "数据集")
TRAIN = os.path.join(BASE, "train")
VAL = os.path.join(BASE, "val")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "模型")
IMG_SIZE = 128
BATCH = 32
EPOCHS = 40

os.makedirs(OUT, exist_ok=True)

print("TensorFlow:", tf.__version__)

# ---------- 数据 ----------
train_ds = keras.utils.image_dataset_from_directory(
    TRAIN, image_size=(IMG_SIZE, IMG_SIZE), batch_size=BATCH,
    label_mode="categorical", shuffle=True, seed=42)
val_ds = keras.utils.image_dataset_from_directory(
    VAL, image_size=(IMG_SIZE, IMG_SIZE), batch_size=BATCH,
    label_mode="categorical", shuffle=False)

class_names = train_ds.class_names
num_classes = len(class_names)
print("类别:", class_names)
with open(os.path.join(OUT, "gesture_labels.txt"), "w", encoding="utf-8") as f:
    for c in class_names:
        f.write(c + "\n")

# 归一化 + 数据增强（训练时）
aug = keras.Sequential([
    keras.layers.RandomFlip("horizontal"),
    keras.layers.RandomRotation(0.1),
    keras.layers.RandomContrast(0.1),
])
train_ds = train_ds.map(lambda x, y: (aug(x / 255.0), y))
val_ds = val_ds.map(lambda x, y: (x / 255.0, y))
train_ds = train_ds.prefetch(tf.data.AUTOTUNE)
val_ds = val_ds.prefetch(tf.data.AUTOTUNE)

# ---------- 轻量 CNN（目标 <300KB INT8） ----------
model = keras.Sequential([
    keras.layers.Input((IMG_SIZE, IMG_SIZE, 3)),
    keras.layers.Conv2D(16, 3, strides=2, padding="same", activation="relu"),
    keras.layers.Conv2D(16, 3, padding="same", activation="relu"),
    keras.layers.MaxPool2D(2),
    keras.layers.Conv2D(32, 3, padding="same", activation="relu"),
    keras.layers.Conv2D(32, 3, padding="same", activation="relu"),
    keras.layers.MaxPool2D(2),
    keras.layers.Conv2D(64, 3, padding="same", activation="relu"),
    keras.layers.MaxPool2D(2),
    keras.layers.Flatten(),
    keras.layers.Dropout(0.3),
    keras.layers.Dense(64, activation="relu"),
    keras.layers.Dense(num_classes, activation="softmax"),
])
model.summary()

model.compile(optimizer=keras.optimizers.Adam(1e-3),
              loss="categorical_crossentropy",
              metrics=["accuracy"])

callbacks = [
    keras.callbacks.ModelCheckpoint(os.path.join(OUT, "gesture_cnn.h5"),
                                    save_best_only=True, monitor="val_accuracy"),
    keras.callbacks.ReduceLROnPlateau(patience=4, factor=0.5),
    keras.callbacks.EarlyStopping(patience=8, restore_best_weights=True),
]

model.fit(train_ds, validation_data=val_ds, epochs=EPOCHS, callbacks=callbacks)

# ---------- 评估 ----------
model = keras.models.load_model(os.path.join(OUT, "gesture_cnn.h5"))
loss, acc = model.evaluate(val_ds)
print(f"\n验证集准确率: {acc:.3f} ({acc*100:.1f}%)")

# ---------- 转 TFLite ----------
converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()
with open(os.path.join(OUT, "gesture_cnn.tflite"), "wb") as f:
    f.write(tflite_model)
print("浮点 tflite:", os.path.getsize(os.path.join(OUT, "gesture_cnn.tflite")), "bytes")

# ---------- INT8 量化（PTQ） ----------
def rep_data():
    """代表性数据集：从验证集取 100 张做校准"""
    ds = val_ds.unbatch().take(100)
    for x, _ in ds:
        yield [tf.reshape(x, (1, IMG_SIZE, IMG_SIZE, 3))]

converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = rep_data
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.uint8
converter.inference_output_type = tf.uint8
try:
    int8_model = converter.convert()
    with open(os.path.join(OUT, "gesture_cnn_int8.tflite"), "wb") as f:
        f.write(int8_model)
    sz = os.path.getsize(os.path.join(OUT, "gesture_cnn_int8.tflite"))
    print(f"INT8 tflite: {sz} bytes ({sz/1024:.1f} KB)")
    if sz > 400 * 1024:
        print("⚠️ 超过 400KB！建议：输入降到 96x96 或减少卷积层")
except Exception as e:
    print("INT8 量化失败:", e)
    print("提示: INT8 需要代表性数据集与全整数量化支持；失败时先用浮点 tflite 部署")

print("\n下一步:")
print("  1. xxd -i gesture_cnn_int8.tflite > model_data.cc   (转 C 数组)")
print("  2. 把 model_data.cc 放进 OpenVela 应用，用 TFLite Micro 加载")
print("  3. 板端推理延迟目标 < 300ms")
