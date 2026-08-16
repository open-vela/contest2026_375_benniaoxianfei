#pragma once
#include <Arduino.h>
#include "usage_tracker.h"

// 三个页面
enum PageId {
  PAGE_BALANCE = 0,  // 余额
  PAGE_TODAY   = 1,  // 今日用量 + 近7日
  PAGE_ABOUT   = 2,  // 状态/关于
  PAGE_COUNT
};

// 渲染所需的数据快照
struct UiData {
  bool   apiOk      = false;
  int    httpCode   = 0;
  String err;
  bool   isAvailable = false;
  float  total = 0, granted = 0, toppedUp = 0;
  float  todaySpend = 0;
  float  lastBalance    = 0;   // 上次拿到的余额（查询失败时展示缓存值）
  float  midnightBalance = 0;  // 今日凌晨基线
  time_t lastCheck  = 0;
  String todayKey;                 // YYYYMMDD
  std::vector<DaySpend> history;   // 近 7 日
  String ssid, ip;
  int    rssi = 0;
  uint32_t uptimeSec = 0;
  int    refreshMin  = 30;
  bool   timeSynced  = false;
  String fwVer;
};

// 渲染指定页面（整屏重绘）
void uiRender(PageId page, const UiData& d);
