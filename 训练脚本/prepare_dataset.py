#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
prepare_dataset.py —— 手语数据集整理 + 增强
=============================================
用法:
    python prepare_dataset.py

输入目录结构（原始素材）:
    数据集/raw/<类名>/         照片(jpg/png) 或 视频(mp4/avi)

输出（自动生成）:
    数据集/train/<类名>/       增强后训练集 (128x128)
    数据集/val/<类名>/         验证集 (128x128)
    数据集/gestures.txt        类名清单

依赖: opencv-python, numpy
    pip install opencv-python numpy
"""

import os
import sys
import random
import shutil
import cv2
import numpy as np

BASE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "数据集")
RAW = os.path.join(BASE, "raw")
TRAIN = os.path.join(BASE, "train")
VAL = os.path.join(BASE, "val")
IMG_SIZE = 128          # 与训练脚本一致
VAL_RATIO = 0.2         # 验证集比例
AUG_PER_IMAGE = 6       # 每张原始图增强出几张
SEED = 42

random.seed(SEED)
np.random.seed(SEED)

IMG_EXTS = (".jpg", ".jpeg", ".png", ".bmp", ".webp")
VID_EXTS = (".mp4", ".avi", ".mov", ".mkv", ".m4v")
FRAME_STEP = 10         # 视频每 10 帧抽 1 帧（约 0.3 秒一张）
SKIP_FIRST = 15         # 跳过视频开头 15 帧（手还没进入画面，避免噪音）


def imread_cn(path):
    """读图（支持中文路径，cv2.imread 在 Windows 上不支持非 ASCII 路径）"""
    data = np.fromfile(path, dtype=np.uint8)
    return cv2.imdecode(data, cv2.IMREAD_COLOR)


def imwrite_cn(path, img):
    """写图（支持中文路径）"""
    ext = os.path.splitext(path)[1]
    ok, buf = cv2.imencode(ext, img)
    if ok:
        buf.tofile(path)
    return ok


def load_images_from(class_dir):
    """返回该类的 (素材单元, 路径) 列表：照片每张一个单元，视频每段一个单元"""
    units = []          # [(unit_id, path)]
    tmp_frames = []
    for f in sorted(os.listdir(class_dir)):
        p = os.path.join(class_dir, f)
        ext = os.path.splitext(f)[1].lower()
        if ext in IMG_EXTS:
            units.append((f, p))
        elif ext in VID_EXTS:
            cap = cv2.VideoCapture(p)
            idx = 0
            while True:
                ok, frame = cap.read()
                if not ok:
                    break
                if idx >= SKIP_FIRST and idx % FRAME_STEP == 0:
                    tmp = os.path.join(class_dir, f"_frame_{idx}.jpg")
                    imwrite_cn(tmp, frame)
                    units.append((f, tmp))      # 帧归属同一视频单元
                    tmp_frames.append(tmp)
                idx += 1
            cap.release()
            print(f"  视频 {f}: 有效帧 {idx - SKIP_FIRST}，抽 {max(0, (idx - SKIP_FIRST) // FRAME_STEP)} 帧")
    return units, tmp_frames


def augment(img, allow_flip):
    """对单张图做多种增强，返回生成图列表"""
    outs = []
    h, w = img.shape[:2]
    for _ in range(AUG_PER_IMAGE):
        a = img.copy()
        # 旋转 ±15°
        angle = random.uniform(-15, 15)
        M = cv2.getRotationMatrix2D((w / 2, h / 2), angle, 1.0)
        a = cv2.warpAffine(a, M, (w, h), borderMode=cv2.BORDER_REPLICATE)
        # 水平翻转：仅对非数字手势开启（数字手势翻转会改变语义）
        if allow_flip and random.random() < 0.5:
            a = cv2.flip(a, 1)
        # 亮度/对比度
        a = cv2.convertScaleAbs(a, alpha=random.uniform(0.8, 1.2),
                                beta=random.randint(-20, 20))
        # 平移 ±8%
        dx = int(w * random.uniform(-0.08, 0.08))
        dy = int(h * random.uniform(-0.08, 0.08))
        M = np.float32([[1, 0, dx], [0, 1, dy]])
        a = cv2.warpAffine(a, M, (w, h), borderMode=cv2.BORDER_REPLICATE)
        outs.append(a)
    return outs


def main():
    if not os.path.isdir(RAW):
        print(f"找不到原始素材目录: {RAW}\n请先按 数据采集方案.md 采集数据放进去。")
        sys.exit(1)

    classes = sorted([d for d in os.listdir(RAW)
                      if os.path.isdir(os.path.join(RAW, d))])
    if not classes:
        print("raw/ 下没有类别文件夹！")
        sys.exit(1)

    print(f"发现 {len(classes)} 类: {classes}")

    # 写类名清单
    with open(os.path.join(BASE, "gestures.txt"), "w", encoding="utf-8") as f:
        for c in classes:
            f.write(c + "\n")

    shutil.rmtree(TRAIN, ignore_errors=True)
    shutil.rmtree(VAL, ignore_errors=True)

    for cls in classes:
        class_dir = os.path.join(RAW, cls)
        units, tmp_frames = load_images_from(class_dir)
        print(f"[{cls}] 素材单元 {len(units)} 个")

        # 读取并缩放
        unit_imgs = []          # [(unit_id, img)]
        for unit_id, p in units:
            img = imread_cn(p)
            if img is None:
                continue
            img = cv2.resize(img, (IMG_SIZE, IMG_SIZE))
            unit_imgs.append((unit_id, img))

        # 按素材单元切分（视频整体进训练或验证，防同视频泄漏）
        unit_ids = sorted({u for u, _ in unit_imgs})
        if len(unit_ids) >= 3:
            # 素材充足：视频级切分（无泄漏）
            random.shuffle(unit_ids)
            n_val_units = max(1, int(len(unit_ids) * VAL_RATIO))
            val_ids = set(unit_ids[:n_val_units])
            val_imgs = [img for u, img in unit_imgs if u in val_ids]
            train_imgs = [img for u, img in unit_imgs if u not in val_ids]
            split_note = "视频级切分"
        else:
            # 素材不足（每类仅 1-2 个视频）：帧级切分，尽力而为
            random.shuffle(unit_imgs)
            n_val = max(1, int(len(unit_imgs) * VAL_RATIO))
            val_imgs = [img for _, img in unit_imgs[:n_val]]
            train_imgs = [img for _, img in unit_imgs[n_val:]]
            val_ids = set()
            split_note = "帧级切分(素材不足)"

        # 训练集增强（数字手势禁水平翻转，语义敏感）
        is_digit = cls[0].isdigit()
        os.makedirs(os.path.join(TRAIN, cls), exist_ok=True)
        os.makedirs(os.path.join(VAL, cls), exist_ok=True)
        n = 0
        for img in train_imgs:
            for aug in augment(img, allow_flip=not is_digit):
                imwrite_cn(os.path.join(TRAIN, cls, f"{n:05d}.jpg"), aug)
                n += 1
        print(f"  → 训练 {n} 张（增强后, {len(train_imgs)} 原始帧）")
        for i, img in enumerate(val_imgs):
            imwrite_cn(os.path.join(VAL, cls, f"{i:05d}.jpg"), img)
        print(f"  → 验证 {len(val_imgs)} 张 [{split_note}]")

        # 清理临时帧
        for t in tmp_frames:
            if os.path.exists(t):
                os.remove(t)

    print("\n完成！训练集: 数据集/train/  验证集: 数据集/val/")
    print("下一步: python train_cnn.py")


if __name__ == "__main__":
    main()
