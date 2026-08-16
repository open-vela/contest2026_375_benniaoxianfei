/****************************************************************************
 * sign_translate_main.cxx —— 手语翻译终端应用入口
 *
 * 流程：参数解析 → LVGL 初始化 → 界面构建 → 推理定时器 → 主循环
 ****************************************************************************/

#include <nuttx/config.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <lvgl/lvgl.h>

#include "ui.h"
#include "camera.h"
#include "recognition.h"
#include "audio.h"
#include "upload.h"

/****************************************************************************
 * 配置（未在 Kconfig 定义时使用默认值；接入 defconfig 后可改）
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_SIGN_TRANSLATE_THRESHOLD
#  define CONFIG_EXAMPLES_SIGN_TRANSLATE_THRESHOLD 60   /* 默认置信度阈值 % */
#endif

#ifndef CONFIG_EXAMPLES_SIGN_TRANSLATE_INTERVAL_MS
#  define CONFIG_EXAMPLES_SIGN_TRANSLATE_INTERVAL_MS 200 /* 推理周期 */
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lv_timer_t *g_infer_timer = nullptr;

/****************************************************************************
 * Name: lvgl_init
 *
 * 与 lvgldemo 相同的 OpenVela LVGL-NuttX 初始化：
 *   - lv_nuttx_init() 自动挂载 /dev/fb0（或 goldfish GPU 帧缓冲）与 /dev/input0
 *   - 板端（ESP32-P4）依赖 CONFIG_GRAPHICS_LVGL + CONFIG_LV_USE_NUTTX +
 *     MIPI-DSI 屏驱动（EK73217BCGA）+ 触摸驱动
 ****************************************************************************/

static void lvgl_init(void)
{
  lv_init();

  lv_nuttx_dsc_t   info;
  lv_nuttx_result_t result;

  lv_nuttx_dsc_init(&info);
  lv_nuttx_init(&info, &result);

  if (result.inited == false)
    {
      std::printf("sign_translate: LVGL display init FAILED\n");
    }
}

/****************************************************************************
 * Name: inference_timer_cb —— 周期执行"采集→推理→显示/播报/上传"
 ****************************************************************************/

static void inference_timer_cb(lv_timer_t *timer)
{
  camera_frame_t frame;

  /* 1. 采集（当前为桩，TODO(Phase 3): MIPI-CSI 真采集） */

  if (camera_get_frame(&frame) != CAMERA_OK)
    {
      return;
    }

  /* 2. 推理 */

  recognition_result_t result;
  if (recognition_infer(&frame, &result) != RECOGNITION_OK)
    {
      return;
    }

  /* 3. 阈值过滤 → 显示 + 播报 + 上传 */

  if (result.valid && result.confidence >= ui_get_threshold())
    {
      ui_show_result(result.class_index, result.confidence);
      audio_play_wav(result.class_index);   /* TODO(Phase 4): I2S 播 WAV */
      upload_log(result.class_index, result.confidence); /* TODO(Phase 5) */
    }

  /* 4. 低分辨率预览（TODO(Phase 3): 接真实帧） */

  ui_update_preview(&frame);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

extern "C" int main(int argc, FAR char *argv[])
{
  int threshold = CONFIG_EXAMPLES_SIGN_TRANSLATE_THRESHOLD;
  int mode      = UI_MODE_DIGITS;

  /* 参数：sign_translate [mode] [threshold]
   *   mode: 0=数字 1=字母 2=词
   *   threshold: 置信度 50-99
   */

  if (argc > 1)
    {
      mode = atoi(argv[1]);
    }

  if (argc > 2)
    {
      threshold = atoi(argv[2]);
    }

  std::printf("sign_translate: mode=%d threshold=%d%%\n", mode, threshold);

  /* 初始化子系统 */

  lvgl_init();
  recognition_init();
  camera_init();
  audio_init();
  upload_init();

  /* 构建界面 */

  ui_init(mode, threshold);

  /* 启动推理定时器 */

  g_infer_timer = lv_timer_create(inference_timer_cb,
                                  CONFIG_EXAMPLES_SIGN_TRANSLATE_INTERVAL_MS,
                                  nullptr);
  lv_timer_ready(g_infer_timer);

  /* 主循环 */

  std::printf("sign_translate: running\n");

  for (;;)
    {
      lv_timer_handler();
      usleep(5000);   /* 5ms，给 LVGL 刷屏留时间 */
    }

  return 0;
}
