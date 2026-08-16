#pragma once
#include <Arduino.h>

// DeepSeek /user/balance 接口返回的数据
struct BalanceInfo {
  bool   ok          = false;   // 本次请求整体是否成功
  int    httpCode    = 0;       // HTTP 状态码（200=成功）
  bool   isAvailable = false;   // 账户是否可用（余额是否充足）
  float  total       = 0;       // 总余额（元）
  float  granted     = 0;       // 赠送余额（元）
  float  toppedUp    = 0;       // 充值余额（元）
  String currency    = "CNY";
  String error;                 // 失败时的中文错误描述
};

// 查询余额（阻塞，最长 HTTP_TIMEOUT_MS 毫秒）
BalanceInfo fetchBalance();
