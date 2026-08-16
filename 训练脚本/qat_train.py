#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
qat_train.py —— 量化感知训练（QAT）+ 输入范围统一修复
=====================================================
改进点：
1. 模型第一层加 Rescaling(1/255)：输入为原始像素 [0,255]，与板端/推理一致
   （修复之前"训练用[0,1]、推理喂[0,255]"的错位）
2. 先用 PTQ 量化对比，再做 QAT 微调，输出三档准确率对比

用法:
    pip install tensorflow-model-optimization
    python qat_train.py
"""

import os

# tfmot 需要 Keras 2（tf_keras）；必须在 import tensorflow 之前设置
os.environ["TF_USE_LEGACY_KERAS"] = "1"

import sys

import numpy as np
import tensorflow as tf
from tensorflow import keras   # tf_keras (Keras 2)：模型/QAT 用
import keras as K3             # Keras 3：数据加载用（其 loader 兼容中文路径）

sys.stdout.reconfigure(encoding="utf-8")
os.environ["TF_CPP_MIN_LOG_LEVEL"] = "2"

BASE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "数据集")
TRAIN = os.path.join(BASE, "train")
VAL = os.path.join(BASE, "val")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "模型")
IMG_SIZE = 128
BATCH = 32
EPOCHS = 40          # 主训练
QAT_EPOCHS = 6       # QAT 微调

os.makedirs(OUT, exist_ok=True)


def build_model(num_classes):
    """轻量 CNN：第一层 Rescaling 把 [0,255] 像素归一化到 [0,1]"""
    return keras.Sequential([
        keras.layers.Input((IMG_SIZE, IMG_SIZE, 3)),
        keras.layers.Rescaling(1.0 / 255.0),          # 输入统一为原始像素
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


def load_ds(path, shuffle, augment=False):
    ds = K3.utils.image_dataset_from_directory(
        path, image_size=(IMG_SIZE, IMG_SIZE), batch_size=BATCH,
        label_mode="categorical", shuffle=shuffle, seed=42)
    if augment:
        aug = keras.Sequential([
            keras.layers.RandomRotation(0.1),
            keras.layers.RandomContrast(0.1),
        ])
        ds = ds.map(lambda x, y: (aug(x), y))
    return ds  # 不再 /255（模型内部 Rescaling 处理）


def dump_tensor_info(path, tag):
    """打印转换后模型的输入/输出张量参数，验证与板端一致性"""
    interp = tf.lite.Interpreter(model_content=open(path, "rb").read())
    interp.allocate_tensors()
    for name, det in (("输入", interp.get_input_details()[0]),
                      ("输出", interp.get_output_details()[0])):
        dtype = "uint8" if det["dtype"] == np.uint8 else \
                ("int8" if det["dtype"] == np.int8 else str(det["dtype"]))
        print(f"  {tag} {name}: dtype={dtype} "
              f"scale={det['quantization_parameters']['scales'][0]:.6f} "
              f"zero_point={det['quantization_parameters']['zero_points'][0]}")


def eval_acc(model, ds):
    loss, acc = model.evaluate(ds, verbose=0)
    return acc * 100


def to_int8(model, rep_ds, out_path):
    """PTQ INT8 转换（校准集喂原始像素 [0,255]）"""
    def rep_data():
        for x, _ in rep_ds.unbatch().take(200):
            yield [tf.reshape(x, (1, IMG_SIZE, IMG_SIZE, 3))]
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = rep_data
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.uint8
    converter.inference_output_type = tf.uint8
    tflite = converter.convert()
    with open(out_path, "wb") as f:
        f.write(tflite)
    return os.path.getsize(out_path)


def eval_tflite_int8(path, ds):
    """用 INT8 tflite 在数据集上评估（与板端同路径：喂原始 [0,255] 像素）"""
    interp = tf.lite.Interpreter(model_content=open(path, "rb").read())
    interp.allocate_tensors()
    in_t = interp.get_input_details()[0]
    out_t = interp.get_output_details()[0]
    iq = in_t["quantization_parameters"]
    oq = out_t["quantization_parameters"]
    scale, zp = iq["scales"][0], iq["zero_points"][0]
    o_scale, o_zp = oq["scales"][0], oq["zero_points"][0]
    correct = total = 0
    for x, y in ds.unbatch():
        if in_t["dtype"] == np.uint8:
            q = np.clip(np.round(x.numpy().astype(np.float32) / scale + zp), 0, 255).astype(np.uint8)
        else:
            q = np.clip(np.round(x.numpy().astype(np.float32) / scale + zp), -128, 127).astype(np.int8)
        interp.set_tensor(in_t["index"], q[None, ...])
        interp.invoke()
        out = interp.get_tensor(out_t["index"])[0]
        if out_t["dtype"] == np.float32:
            probs = out
        else:
            probs = (out.astype(np.float32) - o_zp) * o_scale
        pred = int(np.argmax(probs))
        if pred == int(np.argmax(y.numpy())):
            correct += 1
        total += 1
    return correct / total * 100 if total else 0


def main():
    print("加载数据...")
    train_ds = load_ds(TRAIN, shuffle=True, augment=True)
    val_ds = load_ds(VAL, shuffle=False)
    names = val_ds.class_names
    print(f"类别({len(names)}): {names}")
    with open(os.path.join(OUT, "gesture_labels.txt"), "w", encoding="utf-8") as f:
        for c in names:
            f.write(c + "\n")
    train_ds = train_ds.prefetch(tf.data.AUTOTUNE)
    val_ds = val_ds.prefetch(tf.data.AUTOTUNE)

    # 1) 训练（含 Rescaling 层的新架构）
    print("训练主模型...")
    model = build_model(len(names))
    model.compile(optimizer=keras.optimizers.Adam(1e-3),
                  loss="categorical_crossentropy", metrics=["accuracy"])
    model.fit(train_ds, validation_data=val_ds, epochs=EPOCHS, verbose=0,
              callbacks=[keras.callbacks.ReduceLROnPlateau(patience=4, factor=0.5),
                         keras.callbacks.EarlyStopping(patience=8, restore_best_weights=True)])
    model.save(os.path.join(OUT, "gesture_cnn.h5"))
    acc_float = eval_acc(model, val_ds)
    print(f"① 浮点模型验证集: {acc_float:.1f}%")

    # 2) PTQ INT8（校准 [0,255] 原始像素）
    ptq_path = os.path.join(OUT, "gesture_cnn_int8.tflite")
    sz = to_int8(model, val_ds, ptq_path)
    acc_ptq = eval_tflite_int8(ptq_path, val_ds)
    print(f"② PTQ INT8 验证集: {acc_ptq:.1f}%  (模型 {sz/1024:.0f}KB)")
    dump_tensor_info(ptq_path, "PTQ")

    # 3) QAT 微调
    try:
        import tensorflow_model_optimization as tfmot

        # tfmot 默认量化注册表不认识 Rescaling：给它一个"不量化"配置，
        # 让该层直通（转换后作为 float 岛保留，其余层全 int8）
        class NoOpQuantizeConfig(tfmot.quantization.keras.QuantizeConfig):
            def get_weights_and_quantizers(self, layer):
                return []

            def get_activations_and_quantizers(self, layer):
                return []

            def set_quantize_weights(self, layer, quantize_weights):
                pass

            def set_quantize_activations(self, layer, quantize_activations):
                pass

            def get_output_quantizers(self, layer):
                return []

            def get_config(self):
                return {}

        from tensorflow_model_optimization.python.core.quantization.keras.quantize_annotate import (
            QuantizeAnnotate)

        with tfmot.quantization.keras.quantize_scope():
            annotated = tfmot.quantization.keras.quantize_annotate_model(model)
            # 把 Rescaling 层的量化配置替换为"不量化"（该层直通，其余层全 int8）
            for layer in annotated.layers:
                if isinstance(layer, QuantizeAnnotate) and \
                   layer.layer.__class__.__name__ == "Rescaling":
                    layer.quantize_config = NoOpQuantizeConfig()
            q_model = tfmot.quantization.keras.quantize_apply(annotated)
        q_model.compile(optimizer=keras.optimizers.Adam(1e-4),
                        loss="categorical_crossentropy", metrics=["accuracy"])
        print("QAT 微调...")
        q_model.fit(train_ds, validation_data=val_ds, epochs=QAT_EPOCHS, verbose=0)
        with tfmot.quantization.keras.quantize_scope():
            q_model.save(os.path.join(OUT, "gesture_cnn_qat.h5"))

        def rep_data():
            for x, _ in val_ds.unbatch().take(200):
                yield [tf.reshape(x, (1, IMG_SIZE, IMG_SIZE, 3))]
        with tfmot.quantization.keras.quantize_scope():
            converter = tf.lite.TFLiteConverter.from_keras_model(q_model)
        converter.optimizations = [tf.lite.Optimize.DEFAULT]
        # QAT 模型不传代表性数据集：量化范围由训练中的伪量化节点记录，
        # 再传校准集会与 QAT 范围冲突导致精度崩坏
        converter.inference_input_type = tf.uint8
        converter.inference_output_type = tf.uint8
        qat_tflite = converter.convert()
        qat_path = os.path.join(OUT, "gesture_cnn_qat_int8.tflite")
        with open(qat_path, "wb") as f:
            f.write(qat_tflite)
        acc_qat = eval_tflite_int8(qat_path, val_ds)
        print(f"③ QAT INT8 验证集: {acc_qat:.1f}%  (模型 {os.path.getsize(qat_path)/1020:.0f}KB)")
        dump_tensor_info(qat_path, "QAT")
        print(f"\n对比: 浮点 {acc_float:.1f}% | PTQ {acc_ptq:.1f}% | QAT {acc_qat:.1f}%")
    except ImportError as e:
        print(f"\nQAT 不可用（{e}），保留 PTQ 结果")
        print(f"对比: 浮点 {acc_float:.1f}% | PTQ {acc_ptq:.1f}%")


if __name__ == "__main__":
    main()
