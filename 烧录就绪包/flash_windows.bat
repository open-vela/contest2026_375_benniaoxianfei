@echo off
chcp 65001 >nul
REM ============================================================
REM  ESP32-P4 OpenVela 固件一键烧录脚本（Windows）
REM  用法：双击运行，按提示输入 COM 口
REM ============================================================
echo.
echo  [ESP32-P4 OpenVela 固件烧录]
echo  ============================
echo.
echo  1. 确认板子已用 USB 线连接电脑
echo  2. 设备管理器里记下 COM 口号（如 COM3）
echo.
set /p PORT=输入 COM 口（如 COM3）: 

echo.
echo  检查/安装 esptool...
python -m esptool version >nul 2>&1
if %errorlevel% neq 0 (
  echo  正在安装 esptool（需联网）...
  pip install esptool
)

echo.
echo  开始烧录 %~dp0nuttx.bin 到 %PORT% ...
python -m esptool -c esp32p4 -p %PORT% -b 460800 write_flash 0x0 "%~dp0nuttx.bin"

echo.
if %errorlevel% equ 0 (
  echo  ============================
  echo  [烧录成功!] 打开串口终端(115200)看启动日志
  echo  ============================
) else (
  echo  [烧录失败] 检查：COM口是否正确、板子是否进下载模式(BOOT+RESET)
)
pause
