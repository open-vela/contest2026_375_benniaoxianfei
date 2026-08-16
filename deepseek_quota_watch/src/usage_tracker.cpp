#include "usage_tracker.h"
#include <Preferences.h>

void UsageTracker::load(Preferences& p) {
  todayKey        = p.getString("day_key", "");
  todaySpend      = p.getFloat("today_spend", 0);
  midnightBalance = p.getFloat("mid_base", 0);
  lastBalance     = p.getFloat("last_bal", 0);
  lastCheck       = (time_t)p.getLong("last_ts", 0);

  history.clear();
  time_t now = time(nullptr);
  if (now < 1600000000L) return;  // 时间还没同步，先不读历史

  struct tm tmv;
  localtime_r(&now, &tmv);
  for (int i = 7; i >= 1; i--) {
    struct tm t2 = tmv;
    t2.tm_mday -= i;
    mktime(&t2);
    char key[9];
    strftime(key, 9, "%Y%m%d", &t2);
    float v = p.getFloat((String("hist_") + key).c_str(), -1.0f);
    if (v >= 0) history.push_back({String(key), v});
  }
}

void UsageTracker::save(Preferences& p) {
  p.putString("day_key", todayKey);
  p.putFloat("today_spend", todaySpend);
  p.putFloat("mid_base", midnightBalance);
  p.putFloat("last_bal", lastBalance);
  p.putLong("last_ts", (long)lastCheck);
  for (auto& h : history) {
    p.putFloat((String("hist_") + h.key).c_str(), h.spend);
  }
}

float UsageTracker::update(float newTotal, time_t now) {
  struct tm tmv;
  localtime_r(&now, &tmv);
  char key[9];
  strftime(key, 9, "%Y%m%d", &tmv);
  String k(key);

  lastBalance = newTotal;
  lastCheck   = now;

  if (k != todayKey) {
    // 新的一天：把昨天归档
    if (todayKey.length() > 0 && todaySpend > 0.001f) {
      bool found = false;
      for (auto& h : history) {
        if (h.key == todayKey) { h.spend = todaySpend; found = true; break; }
      }
      if (!found) history.push_back({todayKey, todaySpend});
      while (history.size() > 7) history.erase(history.begin());
    }
    todayKey        = k;
    todaySpend      = 0;
    midnightBalance = newTotal;   // 今日基线 = 今天第一笔余额
  } else {
    float s = midnightBalance - newTotal;
    todaySpend = (s > 0) ? s : 0; // 当天充值会导致余额变多，忽略为 0
  }
  return todaySpend;
}
