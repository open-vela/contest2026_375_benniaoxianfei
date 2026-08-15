/****************************************************************************
 * audio.cxx —— 语音播报桩
 *
 * TODO(Phase 4)：I2S 播放 WAV。
 *   1. 启用 ESP32-P4 I2S 驱动 + ES8311 codec（硬件速查表：I2S+I2C 总线）
 *   2. WAV 文件来自 训练脚本/generate_wavs.py（16kHz 单声道 16-bit）
 *   3. 播放方式二选一：
 *      a. 文件系统：WAV 放 LittleFS/ROMFS，open/read + I2S 写
 *      b. C 数组：xxd 转数组内嵌（省文件系统）
 *   4. 建议预加载到内存（12 段共几百 KB，P4 内存够）
 ****************************************************************************/

#include "audio.h"

#include <cstdio>

int audio_init(void)
{
  std::printf("audio: 桩模式（TODO Phase 4: I2S + ES8311）\n");
  return 0;
}

int audio_play_wav(int class_index)
{
  std::printf("audio: 播报类别 %d（TODO: 播放 WAV）\n", class_index);
  return 0;
}

void audio_deinit(void)
{
}
