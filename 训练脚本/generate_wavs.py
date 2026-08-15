#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
generate_wavs.py —— 用 TTS 生成 12 段手势语音播报 WAV
=======================================================
用法:
    pip install edge-tts
    python generate_wavs.py

说明:
  - 用微软 Edge TTS（免费，国内可用，无需注册）
  - 输出: 语音/<类名>.wav  (16kHz 单声道 16-bit PCM，适配板端 I2S 播放)
  - 手势文案写在脚本底部 MAP 里，按需修改

备选:
  - 想换声音/引擎（如百度/讯飞），改 tts() 函数即可
"""

import asyncio
import os
import subprocess
import sys

import edge_tts

OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "语音")
VOICE = "zh-CN-XiaoxiaoNeural"   # 中文女声（可换 zh-CN-YunxiNeural 男声）
RATE = "+0%"                     # 语速

# 每类手势对应的播报文案（类名必须与数据集目录名一致）
MAP = {
    "0_零":   "零",
    "1_一":   "一",
    "2_二":   "二",
    "3_三":   "三",
    "4_四":   "四",
    "5_五":   "五",
    "6_六":   "六",
    "7_七":   "七",
    "8_八":   "八",
    "9_九":   "九",
    "你好":   "你好",
    "谢谢":   "谢谢",
}


def convert_to_wav(mp3, wav):
    """mp3 → 16kHz 单声道 16-bit WAV。优先 ffmpeg，失败用 pydub"""
    try:
        subprocess.run(["ffmpeg", "-y", "-i", mp3, "-ar", "16000",
                        "-ac", "1", "-sample_fmt", "s16", wav],
                       check=True, capture_output=True)
        return True
    except FileNotFoundError:
        pass
    try:
        from pydub import AudioSegment
        AudioSegment.from_mp3(mp3).set_frame_rate(16000).set_channels(1) \
            .set_sample_width(2).export(wav, format="wav")
        return True
    except ImportError:
        print("  ⚠️ 没有 ffmpeg 也没装 pydub，需: pip install pydub 或装 ffmpeg")
        return False


async def gen_one(name, text):
    os.makedirs(OUT_DIR, exist_ok=True)
    mp3 = os.path.join(OUT_DIR, name + ".mp3")
    wav = os.path.join(OUT_DIR, name + ".wav")
    comm = edge_tts.Communicate(text, VOICE, rate=RATE)
    await comm.save(mp3)
    if convert_to_wav(mp3, wav):
        os.remove(mp3)
        sz = os.path.getsize(wav)
        print(f"  ✅ {name}.wav  {sz//1024}KB  <- \"{text}\"")
    else:
        print(f"  ⚠️ {name}.mp3 已生成，但未转 WAV")


async def main():
    print(f"生成 {len(MAP)} 段语音到 {OUT_DIR}/ ...")
    for name, text in MAP.items():
        try:
            await gen_one(name, text)
        except Exception as e:
            print(f"  ❌ {name} 失败: {e}（可能网络问题，重试即可）")
    print("\n完成！把 语音/*.wav 拷进板端文件系统（或做成 C 数组）")


if __name__ == "__main__":
    asyncio.run(main())
