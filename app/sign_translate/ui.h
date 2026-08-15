/****************************************************************************
 * ui.h —— LVGL 界面接口
 ****************************************************************************/

#pragma once

#include <lvgl/lvgl.h>

/* 识别模式 */
enum ui_mode_e
{
  UI_MODE_DIGITS = 0,   /* 数字 */
  UI_MODE_LETTERS = 1,  /* 字母 */
  UI_MODE_WORDS = 2,    /* 词 */
};

/* 初始化界面（含相机预览区/结果标签/模式按钮/阈值滑条/状态栏） */
void ui_init(int mode, int threshold);

/* 显示识别结果（类别序号 + 置信度 0-100） */
void ui_show_result(int class_index, int confidence);

/* 更新相机预览区（当前为占位绘制，TODO: 接真实帧） */
void ui_update_preview(void *frame);

/* 获取当前阈值（滑条值，50-99） */
int ui_get_threshold(void);

/* 设置识别模式 */
void ui_set_mode(int mode);

/* 获取当前模式 */
int ui_get_mode(void);
