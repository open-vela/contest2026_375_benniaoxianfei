#pragma once
#include <Arduino.h>
#include <vector>
#include <time.h>

class Preferences;

// 某一天的花费记录
struct DaySpend {
  String key;    // "YYYYMMDD"
  float  spend;  // 元
};

// 每日用量追踪：
// 思路——每天 0 点后第一次刷新时，把当时余额记为“今日基线”，
// 之后“今日用量 = 基线 − 当前余额”。只依赖官方 /user/balance 接口。
class UsageTracker {
public:
  void load(Preferences& p);
  void save(Preferences& p);

  // 每次拿到新余额后调用，自动处理跨天归档；返回今日已花费金额（元）
  float update(float newTotal, time_t now);

  String todayKey;          // "YYYYMMDD"
  float  todaySpend  = 0;   // 今日已花费（元）
  float  midnightBalance = 0; // 今日凌晨基线余额（元）
  float  lastBalance = 0;   // 最近一次拿到的余额（元）
  time_t lastCheck   = 0;   // 最近一次查询时间

  std::vector<DaySpend> history; // 近 7 日（不含今天），新→旧无所谓，最多 7 条
};
