/****************************************************************************
 * recognition.cxx —— TFLite Micro 推理封装
 *
 * 模型约定（与 训练脚本/train_cnn.py 一致）：
 *   - 输入：128x128x3 RGB，INT8（scale/zero_point 从模型读）
 *   - 输出：类别数 的 INT8 softmax 概率
 *   - 模型 C 数组：model_data.cc（xxd -i 生成）
 ****************************************************************************/

#include "recognition.h"

#include <cstdio>
#include <cstring>

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_profiler.h"
#include "tensorflow/lite/schema/schema_generated.h"

/* 模型数组：由训练脚本生成（见 model_data.cc 注释） */
extern const unsigned char g_sign_model[];

/****************************************************************************
 * Private Data
 ****************************************************************************/

#define INPUT_W 128
#define INPUT_H 128
#define INPUT_C 3

/* 张量内存池：按实际模型调整（128KB 起步，P4 内存充足） */
static constexpr int kTensorArenaSize = 128 * 1024;
static uint8_t s_tensor_arena[kTensorArenaSize];

static tflite::MicroInterpreter *s_interpreter = nullptr;
static int s_class_count = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* 把 RGB888 帧缩放 + 量化写入 INT8 输入张量 */
static void preprocess(const camera_frame_t *frame, TfLiteTensor *input)
{
  const float scale   = input->params.scale;
  const int   zpoint  = input->params.zero_point;
  int8_t     *in_data = tflite::GetTensorData<int8_t>(input);

  for (int y = 0; y < INPUT_H; y++)
    {
      int sy = (y * frame->height) / INPUT_H;
      for (int x = 0; x < INPUT_W; x++)
        {
          int sx = (x * frame->width) / INPUT_W;
          const uint8_t *pix = frame->pixels + (sy * frame->width + sx) * 3;

          for (int c = 0; c < INPUT_C; c++)
            {
              float q = (pix[c] - zpoint) / scale;
              int idx = (y * INPUT_W + x) * INPUT_C + c;
              in_data[idx] = (int8_t)(q < -128 ? -128 : (q > 127 ? 127 : q));
            }
        }
    }
}

/* 从 INT8 输出取 argmax + 置信度 */
static void postprocess(TfLiteTensor *output, recognition_result_t *result)
{
  const int8_t *out = tflite::GetTensorData<int8_t>(output);
  const float   scale = output->params.scale;
  const int     zpoint = output->params.zero_point;

  int    best = 0;
  int    best_raw = out[0];
  float  sum = 0.0f;
  float  probs[64];   /* 类别数上限，按实际调整 */

  for (int i = 0; i < s_class_count; i++)
    {
      float p = (out[i] - zpoint) * scale;
      probs[i] = p > 0 ? p : 0.0f;
      sum += probs[i];
      if (out[i] > best_raw)
        {
          best_raw = out[i];
          best = i;
        }
    }

  /* 归一化为百分比置信度 */
  float conf = (sum > 0) ? probs[best] / sum * 100.0f : 0.0f;

  result->class_index = best;
  result->confidence  = (int)conf;
  result->valid       = conf > 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int recognition_init(void)
{
  const tflite::Model *model = tflite::GetModel(g_sign_model);
  if (model == nullptr || model->version() != TFLITE_SCHEMA_VERSION)
    {
      std::printf("recognition: model 加载失败/版本不匹配\n");
      return RECOGNITION_ERR_MODEL;
    }

  /* 算子注册：按模型实际算子增减 */
  static tflite::MicroMutableOpResolver<12> resolver;
  resolver.AddConv2D();
  resolver.AddMaxPool2D();
  resolver.AddReshape();
  resolver.AddFullyConnected();
  resolver.AddSoftmax();
  resolver.AddRelu();
  resolver.AddPad();         /* 若训练用 Same padding 转 explicit 时需要 */

  static tflite::MicroInterpreter interpreter(
      model, resolver, s_tensor_arena, kTensorArenaSize);

  if (interpreter.AllocateTensors() != kTfLiteOk)
    {
      std::printf("recognition: AllocateTensors 失败\n");
      return RECOGNITION_ERR_MODEL;
    }

  s_interpreter = &interpreter;

  TfLiteTensor *output = s_interpreter->output(0);
  s_class_count = output->dims->data[output->dims->size - 1];
  std::printf("recognition: %d 类，输入 %dx%dx%d\n",
              s_class_count, INPUT_W, INPUT_H, INPUT_C);

  return RECOGNITION_OK;
}

int recognition_infer(const camera_frame_t *frame,
                      recognition_result_t *result)
{
  if (s_interpreter == nullptr || frame == nullptr || frame->pixels == nullptr)
    {
      return RECOGNITION_ERR_INFER;
    }

  TfLiteTensor *input = s_interpreter->input(0);
  preprocess(frame, input);

  if (s_interpreter->Invoke() != kTfLiteOk)
    {
      return RECOGNITION_ERR_INFER;
    }

  postprocess(s_interpreter->output(0), result);
  return RECOGNITION_OK;
}

int recognition_class_count(void)
{
  return s_class_count;
}
