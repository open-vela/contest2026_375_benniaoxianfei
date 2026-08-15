/****************************************************************************
 * camera.cxx —— 相机采集桩
 *
 * TODO(Phase 3)：接入 OpenVela VIDEO 框架 + MIPI-CSI。
 *   1. 确认板载 sensor 型号（硬件速查表待确认②）
 *   2. 启用 CONFIG_VIDEO* 与 esp32p4 MIPI-CSI 驱动
 *   3. 用 video_initialize()/video_open() 抓帧，填充本接口
 *   4. 预览走低分辨率(320x240)，推理帧单独 128x128 缩放
 ****************************************************************************/

#include "camera.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

/* 桩帧缓冲（灰色测试图，方便先联调 UI/推理链路） */
#define STUB_W 320
#define STUB_H 240
static uint8_t s_stub_pixels[STUB_W * STUB_H * 3];
static camera_frame_t s_stub_frame;

int camera_init(void)
{
  /* 生成灰色渐变测试帧 */
  for (int y = 0; y < STUB_H; y++)
    {
      for (int x = 0; x < STUB_W; x++)
      {
        uint8_t v = (uint8_t)((x * 255) / STUB_W);
        uint8_t *p = s_stub_pixels + (y * STUB_W + x) * 3;
        p[0] = v;
        p[1] = v;
        p[2] = v;
      }
    }

  s_stub_frame.width  = STUB_W;
  s_stub_frame.height = STUB_H;
  s_stub_frame.pixels = s_stub_pixels;

  std::printf("camera: 桩模式（TODO Phase 3: MIPI-CSI 真采集）\n");
  return CAMERA_OK;
}

int camera_get_frame(camera_frame_t *frame)
{
  /* TODO(Phase 3)：从 VIDEO 设备抓真实帧 */
  *frame = s_stub_frame;
  return CAMERA_OK;
}

void camera_deinit(void)
{
}
