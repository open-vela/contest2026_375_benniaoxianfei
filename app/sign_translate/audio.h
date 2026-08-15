/****************************************************************************
 * audio.h —— 语音播报接口（I2S + ES8311）
 ****************************************************************************/

#pragma once

/* 打开音频设备（TODO Phase 4：I2S + ES8311 codec 驱动） */
int audio_init(void);

/* 播放第 class_index 类手势对应的 WAV（预合成，不做 TTS） */
int audio_play_wav(int class_index);

/* 关闭音频 */
void audio_deinit(void);
