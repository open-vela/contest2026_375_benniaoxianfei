/****************************************************************************
 * model_data.cc —— INT8 模型 C 数组（占位）
 *
 * 生成步骤（训练完成后，PC 上执行）：
 *   xxd -i gesture_cnn_int8.tflite > model_data.cc
 * 然后：
 *   1. 把数组名改成 g_sign_model
 *   2. 加上 alignas(16)（模型加载要求 16 字节对齐）
 *   3. 删除本占位内容
 *
 * 示例（真数据会很大，几十 KB ~ 几百 KB）：
 ****************************************************************************/

#include <cstdint>

alignas(16) const unsigned char g_sign_model[] = {
  /* ---- 此处粘贴 xxd -i 生成的字节 ---- */
  0x1c, 0x00, 0x00, 0x00, 0x54, 0x46, 0x4c, 0x33, /* TFL3 魔数占位 */
};

const unsigned int g_sign_model_len = sizeof(g_sign_model);
