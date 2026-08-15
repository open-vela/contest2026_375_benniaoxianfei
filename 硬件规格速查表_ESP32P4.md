# ESP32-P4 开发板硬件规格速查表

> 来源：`esp-dev-kits-zh_CN-master-esp32p4.pdf`（乐鑫官方文档，2026-08-09 Release master）
> 整理：2026-08-14
> 用途：OpenCode 写驱动/bringup 时查硬件参数；答辩材料引用

---

## ⚠️ 板卡身份确认（第一优先）

- 本文档 PDF 介绍的是 **ESP32-P4X-Function-EV-Board**（芯片版本 v3.1+ 的新硬件）
- 计划书里写的是老的 **ESP32-P4-Function-EV-Board**
- **板子到手先看丝印确认是哪款**：外设大概率相同（ES8311 + C6 + 7 寸屏 + 2MP 摄像头），但 PCB/引脚可能有微调
- 注意：**P4X（芯片 v3.1）不支持安全下载模式**，烧录时不要启用 secure download

---

## 一、主控与存储

| 项目 | 规格 |
|------|------|
| 主控芯片 | ESP32-P4，双核 RISC-V 400MHz，AI 指令扩展（非独立 NPU） |
| PSRAM | 芯片支持最大 32MB（板载以实际为准，待确认） |
| SPI Flash | 16MB，SPI 接口；最大时钟 80MHz，**不支持自动暂停**（120MHz 需联系乐鑫） |
| MicroSD | 卡槽支持 4-bit 模式，可存/播音频文件 |
| 晶振 | 40MHz 主晶振 + 32.768kHz 低功耗晶振 |

## 二、屏幕（Phase 2 用）

| 项目 | 规格 |
|------|------|
| 屏幕 | 7 英寸 **MIPI-DSI** 电容触摸屏，分辨率 **1024×600** |
| 面板驱动芯片 | **EK73217BCGA**（+ EK79007AD） |
| 接口 | MIPI DSI 连接器（FPC 1.0K-GT-15PB，15 脚） |
| 背光 | PWM 控制，默认 GPIO26（软件可改） |
| 复位 | RST_LCD，默认 GPIO27（软件可改） |
| 参考 | 乐鑫官方有 LVGL Demo v8/v9（ESP-IDF 示例），屏幕初始化代码可参考 |

## 三、摄像头（Phase 3 用）

| 项目 | 规格 |
|------|------|
| 摄像头 | 200 万像素 **MIPI-CSI** 接口 |
| 接口 | MIPI CSI 连接器（FPC 1.0K-GT-15PB，15 脚） |
| **sensor 型号** | ⚠️ **待确认**：PDF 正文未写，需单独"摄像头规格书 PDF"或看模组丝印 |
| 备注 | 其他板参考：P4X-EYE 用 OV2710（2MP）；拿到型号后查 NuttX/ESP-IDF 是否有现成驱动 |

## 四、音频（Phase 4 用）

| 项目 | 规格 |
|------|------|
| Codec | **ES8311**（低功耗单声道：ADC+DAC+前置放大+耳机驱动） |
| 总线 | I2S + I2C 连接 ESP32-P4 |
| 麦克风 | 板载麦克风，接 Audio Codec 接口 |
| 扬声器 | 输出端口，最高可驱动 3W（引脚间距 2.00mm） |
| 其他 | 有独立 Audio PA 芯片 |

## 五、无线与网络（Phase 5 用）

| 项目 | 规格 |
|------|------|
| WiFi/BT 模组 | **ESP32-C6-MINI-1**（2.4GHz Wi-Fi 6 + Bluetooth 5 LE） |
| C6 与 P4 连接方式 | ⚠️ **待确认**：PDF 正文未写（需原理图），大概率 SDIO/SPI（ESP-Hosted 方案） |
| C6 烧录口 | 独立编程接口（ESP-Prog 或 UART 工具） |
| 🎁 有线网络 | **板上还有 Ethernet PHY（RMII）+ RJ45**——WiFi 调不通时可用有线兜底，演示不怕断网 |

## 六、电源与其他

| 项目 | 规格 |
|------|------|
| 供电 | 5V（USB Type-C / 外部），TPS2051C USB 电源开关（500mA 限流） |
| LDO | LDO_VO3 / LDO_VO4 供电板载部分电源域，**需软件配置输出电压与使能**；sleep 模式下注意功耗 |
| 下载调试 | USB 串口/JTAG 接口；BOOT+RESET 进入固件下载模式 |
| 排针 | J1：大部分 GPIO 引出，方便跳线接外设 |

---

## 七、与项目各阶段的对应

| 阶段 | 硬件 | 备注 |
|------|------|------|
| Phase 1 BSP 移植 | 全板 | 串口 console（UART0）、时钟、PSRAM、点灯 |
| Phase 2 显示+触摸 | MIPI-DSI 屏 + EK73217BCGA | 找 NuttX MIPI DSI 框架 + panel 驱动 |
| Phase 3 摄像头 | MIPI-CSI + sensor（型号待定） | 先确认型号再找驱动 |
| Phase 4 音频 | ES8311 + I2S | NuttX 大概率有 ES8311 驱动 |
| Phase 5 网络 | C6（待确认接口）/ Ethernet RMII | 有线做兜底 |
| Phase 6 AI | P4 双核 CPU 推理 | 参考：P4X-EYE 出厂 demo 能跑 YOLOv11nano 实时检测 |

---

## 八、待确认事项清单（板子到手后逐项核对）

1. [ ] 板子丝印：P4-Function-EV-Board 还是 P4X？（决定参考哪版文档/原理图）
2. [ ] 摄像头模组丝印/规格书：sensor 具体型号？（找驱动的前提）
3. [ ] C6 与 P4 的连接接口：SDIO 还是 SPI？（Phase 5 关键，查原理图）
4. [ ] 板载 PSRAM 实际容量（32MB？）
5. [ ] 屏幕触摸 IC 型号（触摸驱动用）

---

## 九、文档里提到的其他板（防止拿错板）

| 板卡 | 差异 |
|------|------|
| ESP32-P4X-C5-Function-EV-Board | WiFi 模组换为 **ESP32-C5-MINI-1**（2.4G+5G 双频 WiFi6 + 802.15.4），其余外设类似 |
| ESP32-P4X-EYE | 视觉开发板：1.54 寸 **ST7789** SPI 屏（240×240）、OV2710 摄像头、C6-MINI-1U、USB 2.0 Device、电池接口 |
