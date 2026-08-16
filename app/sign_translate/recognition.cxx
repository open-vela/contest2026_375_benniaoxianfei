/****************************************************************************
 * recognition.cxx —— 识别引擎
 *
 * 两种模式：
 *   [默认/TFLM]  TFLite Micro 推理
 *     - 输入：128x128x3 RGB 原始像素 [0,255]，张量类型/scale/zero_point 从模型读
 *       （转换时 inference_input_type=uint8；若为 int8 同样按参数适配）
 *     - 输出：类别数 的量化 softmax 概率（uint8 或 int8，按模型实际类型）
 *     - 模型 C 数组：model_data.cc（gen_model_data.py 生成）
 *   [CONFIG_EXAMPLES_SIGN_TRANSLATE_SIM]  模拟模式（无 TFLM）
 *     - 用于 OpenVela 模拟器/无 TFLM 环境验证 UI 与交互流程
 *     - 每 SIM_FRAME_STEP 帧轮换一个类别，置信度 85~95%
 ****************************************************************************/

#include "recognition.h"

#include <cstdio>
#include <cstring>

#ifndef CONFIG_EXAMPLES_SIGN_TRANSLATE_SIM

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

/* 把 RGB888 帧缩放 + 量化写入输入张量（uint8/int8 按模型实际类型适配）
 * 量化公式：q = round(pixel/scale + zero_point)，与 infer_test.py 完全一致 */
static void preprocess(const camera_frame_t *frame, TfLiteTensor *input)
{
  const float scale   = input->params.scale;
  const int   zpoint  = input->params.zero_point;
  const bool  is_u8   = (input->type == kTfLiteUInt8);
  uint8_t    *u8_data = tflite::GetTensorData<uint8_t>(input);
  int8_t     *i8_data = tflite::GetTensorData<int8_t>(input);

  for (int y = 0; y < INPUT_H; y++)
    {
      int sy = (y * frame->height) / INPUT_H;
      for (int x = 0; x < INPUT_W; x++)
        {
          int sx = (x * frame->width) / INPUT_W;
          const uint8_t *pix = frame->pixels + (sy * frame->width + sx) * 3;

          for (int c = 0; c < INPUT_C; c++)
            {
              float q = (float)pix[c] / scale + (float)zpoint;
              int idx = (y * INPUT_W + x) * INPUT_C + c;
              if (is_u8)
                {
                  int v = (int)(q + 0.5f);
                  u8_data[idx] = (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
                }
              else
                {
                  int v = (int)(q + 0.5f);
                  i8_data[idx] = (int8_t)(v < -128 ? -128 : (v > 127 ? 127 : v));
                }
            }
        }
    }
}

/* 从量化输出取 argmax + 置信度（uint8/int8 按模型实际类型适配） */
static void postprocess(TfLiteTensor *output, recognition_result_t *result)
{
  const float scale   = output->params.scale;
  const int   zpoint  = output->params.zero_point;
  const bool  is_u8   = (output->type == kTfLiteUInt8);
  const uint8_t *u8_out = tflite::GetTensorData<uint8_t>(output);
  const int8_t  *i8_out = tflite::GetTensorData<int8_t>(output);

  int    best = 0;
  int    best_raw = 0;
  float  sum = 0.0f;
  float  probs[64];   /* 类别数上限，按实际调整 */

  for (int i = 0; i < s_class_count; i++)
    {
      int raw = is_u8 ? (int)u8_out[i] : (int)i8_out[i];
      float p = ((float)raw - zpoint) * scale;
      probs[i] = p > 0 ? p : 0.0f;
      sum += probs[i];
      if (raw > best_raw)
        {
          best_raw = raw;
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

  /* 算子注册：按模型实际算子增减
   * 当前模型（含 Rescaling 首层）：CONV_2D / MAX_POOL_2D / QUANTIZE /
   *                                RESHAPE / FULLY_CONNECTED / SOFTMAX */
  static tflite::MicroMutableOpResolver<12> resolver;
  resolver.AddConv2D();
  resolver.AddMaxPool2D();
  resolver.AddQuantize();   /* Rescaling(1/255) 折叠成的量化节点 */
  resolver.AddReshape();
  resolver.AddFullyConnected();
  resolver.AddSoftmax();
  resolver.AddRelu();       /* 备用（conv 已 fused relu 时用不到） */
  resolver.AddPad();        /* 若训练用 Same padding 转 explicit 时需要 */

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

#else /* CONFIG_EXAMPLES_SIGN_TRANSLATE_SIM —— 模拟模式（无 TFLM） */

#define SIM_CLASS_COUNT 10
#define SIM_FRAME_STEP  20   /* 每 20 帧轮换一个类别 */
#define SIM_CONF_BASE   88

static int s_sim_class = 0;
static int s_sim_frame = 0;
static int s_sim_guard  = 0;   /* 自愈守卫：绕过 goldfish 模拟器 BSS 被启动期覆盖的缺陷 */

/* goldfish 模拟器的应用 BSS 可能被启动期代码覆盖（KASAN/初始化流程），
 * 首次调用时用魔法值校验并重新初始化状态。板端（RISC-V）无此问题。 */
static void sim_state_ensure(void)
{
  if (s_sim_guard != 0x5a5a5a5a)
    {
      s_sim_guard  = 0x5a5a5a5a;
      s_sim_class = 0;
      s_sim_frame = 0;
    }
}

int recognition_init(void)
{
  sim_state_ensure();
  std::printf("recognition: SIM 模式（无 TFLM，轮换 %d 类演示）\n",
              SIM_CLASS_COUNT);
  return RECOGNITION_OK;
}

int recognition_infer(const camera_frame_t *frame,
                      recognition_result_t *result)
{
  if (frame == nullptr || frame->pixels == nullptr)
    {
      return RECOGNITION_ERR_INFER;
    }

  sim_state_ensure();

  /* 每 SIM_FRAME_STEP 帧轮换类别，模拟"识别到不同手势" */
  if (++s_sim_frame >= SIM_FRAME_STEP)
    {
      s_sim_frame = 0;
      s_sim_class = (s_sim_class + 1) % SIM_CLASS_COUNT;
    }

  result->valid       = true;
  result->class_index = s_sim_class;
  result->confidence  = SIM_CONF_BASE + (s_sim_frame * 7) / SIM_FRAME_STEP;
  return RECOGNITION_OK;
}

int recognition_class_count(void)
{
  return SIM_CLASS_COUNT;
}

#endif /* CONFIG_EXAMPLES_SIGN_TRANSLATE_SIM */
