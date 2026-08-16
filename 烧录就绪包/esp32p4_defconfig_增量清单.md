# ESP32-P4 板端 defconfig 增量清单（周一板子到货直接用）

> 用途：在 `vendor/espressif/boards/esp32p4/esp32p4-function-ev/configs/openvela/defconfig`
> （当前 45 行最小配置）基础上追加以下开关，然后 `./build.sh vendor/espressif/boards/esp32p4/esp32p4-function-ev/configs/openvela/ --cmake -j$(nproc)`
> 组合已在 **goldfish-arm64-v8a-ap 模拟器验证通过**（2026-08-16）。
> 构建注意：P4 构建前确认 `nuttx/include/arch` 符号链接指向 **risc-v**（goldfish 后改回 arm64 的坑！）。

---

## 一、已验证部分（模拟器跑通，直接照抄）

### 1. 应用
```ini
CONFIG_EXAMPLES_SIGN_TRANSLATE=y
# 模拟器 UI 演示才开（无 TFLM 假识别）；板端真机请保持关闭！
# CONFIG_EXAMPLES_SIGN_TRANSLATE_SIM=y
```

### 2. TFLite Micro（真推理必需）
```ini
CONFIG_TFLITEMICRO=y
CONFIG_SYSTEM_FLATBUFFERS=y
CONFIG_MATH_GEMMLOWP=y
CONFIG_MATH_KISSFFT=y
CONFIG_MATH_RUY=y
```
> goldfish 上 TFLM 编译会因 gemmlowp NEON + libcxx `<format>` 报错，
> **P4 是 RISC-V 无 NEON，不受影响**（这正是模拟器用 SIM 模式的原因）。

### 3. LVGL 基础（字体/刷新/内存，来自 goldfish 验证组合）
```ini
CONFIG_GRAPHICS_LVGL=y
CONFIG_LV_USE_NUTTX=y
CONFIG_LV_USE_NUTTX_LIBUV=y
CONFIG_LV_USE_NUTTX_TOUCHSCREEN=y
CONFIG_LV_DEF_REFR_PERIOD=16
CONFIG_LV_CACHE_DEF_SIZE=5242880
CONFIG_LV_COLOR_MIX_ROUND_OFS=0
CONFIG_LV_FONT_MONTSERRAT_12=y
CONFIG_LV_FONT_MONTSERRAT_16=y
CONFIG_LV_FONT_MONTSERRAT_20=y
CONFIG_LV_FONT_MONTSERRAT_32=y    # 应用结果大字用（ui.cxx）
CONFIG_LV_USE_CLIB_MALLOC=y
CONFIG_LV_USE_CLIB_SPRINTF=y
CONFIG_LV_USE_CLIB_STRING=y
CONFIG_LV_USE_FS_POSIX=y
CONFIG_LV_FS_POSIX_LETTER=47
CONFIG_LV_FS_POSIX_PATH="/"
```

### 4. 其他依赖
```ini
CONFIG_ARCH_DCACHE=y      # 缺了会编译错（arm64 上踩过，P4 同样需要）
CONFIG_ARCH_ICACHE=y
```

---

## 二、待板子到手确认（Phase 2-5 外设，真机逐项点亮）

| 外设 | 待确认项 | 建议配置方向 |
|------|---------|-------------|
| 屏幕 MIPI-DSI | 驱动名（EK73217BCGA+EK79007AD，文档为 P4X 新硬件，**先看丝印**） | `CONFIG_ESP32P4_*LCD*` + `CONFIG_VIDEO_FB` 或 DSI 帧缓冲 |
| 触摸 | 型号（I2C？） | `CONFIG_INPUT_*` + `CONFIG_LV_USE_NUTTX_TOUCHSCREEN`（已开） |
| 摄像头 | 2MP MIPI-CSI sensor 型号 | `CONFIG_VIDEO_*` + esp32p4 CSI 驱动 |
| 音频 | ES8311 codec | `CONFIG_AUDIO_*` + `CONFIG_ES8311*`（I2S+I2C） |
| WiFi | ESP32-C6 接口（SPI/SDIO？） | `CONFIG_NET*` + `CONFIG_ESP32C6*`（备选 Ethernet RMII） |

> 通用排查顺序（照 goldfish 经验）：
> 1. `-Werror` 报错 → 构建加 `-e "-Wno-error"`（build.sh 参数）
> 2. 陈旧的 `nuttx/include/arch` 符号链接、`nuttx/.config`、`apps/builtin/builtin_list.h` → 确认指向正确/删除
> 3. 编译错"expected ')' before CONFIG_XXX" → 检查生成的 config.h 是否陈旧

---

## 三、烧录与验证（沿用现有就绪包）

1. `python 训练脚本/gen_model_data.py` 重新生成 `model_data.cc`（若模型更新）
2. 构建出 `nuttx.bin` → 覆盖 `烧录就绪包/nuttx.bin` → 运行 `flash_windows.bat`（115200 串口）
3. 上电看 `nsh>`（M1 真机）→ `sign_translate` 启动应用
4. 逐项点亮：屏幕显示 → 触摸 → 摄像头 → 音频 → 网络（对照计划书 Phase 2-5）
