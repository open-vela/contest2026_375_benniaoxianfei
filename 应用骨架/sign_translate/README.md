# sign_translate —— 手语翻译应用骨架（LVGL + TFLite Micro）

> 目标平台：OpenVela (NuttX) + ESP32-P4-Function-EV-Board
> 状态：**骨架**——结构完整、接口清晰，外设驱动/TFLite 集成为 TODO
> 对应计划书：Phase 6（AI 识别）+ Phase 7（应用集成）

## 目录结构

```
sign_translate/
├── CMakeLists.txt          # NuttX apps 构建（复制到 apps/examples/sign_translate/）
├── README.md
├── sign_translate_main.cxx # 入口：解析参数 → LVGL 初始化 → UI → 主循环
├── ui.h / ui.cxx           # LVGL 界面（相机预览区/结果/模式/阈值/状态）
├── recognition.h/.cxx      # TFLite Micro 推理封装（INT8 输入预处理）
├── model_data.cc           # INT8 模型 C 数组（xxd -i 生成后粘贴）
├── camera.h / camera.cxx   # MIPI-CSI 采集桩（TODO: 接 VIDEO 框架）
├── audio.h / audio.cxx     # WAV 播放桩（TODO: 接 I2S/ES8311）
└── upload.h / upload.cxx   # HTTP 日志上传桩（TODO: 接网络栈）
```

## 部署步骤（板端 BSP 跑通后）

1. 把本目录拷到 OpenVela 的 `apps/examples/sign_translate/`（或 vendor 对应位置）
2. 训练出模型 → `xxd -i gesture_cnn_int8.tflite > model_data.cc` → 替换占位文件
3. defconfig 打开：
   - `CONFIG_GRAPHICS_LVGL=y`
   - `CONFIG_EXAMPLES_SIGN_TRANSLATE=y`（构建系统自动）
   - TFLite Micro 库（`CONFIG_TFLM_*` 或按 OpenVela edge_ai 文档）
   - 相机/音频/网络对应配置
4. 编译、烧录、`sign_translate` 命令启动

## 主流程（应用内）

```
定时器(100ms)
  ├─ camera_get_frame()        → 相机桩（TODO: 真采集）
  ├─ recognition_infer(frame)  → TFLite Micro 推理（模型/预处理）
  ├─ 置信度 ≥ 阈值 ?
  │    ├─ ui_show_result(类别, 置信度)
  │    ├─ audio_play_wav(类别)     → 语音播报（TODO: I2S）
  │    └─ upload_log(类别, 置信度) → 日志上传（TODO: HTTP）
  └─ ui_update_preview(frame)  → 屏幕预览
触摸事件：模式切换（数字/字母/词）、阈值滑条
```

## 关键约定

- **LVGL v9 API**（OpenVela 内置版本：`lv_button_create`/`lv_image_create` 风格）
- **C++ 实现**（TFLite Micro 解释器是 C++ API）
- 输入尺寸 `128×128×3`、INT8 量化（与 `训练脚本/train_cnn.py` 一致）
- 类别顺序与 `模型/gesture_labels.txt` 一致（由训练脚本输出）

## TODO 清单（每项对应一个外设阶段）

- [ ] 显示/触摸初始化：用 OpenVela LVGL NuttX 集成（`lv_nuttx_*`）接 MIPI-DSI + 触摸
- [ ] 相机：MIPI-CSI 采集帧 → 预览（320×240）+ 推理帧（128×128 灰度/缩放）
- [ ] 音频：I2S 播 WAV（ES8311 codec）
- [ ] 网络：WiFi（C6）或 Ethernet → HTTP POST JSON
- [ ] 模型集成：TFLite Micro 链接进构建（参考 docs-dev edge_ai_dev 三篇文档）
