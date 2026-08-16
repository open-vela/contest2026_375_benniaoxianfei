#include "deepseek_api.h"
#include "config.h"

#include <WiFiClientSecure.h>
#include <esp_crt_bundle.h>
#include <ArduinoJson.h>

BalanceInfo fetchBalance() {
  BalanceInfo r;
  WiFiClientSecure client;

  client.setTimeout(HTTP_TIMEOUT_MS / 1000);
#if USE_CERT_BUNDLE
  client.setCACertBundle(esp_crt_bundle_get());
#else
  client.setInsecure();
#endif

  Serial.println("[api] 连接 " DEEPSEEK_HOST " ...");
  if (!client.connect(DEEPSEEK_HOST, 443)) {
    r.error = "TLS 连接失败（网络不通或证书问题）";
    Serial.println("[api] connect failed");
    return r;
  }

  client.print("GET " DEEPSEEK_PATH " HTTP/1.1\r\n"
               "Host: " DEEPSEEK_HOST "\r\n"
               "Authorization: Bearer " DEEPSEEK_API_KEY "\r\n"
               "Accept: application/json\r\n"
               "Connection: close\r\n\r\n");

  // 读取完整响应（带超时保护）
  String resp;
  uint32_t t0 = millis();
  bool connOpen = true;
  while (connOpen && (millis() - t0) < HTTP_TIMEOUT_MS) {
    while (client.available()) {
      char c = (char)client.read();
      resp += c;
      if (resp.length() > 8192) break;
    }
    if (!client.connected() && !client.available()) connOpen = false;
    else delay(10);
  }
  client.stop();

  if (resp.length() == 0) {
    r.error = "服务器无响应";
    return r;
  }

  // 拆出状态行 / 头部 / 正文
  int hdrEnd = resp.indexOf("\r\n\r\n");
  String head = (hdrEnd >= 0) ? resp.substring(0, hdrEnd) : resp;
  String body = (hdrEnd >= 0) ? resp.substring(hdrEnd + 4) : "";

  int sp = head.indexOf(' ');
  if (sp >= 0) r.httpCode = atoi(head.c_str() + sp + 1);
  Serial.printf("[api] HTTP %d, body %u bytes\n", r.httpCode, body.length());

  if (r.httpCode != 200) {
    r.error = "HTTP " + String(r.httpCode);
    if (body.length() > 0) r.error += "  " + body.substring(0, 120);
    return r;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    r.error = "JSON 解析失败";
    return r;
  }

  r.isAvailable = doc["is_available"] | false;

  JsonArray infos = doc["balance_infos"].as<JsonArray>();
  bool picked = false;
  for (JsonObject info : infos) {
    const char* cur = info["currency"] | "";
    bool isCny = (strcmp(cur, "CNY") == 0);
    if (!picked || isCny) {
      r.currency = cur;
      r.total    = atof(info["total_balance"]     | "0");
      r.granted  = atof(info["granted_balance"]   | "0");
      r.toppedUp = atof(info["topped_up_balance"] | "0");
      picked = true;
    }
    if (isCny) break;  // 优先取人民币
  }

  r.ok = true;
  Serial.printf("[api] 余额 ¥%.2f (充值 ¥%.2f / 赠送 ¥%.2f)\n",
                r.total, r.toppedUp, r.granted);
  return r;
}
