# sign_translate —— 手语翻译应用（LVGL + TFLite Micro）

> 目标平台：OpenVela (NuttX) + ESP32-P4-Function-EV-Board
> 状态：**已集成**——代码可在 OpenVela apps 构建系统编译；
> 模拟器（goldfish）SIM 模式跑通，板端 TFLM 路径代码就绪待真机
> 对应计划书：Phase 6（AI 识别）+ Phase 7（应用集成）

## 目录结构

```
sign_translate/
├── CMakeLists.txt          # NuttX apps 构建（DEPENDS lvgl tflite_micro）
├── Kconfig                 # EXAMPLES_SIGN_TRANSLATE（含 SIM 模式开关）
├── README.md
├── sign_translate_main.cxx # 入口：LVGL(lv_nuttx_init) → UI → uv/轮询主循环
├── ui.h / ui.cxx           # LVGL 界面（相机预览区/结果/模式/阈值/状态）
├── recognition.h/.cxx      # TFLite Micro 推理封装（UINT8/INT8 自适应预处理）
├── model_data.cc           # INT8 模型 C 数组（gen_model_data.py 生成）
├── camera.h / camera.cxx   # MIPI-CSI 采集桩（TODO: 接 VIDEO 框架）
├── audio.h / audio.cxx     # WAV 播放桩（TODO: 接 I2S/ES8311）
└── upload.h / upload.cxx   # HTTP 日志上传桩（TODO: 接网络栈）
```

## 两种构建模式

| 模式 | 配置 | 说明 |
|------|------|------|
| **板端 TFLM** | `CONFIG_TFLITEMICRO=y`（默认） | 真推理，需 TFLM + flatbuffers/gemmlowp/kissfft/ruy |
| **模拟器 SIM** | `CONFIG_EXAMPLES_SIGN_TRANSLATE_SIM=y` | 假识别轮换 10 类，验证 UI/交互（无 TFLM 依赖） |

## 部署步骤

1. 把本目录拷到 OpenVela 的 `apps/examples/sign_translate/`
2. 模型：`python 训练脚本/gen_model_data.py` 生成 `model_data.cc`（alignas(16)）
3. defconfig 打开（参照 goldfish-arm64-v8a-ap 验证过的组合）：
   - `CONFIG_GRAPHICS_LVGL=y` + `CONFIG_LV_USE_NUTTX=y`（+ LIBUV/触摸按板）
   - `CONFIG_TFLITEMICRO=y`（+ SYSTEM_FLATBUFFERS / MATH_GEMMLOWP / MATH_KISSFFT / MATH_RUY）
   - `CONFIG_EXAMPLES_SIGN_TRANSLATE=y`
   - 模拟器验证时另加 `CONFIG_EXAMPLES_SIGN_TRANSLATE_SIM=y`
4. 编译、烧录、`sign_translate` 命令启动（模拟器可用 rcS 自启）

## 主流程（应用内）

```
定时器(200ms)
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
- 输入尺寸 `128×128×3` 原始像素 [0,255]（模型首层 Rescaling 归一化，训练/校准/推理全链路一致）
- 板端识别预处理按模型输入张量实际类型（UINT8/INT8）自适应
- 类别顺序与 `模型/gesture_labels.txt` 一致（由训练脚本输出）

## TODO 清单（每项对应一个外设阶段，板子到手后）

- [x] 显示/触摸初始化：`lv_nuttx_init` 已在模拟器验证（LIBUV 模式 uv 循环驱动刷新）
- [ ] 相机：MIPI-CSI 采集帧 → 预览（320×240）+ 推理帧（128×128 缩放）
- [ ] 音频：I2S 播 WAV（ES8311 codec）
- [ ] 网络：WiFi（C6）或 Ethernet → HTTP POST JSON
- [x] 模型集成：TFLite Micro 构建接入（板端 defconfig 待真机验证）
