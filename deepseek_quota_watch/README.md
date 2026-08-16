# DeepSeek 余额 / 每日用量手表（M5Stack StopWatch C152）

把 **M5Stack StopWatch**（ESP32-S3R8，1.75 寸圆形 AMOLED 触屏）变成一块
**DeepSeek 开放平台余额监视手表**：

- 显示总余额、充值余额、赠送余额、账户是否可用
- 计算并显示**今日已花费金额**（今日凌晨基线余额 − 当前余额）
- 近 7 日花费历史（存在手表 Flash 里）
- 状态页：WiFi / IP / 信号 / API 状态 / 更新时间
- 触摸滑动或按键切页；到点自动刷新；支持省电深睡模式

数据只来自 DeepSeek 官方接口：`GET https://api.deepseek.com/user/balance`
（参考 [官方文档-查询余额](https://api-docs.deepseek.com/zh-cn/api/get-user-balance/)）。

---

## 一、准备工作

1. **注册 DeepSeek 开放平台并充值**：https://platform.deepseek.com
2. **创建 API Key**：控制台 → API Keys → 创建新密钥（`sk-` 开头）
3. **用电脑验证 Key 是否可用**（可选但推荐）：

   ```bash
   python scripts/test_balance.py sk-你的key
   ```

   能打印出余额 JSON 就说明 Key 没问题。

## 二、配置（只需改一个文件）

编辑 `src/config.h`：

```c
#define WIFI_SSID        "你家WiFi名"
#define WIFI_PASS        "WiFi密码"
#define DEEPSEEK_API_KEY "sk-你的key"
#define REFRESH_MIN      30        // 刷新间隔（分钟）
#define ALWAYS_ON        true      // true=常亮(插电用)  false=省电(深睡)
```

> 注意：API Key 会编译进固件，属于私人固件，不要到处分享。

## 三、编译 & 烧录（Windows）

### 方式 A：PlatformIO 命令行（本机已装）

```bash
# 在项目目录
cd E:\Desktop\Openvela\deepseek_quota_watch
pio run -t upload          # 端口自动识别；不对就加 --upload-port COM7
```

### 方式 B：VS Code + PlatformIO 插件（图形化，推荐新手）

1. VS Code 安装 **PlatformIO IDE** 插件
2. 打开本文件夹，等依赖下载完成
3. 点底部 **Upload**（上传箭头）按钮

### 手表进入下载模式

USB-C 数据线连接电脑后，**长按电源键约 2 秒直到绿灯亮起**，松开即为下载模式，
然后点上传。烧录成功后会自动重启。

### 看日志

```bash
pio device monitor -b 115200
```

## 四、使用说明

| 操作 | 效果 |
|------|------|
| 右滑 / 按 KEYA | 下一页（余额 → 今日用量 → 状态） |
| 左滑 | 上一页 |
| 短按 KEYB | 立即刷新一次 |
| 到点自动 | 每 `REFRESH_MIN` 分钟自动刷新 |

页面内容：

- **余额页**：总余额大字、充值/赠送拆分、账户可用状态、上次更新时间
- **今日用量页**：今日花费大字、近 7 日花费列表、今日基线余额
- **状态页**：WiFi/SSID/IP/信号/API 状态/NTP 时间/运行时长/版本

## 五、常见问题

| 现象 | 处理 |
|------|------|
| 上传失败 “Failed to connect” | 重新进下载模式（长按电源键到绿灯）；换根能传数据的 USB 线 |
| 屏幕不亮 | 检查是否烧录成功；把 M5Unified/M5GFX 更新到最新版（本工程已用 GitHub 最新版） |
| 显示“WiFi 连接失败” | 确认 WiFi 是 2.4GHz、密码正确、`WIFI_SSID` 没写错 |
| 显示“TLS 连接失败” | 多半是网络问题；确认能访问 api.deepseek.com；也可把 `USE_CERT_BUNDLE` 改 false 重试（不推荐） |
| 显示“HTTP 401” | API Key 错误或未充值，用 `test_balance.py` 复查 |
| 今日用量是 0 | 正常——当天没调用 API 或刚跨天；跨天后第一次刷新才会归档前一天 |
| 中文字体有缺字 | efontCN 字库是 GB2312 常用字，生僻字会显示空白，可换成英文标签 |

## 六、目录结构

```
deepseek_quota_watch/
├── platformio.ini          # 编译配置（官方 M5Stack 配置）
├── partitions.csv          # 分区表（app 3MB，够放中文字库）
├── scripts/test_balance.py # 电脑端验证 API Key
└── src/
    ├── config.h            # ★ 用户配置（WiFi/Key/刷新间隔）
    ├── main.cpp            # 主逻辑（WiFi/NTP/刷新/深睡/交互）
    ├── deepseek_api.*      # HTTPS 余额查询 + JSON 解析
    ├── usage_tracker.*     # 每日用量统计（NVS 持久化）
    └── ui.*                # 圆形 AMOLED 三页 UI
```

## 七、原理：每日用量怎么算的？

DeepSeek 没有公开“每日用量”接口，但余额接口是实时的：

- 每天 0 点后第一次刷新时，把当时余额记为**今日基线**
- 之后每次刷新：`今日用量 = 今日基线 − 当前余额`
- 跨天时把昨天金额归档到历史，并重置基线

这样只依赖官方接口，断电也不丢数据（存在 Flash NVS 里）。
