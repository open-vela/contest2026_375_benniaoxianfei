# 面向听障群体的端侧静态手语手势翻译终端 — OpenVela 适配计划书

> 版本：v1.1　日期：2026-08-11　状态：实施中
>
> **进度快照（2026-08-17）**：Phase 0（环境准备）✅ 100%；Phase 1（BSP 移植）**~95%——固件 `nuttx.bin` 编译成功（591KB），编译层面里程碑 M1 达成；正确开发板已邮寄，预计 8-18（周一）到货，随即真机烧录验证**。Phase 6（AI 识别）**并行推进中**：双人数据（10 类 × 2 人），浮点模型跨人验证 **95.5%**，INT8 部署版待 QAT 优化。Phase 2-5 外设（屏幕/摄像头/音频/网络）待板子到位逐项点亮。硬件资料已从官方 PDF 确认（屏幕 EK73217BCGA、音频 ES8311、C6 模组等，详见 `硬件规格速查表_ESP32P4.md`；**注意文档为 P4X 新硬件，板子到手需确认丝印**）。详见 `开发进度日志.md` 与 `移植调研报告_ESP32P4.md`。

---

## 一、项目概述

- **项目名称**：面向听障群体的端侧静态手语手势翻译终端
- **一句话定位**：基于 ESP32-P4-Function-EV-Board 与 OpenVela 系统，摄像头采集静态手语手势，端侧 AI 识别为文字并语音播报，解决听障人士简易日常沟通障碍。
- **核心功能（5 项）**：
  1. 摄像头采集手部图像，识别 12 种静态手语手势（字母 / 数字）
  2. 板载屏幕实时渲染摄像头画面 + 显示翻译文字
  3. 识别成功后本地播放预合成语音（WAV，不做端侧 TTS）
  4. WiFi 上传识别记录，网页查看历史日志
  5. 触摸屏切换识别模式、调整置信度阈值

---

## 1.5 方案调整记录（2026-08-11 修订）

基于环境搭建与源码核实，对原方案做以下调整（答辩与申报材料同步修改）：

### 事实修正（避免答辩被质疑）
| 原表述 | 修正后 |
|--------|--------|
| 板载 NPU 神经网络加速器 | 400MHz 双核 RISC-V + AI 指令扩展（AI instruction extensions，官方术语，非独立 NPU），CPU 端侧推理 |
| 调用板载 NPU 完成推理 | 用 TFLite Micro（OpenVela 官方教程）在 CPU 上跑 INT8 模型 |
| 原生支持 OpenVela 系统 | 需自行移植，可复用 Apache NuttX 上游 esp32p4 BSP |
| WiFi6 蓝牙模组 | 由板载 ESP32-C6 协处理器通过 SDIO（ESP-Hosted）提供 |

### 功能与技术方案调整
1. **摄像头预览降负载**：7 寸屏实时预览用低分辨率（320×240）+ 识别结果叠加，降低带宽与 CPU 占用，把算力留给推理。
2. **WiFi 上传降级预案**：识别日志先本地持久化（Flash/SD）；WiFi 上传为"锦上添花"，若 ESP32-C6 驱动一时调不通，用 USB 导出 / PC 端模拟上传演示，不影响主流程与评分核心。
3. **数据采集提前并行**：12 类手势需自行采集标注，此项提前到 BSP 移植同期在 PC 上进行（模型训练不依赖板子），避免末期赶工。
4. **AI 方案统一**：确定使用 TFLite Micro + INT8 量化轻量 CNN，不依赖专用加速器；推理延迟目标 <300ms。
5. **语音播报保持原案**：预合成 12 段 WAV + 识别提示音，I2S 输出，不使用端侧 TTS。

---

## 二、关键现状核实（截至 2026-08-11，基于你本地 `docs-dev` 文档 + 官方 GitHub 仓库实测）

### 2.1 编译环境约束（重要）

OpenVela 官方快速入门文档明确：

> 仅适配 **Ubuntu 22.04**，**不支持在 Windows 下直接编译，不支持 WSL / Docker 容器**。

- 硬件要求：≥40 GB 硬盘、≥16 GB 内存。
- 结论：**必须准备一台 Ubuntu 22.04 环境**（建议原生安装或独立虚拟机；WSL/Docker 官方不保证可用）。

### 2.2 板卡支持现状：ESP32-P4 尚未被 OpenVela 支持

- 官方文档列出的开发板案例（STM32 系列、ESP32-S3-EYE、ESP32-S3-BOX、FC7300F8M、TC4D9、黄山派等）中 **没有 ESP32-P4-Function-EV-Board**。
- 官方源码实测：
  - `open-vela/nuttx`（dev 分支）`arch/risc-v/src/` 下仅有 `esp32c3 / esp32c6 / esp32h2`，**无 `esp32p4`**；
  - `open-vela/vendor_espressif`（dev 与 `dev-ai-contest-2026` 分支）`boards/` 下仅有 `esp32s3`，**无 P4 板级**。
- 结论：**“板卡原生支持 OpenVela”目前不成立**。ESP32-P4 需自行移植，或等待比赛官方在 `dev-ai-contest-2026` 分支补充 BSP。

### 2.3 好消息：上游 NuttX 已有完整 P4 支持可复用

Apache NuttX 上游已包含：
- 芯片层：`arch/risc-v/src/esp32p4/`
- 板级层：`boards/risc-v/esp32p4/esp32p4-function-ev-board/`（含 common、esp32p4-pico-wifi-wareshare、esp32p4-tab5 等）

OpenVela 基于 NuttX，因此**可将上游芯片/板级代码移植到 OpenVela 的 vendor 结构**，大幅降低工作量。

### 2.4 硬件规格修正（答辩防坑）

需求书中有三处与芯片实际规格不符，建议修正措辞：
1. **“内置 NPU 神经网络加速器”**：ESP32-P4 芯片官方规格中没有独立 NPU。其 AI 加速依赖 400 MHz 双核 RISC-V 的 AI 指令扩展（AI instruction extensions）与 HiFi4 DSP。对 12 类轻量 CNN 完全够用，但答辩时“调用板载 NPU”的说法需改为“CPU + 向量扩展推理”或核实 PDF 是否另有专用加速器。
2. **“WiFi6 蓝牙模组”**：P4 芯片本身无无线；P4-Function-EV-Board 通过板载 **ESP32-C6 协处理器**提供 WiFi6 / BLE5。
3. **“原生支持 OpenVela 系统”**：见 2.2，目前需自行移植。

> 以上三条建议在比赛申报材料中改为：“板卡生态与 OpenVela 同为乐鑫/开源路线，具备移植可行性，且上游 NuttX 已有 BSP 参考”。

---

## 三、总体架构

```
┌─────────────────────────────────────────────────────────────┐
│  应用层（本项目开发）                                          │
│  手语翻译应用 app_sign_translate                             │
│  ├─ 手势识别引擎（TFLite Micro / ESP-DL，INT8 模型）           │
│  ├─ LVGL UI（摄像头预览 + 识别文字 + 模式/阈值设置界面）        │
│  ├─ 语音播报（WAV 播放，I2S 输出）                             │
│  └─ 日志上报（HTTP + JSON → 云端网页）                        │
├─────────────────────────────────────────────────────────────┤
│  OpenVela 系统服务层（NuttX）                                 │
│  ├─ LVGL 图形栈 / VIDEO / AUDIO / NET（TCP-IP, WiFi）        │
│  ├─ 驱动：MIPI-DSI LCD、触摸、MIPI-CSI 摄像头、I2S/Codec      │
│  └─ 文件系统（LittleFS/ROMFS 存模型与 wav 片段）              │
├─────────────────────────────────────────────────────────────┤
│  BSP 层（本次主要移植对象）                                    │
│  ├─ arch：arch/risc-v/src/esp32p4（自 NuttX 上游移植）        │
│  ├─ board：vendor/espressif/boards/esp32p4/esp32p4-function-ev│
│  └─ 工具链：riscv32-unknown-elf-gcc + esptool                 │
└─────────────────────────────────────────────────────────────┘
         硬件：ESP32-P4-Function-EV-Board（P4 + ESP32-C6）
```

---

## 四、适配方案总览

### 方案 A：等待官方 BSP（低风险但被动）
- 比赛使用 `dev-ai-contest-2026` 分支，官方可能陆续补齐 ESP32-P4 BSP。
- 动作：开赛前先确认比赛资料 / 群公告是否提供 P4 固件与 BSP；若提供则直接在其上做应用。

### 方案 B：从 NuttX 上游移植（主动，推荐）
- 移植范围：
  1. 芯片层：`nuttx/arch/risc-v/src/esp32p4` → OpenVela 的 `nuttx` 对应目录（或 vendor 结构）；
  2. 板级层：`boards/risc-v/esp32p4/esp32p4-function-ev-board` → `vendor/espressif/boards/esp32p4/esp32p4-function-ev/`，按 OpenVela 的 `vendor` 目录规范改写（Kconfig、Make.defs、ld.script、`src/<vendor>_boot.c / _bringup.c / _appinit.c`）；
  3. 工具链：安装 RISC-V 交叉编译工具链，烧录用 esptool。
- 参照文档：`docs-dev/zh-cn/chip_porting/porting_guide.md`（新平台适配指南）、`Vendor.md`（vendor 目录规范）、`quickstart/development_board/ESP32-S3-EYE.md`（同类板移植范例，含完整 defconfig）。

**建议：方案 B 为主线，方案 A 随时保持同步**（比赛分支一旦更新 P4 BSP，优先切换）。

---

## 五、分阶段实施计划（每阶段含目标 / 关键工作 / 验收标准）

### Phase 0：环境准备（0.5 周）
- 目标：可编译、可烧录的最小闭环。
- 工作：
  1. 安装 Ubuntu 22.04（或独立虚拟机），装 git / curl / cmake / python3 / build-essential / Git LFS；
  2. 安装 VS Code（≥1.99）与 openvela 插件：`vela.vs-aiot-ide-vela`、`vela.vela-preview`（可选命令行方式）；
  3. `repo init`（GitHub 或 Gitee，dev 分支）→ `repo sync -c -j8`；
  4. 安装 RISC-V 工具链（`riscv32-unknown-elf-gcc`）与 esptool。
- 验收：成功编译任一已有配置（如 goldfish-arm64 模拟器 / esp32s3 配置），模拟器可启动 `lvgldemo`。
- 注意事项：工作目录绝对路径**禁止含中文/空格**（当前 `E:\Desktop\Openvela` 有中文，仅作资料存放，源代码要放到无中文路径）。

### Phase 1：BSP 移植，让 OpenVela 在 P4 板上跑起来（2~3 周，风险最高）
- 目标：板子启动到 NSH，串口可交互。
- 工作：
  1. 核对 OpenVela nuttx 是否已含 `arch/risc-v/src/esp32p4`；没有则从 Apache NuttX 移植（重点：`esp32p4_start.c`、`esp32p4_irq.c`、`esp32p4_lowputc.c`、`esp32p4_timer.c`、Kconfig、Make.defs）；
  2. 创建 `vendor/espressif/boards/esp32p4/esp32p4-function-ev/` 目录结构（configs/nsh、include/board.h、scripts/ld.script+Make.defs、src/boot/bringup/appinit、etc/rcS 等）；
  3. 编写最小 defconfig：串口 console、时钟、PSRAM、基础 FS；
  4. 编译 → 用 esptool 烧录 → NSH 验证 `ifconfig`/`ls`/GPIO 点灯。
- 验收：上电打印 OpenVela 启动日志，`nsh>` 可用，板载 LED 可控制。
- 里程碑 M1：**BSP 点亮**（本阶段为整个项目成败关键，预留最多缓冲时间）。

### Phase 2：显示 + 触摸（1 周）
- 目标：7 寸 MIPI-DSI 屏亮起来，LVGL 出画面，触摸可点。
- 工作：
  1. 移植/启用 esp32p4 MIPI-DSI 与 LCD panel 驱动；
  2. 启用 `CONFIG_GRAPHICS_LVGL`、`CONFIG_LV_USE_NUTTX_LCD`、触摸驱动，参考 esp32s3 defconfig 里的 LVGL 配置段；
  3. 跑通 `lvgldemo` 或自制 UI。
- 验收：屏幕显示 LVGL 界面，触摸点击有响应。

### Phase 3：摄像头采集（1 周）
- 目标：摄像头出图并送入预处理。
- 工作：
  1. 启用 esp32p4 MIPI-CSI 驱动与 VIDEO 框架（`CONFIG_VIDEO*`）；
  2. 适配板载摄像头 sensor 驱动（以板子实际 sensor 型号为准，如 GC2145 / OV2640 类）；
  3. 输出预览帧（送 LVGL，建议低分辨率 320×240 以降低负载）与预处理帧（裁剪、灰度、缩放 96×96/128×128）。
- 验收：屏幕实时显示摄像头画面（低分辨率预览）；可抓取一帧灰度图供后续推理。

### Phase 4：音频播报（0.5 周）
- 目标：识别成功后播放预合成 WAV。
- 工作：启用 I2S + 音频 codec 驱动，用 OpenVela 音频框架或直接 I2S DMA 播放 WAV；预生成 12 个手势的语音片段。
- 验收：程序触发可听到对应语音。

### Phase 5：网络上传（1 周）
- 目标：WiFi6 联网，HTTP 上传 JSON 日志，网页可查。
- 工作：
  1. 启用 ESP32-C6 协处理器无线（SDIO/SPI/UART 与 C6 通信，以 BSP 支持方式为准）+ WiFi 协议栈（参考 esp32s3 defconfig 的 `CONFIG_WIRELESS_WAPI*` / 网络段）；
  2. 用 `CONFIG_NETUTILS_HTTPD` 或自写 socket 实现 HTTP POST JSON；
  3. 云端搭简易 Web 服务（如本地 Python Flask / 免费云）存储与展示日志。
- 验收：板子上传后网页能看到识别记录。
- **降级预案**：若 C6 WiFi 驱动迟迟不通，识别日志先本地持久化（LittleFS/SD），演示时用 USB 导出或 PC 端模拟上传，不阻塞其他功能。

### Phase 6：AI 识别（2 周，与 Phase 1 可并行）
- 目标：12 类手势 INT8 模型在端侧跑通。
- 工作：
  1. 采集/整理 12 类静态手势数据集（字母+数字），PC 端训练轻量 CNN（如 MobileNetV2-0.5 或自定义浅 CNN，目标 <300 KB）；
  2. INT8 量化（PTQ 优先，必要时 QAT），转 `.tflite`；
  3. `xxd -i` 转 C 数组（`alignas(16) const`，放 Flash），集成 TFLite Micro（参照 `docs-dev/zh-cn/edge_ai_dev/` 三篇文档：overview / integration / model_integration）；
  4. 默认 CPU + 向量扩展推理（TFLite Micro），不依赖专用 NPU；数据采集与模型训练在 PC 上提前并行开展。
- 验收：对测试集分类准确率 ≥90%，单次推理延迟 <300 ms。

### Phase 7：应用集成与演示（1 周）
- 目标：5 大功能串成完整演示。
- 工作：
  1. 手语翻译应用 `app_sign_translate`：采集 → 推理 → LVGL 显示文字 → 语音播报 → 日志上传；
  2. 触摸切换模式（数字/字母）与置信度阈值设置；
  3. 稳定性测试（长时间运行不崩溃），按 `porting_guide` 的 xTS 精简集跑一遍 BSP 基础用例。
- 验收：完整演示视频 + 答辩材料。

**总工期建议：8~10 周（其中 BSP 移植留 3 周缓冲）。**

---

## 六、关键目录结构规划

```
openvela/
├── nuttx/
│   └── arch/risc-v/src/esp32p4/          # 芯片层（自 NuttX 上游移植/核对）
├── vendor/espressif/
│   ├── boards/esp32p4/esp32p4-function-ev/
│   │   ├── configs/nsh/defconfig         # 基础 NSH 配置
│   │   ├── configs/ai/defconfig          # 本项目完整配置（AI+LVGL+相机+音频+网络）
│   │   ├── include/board.h
│   │   ├── scripts/ld.script, Make.defs
│   │   └── src/ espressif_boot.c, _bringup.c, _appinit.c
│   └── prebuilt/ riscv32-unknown-elf-gcc
└── apps/ (或 vendor 内)
    └── examples/sign_translate/          # 手语翻译应用
        ├── sign_translate_main.cxx
        ├── model_data.cc                 # INT8 模型数组
        ├── audio/ *.wav                  # 预合成语音片段
        └── CMakeLists.txt
```

---

## 七、风险与对策

| # | 风险 | 等级 | 对策 |
|---|------|------|------|
| 1 | OpenVela 无现成 P4 BSP，芯片层移植难 | 高 | 优先复用 NuttX 上游 esp32p4；提前确认比赛官方是否提供 BSP；Phase 1 预留最多时间 |
| 2 | 环境必须是 Ubuntu 22.04，Windows 用户不便 | 高 | 尽快搭 Ubuntu 22.04（原生/虚拟机）；不依赖 WSL/Docker |
| 3 | “NPU”实为 CPU 向量扩展，推理延迟不确定 | 中 | 12 类静态手势用轻量 CNN，400 MHz 双核足够；提前压测延迟，必要时缩小输入分辨率 |
| 4 | WiFi6 依赖 ESP32-C6 协处理器，驱动/协议栈适配复杂 | 中 | 先以“能联网上传”为目标（不苛求 6 代速率）；预留降级方案（USB/SD 卡导出日志） |
| 5 | 摄像头 sensor 驱动无现成支持 | 中 | 优先用 NuttX/ESP-IDF 已有支持的 sensor 型号，或先用官方 demo 库抓图 |
| 6 | 比赛分支代码变动 | 低 | 定期 `repo sync`，关注 `dev-ai-contest-2026` 分支更新 |
| 7 | 数据准备与标注耗时 | 中 | 提前开始采集 12 类手势数据；用数据增强扩大样本 |

---

## 八、待确认事项清单（评审会前请逐项确认）

1. 比赛是否提供 ESP32-P4-Function-EV-Board 的 OpenVela BSP / 固件？在哪个分支？
2. PDF 硬件资料中板载摄像头 sensor 型号、屏幕分辨率/接口、音频 codec 型号、C6 与 P4 连接方式？（可把 PDF 关键页转成文字/图片发我，我帮你梳理驱动需求）
3. 比赛对“云端日志”有无指定平台？是否允许自建本地服务演示？
4. 手语手势定义：12 类具体是哪些字母/数字？有无标准数据集或必须自采？
5. 答辩/评分侧重（BSP 移植深度 vs 应用完整度 vs AI 效果），决定时间分配。

---

## 九、工具链与 VS Code 问题解答

**需要 VS Code 参与吗？——建议使用，但不是硬性必须。**

- OpenVela 官方提供 **VS Code 插件**（`vela.vs-aiot-ide-vela` + `vela.vela-preview`），支持图形化“创建项目 → 配置 defconfig → 编译 → 模拟器运行 → 断点调试”全流程，还带图片/字体/资源预览，**调试功能必须依赖该插件（C++ 插件）**，对学生很友好。
- 不装插件也能开发：命令行 `./build.sh <defconfig路径> -j$(nproc)` 编译 + esptool 烧录 + GDB 调试。
- **硬性前提**：VS Code 与插件必须在 **Ubuntu 22.04** 上运行（Windows 版 VS Code 无法编译 OpenVela，官方不支持 WSL/Docker）。
- 结论：**在 Ubuntu 22.04 上安装 VS Code + openvela 插件**，作为主力开发 IDE；命令行作为备用。

---

## 十、下一步建议动作（确认计划后执行）

1. 确认第八节“待确认事项”，尤其是比赛是否提供 P4 BSP；
2. 搭建 Ubuntu 22.04 环境 + VS Code + openvela 插件，跑通官方快速入门（模拟器）；
3. 开始 Phase 1 BSP 移植调研（先核对 OpenVela nuttx 是否有 esp32p4，无则评估上游代码量）；
4. 并行启动数据集收集与模型训练（Phase 6 前置）。
