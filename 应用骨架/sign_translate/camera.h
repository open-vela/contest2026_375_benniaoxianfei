/****************************************************************************
 * camera.h —— 相机采集接口（MIPI-CSI）
 ****************************************************************************/

#pragma once

#include <cstdint>

/* 一帧 RGB888 图像 */
struct camera_frame_s
{
  int      width;    /* 像素宽（推理用 128 内缩放，预览用原分辨率） */
  int      height;
  uint8_t *pixels;   /* width*height*3 字节，RGB 顺序 */
};

typedef struct camera_frame_s camera_frame_t;

enum camera_status_e
{
  CAMERA_OK = 0,
  CAMERA_ERR_NO_FRAME,
};

/* 打开摄像头（TODO Phase 3：MIPI-CSI + sensor 驱动） */
int camera_init(void);

/* 抓一帧（阻塞；返回的帧在下次调用前有效） */
int camera_get_frame(camera_frame_t *frame);

/* 关闭摄像头 */
void camera_deinit(void);
