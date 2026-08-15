/****************************************************************************
 * recognition.h —— 识别引擎接口
 ****************************************************************************/

#pragma once

#include "camera.h"

/* 识别结果 */
struct recognition_result_s
{
  bool   valid;          /* 是否有有效结果 */
  int    class_index;    /* 类别序号（对应 gesture_labels.txt 顺序） */
  int    confidence;     /* 置信度 0-100 */
};

typedef struct recognition_result_s recognition_result_t;

enum recognition_status_e
{
  RECOGNITION_OK = 0,
  RECOGNITION_ERR_MODEL,
  RECOGNITION_ERR_INFER,
};

/* 初始化 TFLite Micro 解释器（加载模型、分配张量） */
int recognition_init(void);

/* 对一帧执行推理；成功返回 RECOGNITION_OK */
int recognition_infer(const camera_frame_t *frame,
                      recognition_result_t *result);

/* 类别总数 */
int recognition_class_count(void);
