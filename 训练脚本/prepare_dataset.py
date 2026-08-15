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
    """返回该类的图片路径列表（照片 + 视频抽帧临时文件）"""
    paths = []
    tmp_frames = []
    for f in sorted(os.listdir(class_dir)):
        p = os.path.join(class_dir, f)
        ext = os.path.splitext(f)[1].lower()
        if ext in IMG_EXTS:
            paths.append(p)
        elif ext in VID_EXTS:
            cap = cv2.VideoCapture(p)
            idx = 0
            while True:
                ok, frame = cap.read()
                if not ok:
                    break
                if idx % FRAME_STEP == 0:
                    tmp = os.path.join(class_dir, f"_frame_{idx}.jpg")
                    imwrite_cn(tmp, frame)
                    paths.append(tmp)
                    tmp_frames.append(tmp)
                idx += 1
            cap.release()
            print(f"  视频 {f}: 抽出 {idx // FRAME_STEP} 帧")
    return paths, tmp_frames


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
        paths, tmp_frames = load_images_from(class_dir)
        print(f"[{cls}] 原始 {len(paths)} 张")

        # 先缩放
        imgs = []
        for p in paths:
            img = imread_cn(p)
            if img is None:
                continue
            img = cv2.resize(img, (IMG_SIZE, IMG_SIZE))
            imgs.append(img)

        # 抽验证集（原始图层面抽，避免增强泄漏）
        random.shuffle(imgs)
        n_val = max(1, int(len(imgs) * VAL_RATIO))
        val_imgs = imgs[:n_val]
        train_imgs = imgs[n_val:]

        # 训练集增强（数字类不禁翻转？数字手势语义敏感，统一关闭翻转）
        is_digit = cls[0].isdigit()
        os.makedirs(os.path.join(TRAIN, cls), exist_ok=True)
        os.makedirs(os.path.join(VAL, cls), exist_ok=True)
        n = 0
        for img in train_imgs:
            for aug in augment(img, allow_flip=not is_digit):
                imwrite_cn(os.path.join(TRAIN, cls, f"{n:05d}.jpg"), aug)
                n += 1
        print(f"  → 训练 {n} 张（增强后）")
        for i, img in enumerate(val_imgs):
            imwrite_cn(os.path.join(VAL, cls, f"{i:05d}.jpg"), img)
        print(f"  → 验证 {len(val_imgs)} 张")

        # 清理临时帧
        for t in tmp_frames:
            if os.path.exists(t):
                os.remove(t)

    print("\n完成！训练集: 数据集/train/  验证集: 数据集/val/")
    print("下一步: python train_cnn.py")


if __name__ == "__main__":
    main()
