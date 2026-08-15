# 网页日志服务（识别记录接收端）

板端手语翻译终端识别成功后，通过 HTTP POST JSON 把记录上传到这里，
浏览器实时查看识别历史。

## 运行（电脑/服务器上）

```bash
pip install -r requirements.txt
python sign_log_server.py
# 启动后: 浏览器打开 http://本机IP:8080
```

## 板端对接（app/sign_translate/upload.cxx 已按此格式预留）

```http
POST http://<服务器IP>:8080/api/log
Content-Type: application/json

{"gesture": "5_五", "confidence": 87, "ts": 1720000000}
```

## 接口

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | /api/log | 接收一条识别记录（JSON） |
| GET | /api/logs | 返回最近 500 条记录（JSON） |
| GET | / | 网页视图（3 秒自动刷新） |

## 测试

```bash
curl -X POST http://127.0.0.1:8080/api/log \
  -H "Content-Type: application/json" \
  -d '{"gesture":"5_五","confidence":87}'
```

## 备注

- 数据存 SQLite（`sign_logs.db`），重启不丢
- 电脑和板子需同一局域网；板端服务器地址在 `upload_set_server()` 里配置
- 演示用：也可在电脑上模拟上传，网页实时显示（答辩时很直观）
