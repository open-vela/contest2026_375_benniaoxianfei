#pragma once
// ============================================================
// 用户配置区 —— 只需要改这一个文件
// ============================================================

// ---- 你的 WiFi（2.4GHz） ----
#define WIFI_SSID       "your_wifi_ssid"
#define WIFI_PASS       "your_wifi_password"

// ---- DeepSeek 开放平台 API Key ----
// 获取：https://platform.deepseek.com → API Keys → 创建新密钥（sk- 开头）
#define DEEPSEEK_API_KEY "sk-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// ---- 刷新间隔（分钟）----
#define REFRESH_MIN     30

// ---- 时区（中国 = 8*3600）----
#define TZ_OFFSET_SEC   (8L * 3600L)

// ---- 显示模式 ----
// true  = 常亮模式（适合插电/桌面使用）：循环运行，到点自动刷新，按键/触摸响应
// false = 省电模式（适合戴在手上）：刷新并显示后进入深度睡眠，定时唤醒重刷
#define ALWAYS_ON       true

// ---- NTP 服务器（国内优先）----
#define NTP_SERVER1     "ntp.aliyun.com"
#define NTP_SERVER2     "pool.ntp.org"

// ---- DeepSeek 官方余额接口（一般不需要改）----
#define DEEPSEEK_HOST   "api.deepseek.com"
#define DEEPSEEK_PATH   "/user/balance"

// ---- 网络超时（毫秒）----
#define HTTP_TIMEOUT_MS 15000
#define WIFI_TIMEOUT_S  12
#define NTP_TIMEOUT_S   8

// ---- 使用 ESP32 内置 CA 证书包校验 HTTPS（推荐 true）----
// 若出现 "certificate verify failed"，可临时改为 false（跳过证书校验，不推荐）
#define USE_CERT_BUNDLE true

// ---- 固件版本（“状态”页显示）----
#define FW_VERSION      "v0.1.0"
