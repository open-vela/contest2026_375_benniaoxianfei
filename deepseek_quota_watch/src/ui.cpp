#include "ui.h"
#include "config.h"

#include <M5Unified.h>
#include <M5GFX.h>
#include <WiFi.h>

// ---------- 配色 ----------
static const uint16_t C_BG    = TFT_BLACK;   // 背景（AMOLED 黑）
static const uint16_t C_TXT   = TFT_WHITE;   // 主文字
static const uint16_t C_ACC   = 0x4B5F;      // DeepSeek 蓝 #4D6BFE
static const uint16_t C_GRAY  = 0x8410;      // 次要文字
static const uint16_t C_GREEN = 0x07E0;      // 可用
static const uint16_t C_RED   = 0xF800;      // 警告/错误

static const int W = 466, H = 466;           // 圆形 AMOLED 面板尺寸

static String money(float v) {
  char buf[24];
  snprintf(buf, sizeof(buf), "%.2f", v);
  return String(buf);
}

// 小标题（顶部）
static void drawTitle(const char* s) {
  M5.Lcd.setFont(&fonts::efontCN_16);
  M5.Lcd.setTextColor(C_ACC, C_BG);
  M5.Lcd.setTextDatum(top_center);
  M5.Lcd.drawString(s, W / 2, 26);
}

// 页面底部小字
static void drawFooter(const String& s) {
  M5.Lcd.setFont(&fonts::efontCN_16);
  M5.Lcd.setTextColor(C_GRAY, C_BG);
  M5.Lcd.setTextDatum(bottom_center);
  M5.Lcd.drawString(s, W / 2, H - 18);
}

// 大数字（中间）
static void drawBig(const String& label, const String& value, uint16_t color) {
  M5.Lcd.setFont(&fonts::efontCN_24);
  M5.Lcd.setTextColor(C_GRAY, C_BG);
  M5.Lcd.setTextDatum(top_center);
  M5.Lcd.drawString(label, W / 2, 86);

  M5.Lcd.setFont(&fonts::efontCN_24);
  M5.Lcd.setTextSize(2);            // 48px 大数字
  M5.Lcd.setTextColor(color, C_BG);
  M5.Lcd.drawString(value, W / 2, 128);
  M5.Lcd.setTextSize(1);
}

static String fmtTime(time_t t) {
  if (t < 1600000000L) return "--:--";
  struct tm tmv;
  localtime_r(&t, &tmv);
  char buf[24];
  strftime(buf, sizeof(buf), "%m-%d %H:%M", &tmv);
  return String(buf);
}

// ============ 页面 1：余额 ============
static void renderBalance(const UiData& d) {
  M5.Lcd.fillScreen(C_BG);
  drawTitle("DeepSeek 余额");

  float shown = d.apiOk ? d.total : d.lastBalance;
  drawBig("总余额（元）", "¥" + money(shown), C_TXT);

  M5.Lcd.setFont(&fonts::efontCN_16);
  M5.Lcd.setTextDatum(top_center);
  M5.Lcd.setTextColor(C_GRAY, C_BG);
  M5.Lcd.drawString("充值 ¥" + money(d.toppedUp) + "    赠送 ¥" + money(d.granted),
                    W / 2, 236);

  // 账户状态
  if (!d.apiOk) {
    M5.Lcd.setTextColor(C_RED, C_BG);
    M5.Lcd.drawString("查询失败：" + d.err, W / 2, 290);
  } else if (d.isAvailable) {
    M5.Lcd.setTextColor(C_GREEN, C_BG);
    M5.Lcd.drawString("账户可用", W / 2, 290);
  } else {
    M5.Lcd.setTextColor(C_RED, C_BG);
    M5.Lcd.drawString("余额不足，请充值", W / 2, 290);
  }

  drawFooter("KEYA 切换  KEYB 刷新 · 更新于 " + fmtTime(d.lastCheck));
}

// ============ 页面 2：今日用量 ============
static void renderToday(const UiData& d) {
  M5.Lcd.fillScreen(C_BG);
  drawTitle("今日用量");

  drawBig("今天已花费（元）", "¥" + money(d.todaySpend), C_ACC);

  M5.Lcd.setFont(&fonts::efontCN_16);
  M5.Lcd.setTextDatum(top_left);
  M5.Lcd.setTextColor(C_GRAY, C_BG);
  M5.Lcd.drawString("近 7 日", 60, 230);

  int y = 262;
  M5.Lcd.setTextColor(C_TXT, C_BG);
  if (d.history.empty()) {
    M5.Lcd.setTextColor(C_GRAY, C_BG);
    M5.Lcd.drawString("暂无历史记录", 60, y);
  } else {
    for (auto it = d.history.rbegin(); it != d.history.rend(); ++it) {
      if (it->key.length() < 8) continue;
      String mm = it->key.substring(4, 6);
      String dd = it->key.substring(6, 8);
      String line = "  " + mm + "-" + dd + "     ¥" + money(it->spend);
      M5.Lcd.drawString(line, 60, y);
      y += 30;
      if (y > H - 60) break;
    }
  }

  drawFooter("今日基线 ¥" + money(d.midnightBalance));
}

// ============ 页面 3：状态 ============
static void renderAbout(const UiData& d) {
  M5.Lcd.fillScreen(C_BG);
  drawTitle("状态");

  M5.Lcd.setFont(&fonts::efontCN_16);
  M5.Lcd.setTextDatum(top_left);

  int yRow = 80;
  auto line = [&yRow](const char* k, const String& v, uint16_t c = C_TXT) {
    M5.Lcd.setTextColor(C_GRAY, C_BG);
    M5.Lcd.drawString(String(k), 60, yRow);
    M5.Lcd.setTextColor(c, C_BG);
    M5.Lcd.drawString(v, 190, yRow);
    yRow += 34;
  };

  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  line("WiFi",  wifiOk ? "已连接" : "未连接", wifiOk ? C_GREEN : C_RED);
  line("SSID",  d.ssid);
  line("IP",    d.ip);
  line("信号",  String(d.rssi) + " dBm");
  line("API",   d.apiOk ? ("正常 HTTP " + String(d.httpCode)) : ("失败 " + d.err),
       d.apiOk ? C_GREEN : C_RED);
  line("时间",  d.timeSynced ? fmtTime(time(nullptr)) : "未同步(NTP)");
  line("刷新",  "每 " + String(d.refreshMin) + " 分钟");
  line("运行",  String(d.uptimeSec / 3600) + " 小时 " + String((d.uptimeSec % 3600) / 60) + " 分");
  line("版本",  d.fwVer);

  drawFooter("DeepSeek 余额手表");
}

// ============ 入口 ============
void uiRender(PageId page, const UiData& d) {
  switch (page) {
    case PAGE_BALANCE: renderBalance(d); break;
    case PAGE_TODAY:   renderToday(d);   break;
    case PAGE_ABOUT:   renderAbout(d);   break;
    default: break;
  }
}
