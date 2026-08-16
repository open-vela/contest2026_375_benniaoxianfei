/****************************************************************************
 * ui.cxx —— LVGL 界面实现（LVGL v9 API）
 *
 * 布局（1024x600 竖屏示例，可按实际屏适配）：
 * ┌──────────────────────────┐
 * │ 状态栏: [模式] [设备状态] │
 * ├──────────────────────────┤
 * │                          │
 * │   相机预览区 (640x360)    │
 * │                          │
 * ├──────────────────────────┤
 * │  识别结果: [大字显示]      │
 * │  置信度:  [进度条]        │
 * ├──────────────────────────┤
 * │ [数字] [字母] [词]  [阈值█]│
 * └──────────────────────────┘
 ****************************************************************************/

#include "ui.h"

#include <cstdio>
#include <cstring>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static lv_obj_t *s_preview_canvas = nullptr;
static lv_obj_t *s_result_label  = nullptr;
static lv_obj_t *s_conf_bar      = nullptr;
static lv_obj_t *s_status_label  = nullptr;
static lv_obj_t *s_threshold_slider = nullptr;

static int s_threshold = 60;
static int s_mode = UI_MODE_DIGITS;
static int s_class_count = 0;
static int s_ui_guard = 0;   /* 自愈守卫：绕过 goldfish 模拟器 BSS 被覆盖的缺陷 */

/* 类别名称表：顺序与 模型/gesture_labels.txt 一致（由训练脚本按目录名排序生成）
 * 当前 12 类：0-10 数字 + 谢谢（你好素材到位后补） */
static const char *s_labels[] = {
  "0 零", "10 十", "1 一", "2 二", "3 三", "4 四",
  "5 五", "6 六", "7 七", "8 八", "9 九", "谢谢",
};

/* goldfish 模拟器的应用 BSS 可能被启动期代码覆盖，首次使用时重新初始化 */
static void ui_state_ensure(void)
{
  if (s_ui_guard != 0x5a5a5a5a)
    {
      s_ui_guard     = 0x5a5a5a5a;
      s_threshold    = 60;
      s_class_count  = (int)(sizeof(s_labels) / sizeof(s_labels[0]));
    }
}

/* 预览画布缓冲（320x180 ARGB8888 占位；TODO: 相机帧直接用） */
#define PREVIEW_W 320
#define PREVIEW_H 180
static lv_color_t s_preview_buf[PREVIEW_W * PREVIEW_H];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void mode_btn_cb(lv_event_t *e)
{
  int mode = (int)(intptr_t)lv_event_get_user_data(e);
  ui_set_mode(mode);
}

static void threshold_cb(lv_event_t *e)
{
  s_threshold = lv_slider_get_value(s_threshold_slider);
  std::printf("ui: threshold=%d\n", s_threshold);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void ui_init(int mode, int threshold)
{
  s_mode = mode;
  s_threshold = threshold;
  s_class_count = (int)(sizeof(s_labels) / sizeof(s_labels[0]));

  lv_obj_t *scr = lv_screen_active();

  /* --- 状态栏 --- */

  s_status_label = lv_label_create(scr);
  lv_obj_align(s_status_label, LV_ALIGN_TOP_LEFT, 12, 8);
  lv_label_set_text(s_status_label, "OpenVela 手语翻译");

  /* --- 相机预览区（canvas 占位，TODO: 接真实相机帧） --- */

  s_preview_canvas = lv_canvas_create(scr);
  lv_obj_set_size(s_preview_canvas, PREVIEW_W, PREVIEW_H);
  lv_obj_align(s_preview_canvas, LV_ALIGN_TOP_MID, 0, 36);
  lv_canvas_set_buffer(s_preview_canvas, s_preview_buf,
                       PREVIEW_W, PREVIEW_H, LV_COLOR_FORMAT_ARGB8888);
  lv_canvas_fill_bg(s_preview_canvas, lv_color_hex(0x202020), LV_OPA_COVER);

  /* --- 识别结果 --- */

  s_result_label = lv_label_create(scr);
  lv_obj_set_style_text_font(s_result_label, &lv_font_montserrat_32, 0);
  lv_obj_align(s_result_label, LV_ALIGN_TOP_MID, 0, 250);
  lv_label_set_text(s_result_label, "等待识别...");

  /* 置信度进度条 */

  s_conf_bar = lv_bar_create(scr);
  lv_obj_set_size(s_conf_bar, 300, 18);
  lv_obj_align(s_conf_bar, LV_ALIGN_TOP_MID, 0, 300);
  lv_bar_set_range(s_conf_bar, 0, 100);
  lv_bar_set_value(s_conf_bar, 0, LV_ANIM_OFF);

  /* --- 模式切换按钮 --- */

  const char *mode_names[] = { "数字", "字母", "词" };
  for (int i = 0; i < 3; i++)
    {
      lv_obj_t *btn = lv_button_create(scr);
      lv_obj_set_size(btn, 90, 42);
      lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, 12 + i * 100, -60);
      lv_obj_add_event_cb(btn, mode_btn_cb, LV_EVENT_CLICKED,
                          (void *)(intptr_t)i);
      lv_obj_t *lbl = lv_label_create(btn);
      lv_label_set_text(lbl, mode_names[i]);
      lv_obj_center(lbl);
    }

  /* --- 阈值滑条（置信度阈值 50-99） --- */

  lv_obj_t *th_lbl = lv_label_create(scr);
  lv_obj_align(th_lbl, LV_ALIGN_BOTTOM_LEFT, 330, -60);
  lv_label_set_text(th_lbl, "阈值");

  s_threshold_slider = lv_slider_create(scr);
  lv_obj_set_size(s_threshold_slider, 220, 20);
  lv_obj_align(s_threshold_slider, LV_ALIGN_BOTTOM_LEFT, 380, -58);
  lv_slider_set_range(s_threshold_slider, 50, 99);
  lv_slider_set_value(s_threshold_slider, s_threshold, LV_ANIM_OFF);
  lv_obj_add_event_cb(s_threshold_slider, threshold_cb, LV_EVENT_VALUE_CHANGED,
                      nullptr);
}

void ui_show_result(int class_index, int confidence)
{
  char buf[64];

  ui_state_ensure();

  if (class_index < 0 || class_index >= s_class_count)
    {
      std::snprintf(buf, sizeof(buf), "未知手势 (%d%%)", confidence);
    }
  else
    {
      std::snprintf(buf, sizeof(buf), "%s  (%d%%)",
                    s_labels[class_index], confidence);
    }

  lv_label_set_text(s_result_label, buf);
  lv_bar_set_value(s_conf_bar, confidence, LV_ANIM_ON);
}

void ui_update_preview(void *frame)
{
  /* TODO(Phase 3): 把相机帧画到 canvas。
   *   1. camera_get_frame() 提供 RGB 帧
   *   2. 缩放/拷贝到 s_preview_buf
   *   3. lv_obj_invalidate(s_preview_canvas)
   */
  (void)frame;
}

int ui_get_threshold(void)
{
  ui_state_ensure();
  return s_threshold;
}

void ui_set_mode(int mode)
{
  s_mode = mode;
  std::printf("ui: mode=%d\n", s_mode);
  /* TODO: 按模式切换类别集合（数字/字母/词） */
}

int ui_get_mode(void)
{
  return s_mode;
}
