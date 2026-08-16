// ============================================================
// DeepSeek 余额 / 每日用量手表 —— M5Stack StopWatch (C152)
// 依赖: M5Unified, M5GFX, ArduinoJson
// 数据源: 官方 GET https://api.deepseek.com/user/balance
// 每日用量: 以“今日凌晨基线余额 − 当前余额”计算，存于 NVS
// ============================================================

#include <Arduino.h>
#include <M5Unified.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_sleep.h>
#include <time.h>

#include "config.h"
#include "deepseek_api.h"
#include "usage_tracker.h"
#include "ui.h"

static Preferences prefs;
static UsageTracker tracker;

static PageId   curPage  = PAGE_BALANCE;
static bool     dirty    = true;
static bool     refreshBusy = false;
static uint32_t lastAuto = 0;
static uint32_t bootMs   = 0;

static bool  timeSynced = false;
static bool  apiOk      = false;
static int   lastHttp   = 0;
static String lastErr;
static float lastTotal = 0, lastGranted = 0, lastToppedUp = 0;
static bool  lastAvailable = false;

// ---------------- WiFi ----------------
static void wifiConnect(int timeoutSec) {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.println("[wifi] 连接 " WIFI_SSID " ...");
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < (uint32_t)timeoutSec * 1000UL) {
    delay(250);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[wifi] 已连接 %s, IP %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[wifi] 连接失败");
  }
}

// ---------------- NTP ----------------
static bool syncNtp(int timeoutSec) {
  configTime(TZ_OFFSET_SEC, 0, NTP_SERVER1, NTP_SERVER2);
  uint32_t t0 = millis();
  while (time(nullptr) < 1600000000L && (millis() - t0) < (uint32_t)timeoutSec * 1000UL) {
    delay(250);
  }
  timeSynced = (time(nullptr) >= 1600000000L);
  if (timeSynced) Serial.println("[ntp] 时间已同步");
  else Serial.println("[ntp] 时间同步失败");
  return timeSynced;
}

// ---------------- 刷新（查询余额 → 更新用量 → 保存）----------------
static void doRefresh() {
  if (refreshBusy) return;
  refreshBusy = true;
  lastErr = "";

  if (WiFi.status() != WL_CONNECTED) wifiConnect(WIFI_TIMEOUT_S);
  if (WiFi.status() != WL_CONNECTED) {
    lastErr = "WiFi 连接失败";
    apiOk = false;
    dirty = true;
    refreshBusy = false;
    return;
  }
  if (!timeSynced) syncNtp(NTP_TIMEOUT_S);

  BalanceInfo b = fetchBalance();
  apiOk     = b.ok;
  lastHttp  = b.httpCode;
  lastAvailable = b.isAvailable;

  if (b.ok) {
    lastTotal   = b.total;
    lastGranted = b.granted;
    lastToppedUp = b.toppedUp;
    time_t now = time(nullptr);
    tracker.update(b.total, now);
    tracker.save(prefs);
  } else {
    lastErr = b.error;
  }

  dirty = true;
  refreshBusy = false;
}

// ---------------- 渲染 ----------------
static void renderNow() {
  UiData d;
  d.apiOk        = apiOk;
  d.httpCode     = lastHttp;
  d.err          = lastErr;
  d.isAvailable  = lastAvailable;
  d.total        = lastTotal;
  d.granted      = lastGranted;
  d.toppedUp     = lastToppedUp;
  d.todaySpend   = tracker.todaySpend;
  d.lastBalance  = tracker.lastBalance;
  d.midnightBalance = tracker.midnightBalance;
  d.lastCheck    = tracker.lastCheck;
  d.todayKey     = tracker.todayKey;
  d.history      = tracker.history;
  d.ssid         = WiFi.SSID();
  d.ip           = WiFi.localIP().toString();
  d.rssi         = WiFi.RSSI();
  d.uptimeSec    = (millis() - bootMs) / 1000;
  d.refreshMin   = REFRESH_MIN;
  d.timeSynced   = timeSynced;
  d.fwVer        = FW_VERSION;
  uiRender(curPage, d);
  dirty = false;
}

// ---------------- 交互 ----------------
static void nextPage() {
  curPage = (PageId)((curPage + 1) % PAGE_COUNT);
  dirty = true;
}
static void prevPage() {
  curPage = (PageId)((curPage + PAGE_COUNT - 1) % PAGE_COUNT);
  dirty = true;
}

static void handleInput() {
  M5.update();

  // 触摸滑动切页
  m5::touch_detail_t td;
  M5.Touch.getDetail(&td);
  static int32_t tsX = -1;
  if (td.wasPressed())  { tsX = td.x; }
  if (td.wasReleased() && tsX >= 0) {
    int32_t dx = td.x - tsX;
    if (dx < -40) nextPage();
    else if (dx > 40) prevPage();
    tsX = -1;
  }

  // 按键：KEYA 切页，KEYB 强制刷新
  if (M5.BtnA.wasPressed()) nextPage();
  if (M5.BtnB.wasPressed()) doRefresh();

  if (dirty) renderNow();
}

// ---------------- 启动 ----------------
void setup() {
  bootMs = millis();
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Lcd.setBrightness(200);
  M5.Lcd.setRotation(0);
  M5.Lcd.fillScreen(TFT_BLACK);

  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== DeepSeek 余额手表 " FW_VERSION " ===");

  prefs.begin("dsq", false);
  tracker.load(prefs);

  wifiConnect(WIFI_TIMEOUT_S);
  if (WiFi.status() == WL_CONNECTED) syncNtp(NTP_TIMEOUT_S);

  doRefresh();          // 开机先查一次
  renderNow();          // 显示第一页

#if !ALWAYS_ON
  // 省电模式：显示后深度睡眠，定时唤醒重新刷新
  Serial.println("[power] 进入深度睡眠 " + String(REFRESH_MIN) + " 分钟");
  M5.Lcd.displaySleep();
  WiFi.disconnect(true);
  prefs.end();
  esp_deep_sleep((uint64_t)REFRESH_MIN * 60ULL * 1000000ULL);
#endif

  lastAuto = millis();
}

// ---------------- 主循环（常亮模式）----------------
void loop() {
  handleInput();

  // 到点自动刷新
  if (millis() - lastAuto >= (uint32_t)REFRESH_MIN * 60000UL) {
    lastAuto = millis();
    doRefresh();
  }

  // WiFi 掉线自动重连
  static uint32_t lastReconnectTry = 0;
  if (WiFi.status() != WL_CONNECTED && (millis() - lastReconnectTry) > 30000UL) {
    lastReconnectTry = millis();
    wifiConnect(WIFI_TIMEOUT_S);
  }

  delay(10);
}
