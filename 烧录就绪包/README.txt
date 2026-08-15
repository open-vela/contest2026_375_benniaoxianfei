# 烧录就绪包（2026-08-15 晚）

**固件**：`nuttx.bin`（591,120 字节）—— OpenVela ESP32-P4 编译产物，2026-08-15 生成

## 三步入坑

1. **连接**：USB 线连板子 → 设备管理器确认 COM 口
2. **烧录**：双击 `flash_windows.bat`，输入 COM 口 → 自动装 esptool 并烧录
   - 或手动：`pip install esptool` 后 `python -m esptool -c esp32p4 -p COM3 -b 460800 write_flash 0x0 nuttx.bin`
   - 烧录失败按 BOOT+RESET 进下载模式再试
3. **看日志**：PuTTY / MobaXterm / VS Code 串口终端，COM 口 + **115200** 波特率
   - 看到 `nsh>` = 真机里程碑达成 🎉
   - 卡住或没输出 → 完整日志发我分析

## 备用：虚拟机烧录（Windows 串口有问题时）

```bash
# VM 里（openvela 虚拟机）
esptool.py -c esp32p4 -p /dev/ttyUSB0 write_flash 0x0 /home/vela/nuttx.bin
```
需要先把 USB 串口挂给虚拟机（VBoxManage usbattach，可找我代劳）

## 配套文档

- `E:\Desktop\Openvela\真机烧录调试手册.md` —— 完整流程 + 常见问题
- `E:\Desktop\Openvela\硬件规格速查表_ESP32P4.md` —— 板子核对清单
