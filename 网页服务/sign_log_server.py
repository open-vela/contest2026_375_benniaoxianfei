#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
sign_log_server.py —— 手语识别日志网页服务（Flask + SQLite）
============================================================
用途：接收板端上传的识别记录（HTTP POST JSON），网页查看历史。

运行:
    pip install flask
    python sign_log_server.py        # 默认 0.0.0.0:8080

板端对接（应用骨架 upload.cxx）:
    POST http://<服务器IP>:8080/api/log
    Content-Type: application/json
    {"gesture": "5_五", "confidence": 87, "ts": 1720000000}

查看:
    浏览器打开 http://<服务器IP>:8080
    API:  GET /api/logs  返回全部记录 JSON
"""

import os
import time
import sqlite3
from datetime import datetime

from flask import Flask, request, jsonify, render_template

DB = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sign_logs.db")
PORT = 8080

app = Flask(__name__)


def init_db():
    conn = sqlite3.connect(DB)
    conn.execute(
        "CREATE TABLE IF NOT EXISTS logs ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " gesture TEXT, confidence INTEGER, ts INTEGER)")
    conn.commit()
    conn.close()


@app.route("/api/log", methods=["POST"])
def add_log():
    """板端上传一条识别记录"""
    data = request.get_json(silent=True) or {}
    gesture = str(data.get("gesture", "unknown"))
    conf = int(data.get("confidence", 0))
    ts = int(data.get("ts", time.time()))
    conn = sqlite3.connect(DB)
    conn.execute("INSERT INTO logs (gesture, confidence, ts) VALUES (?,?,?)",
                 (gesture, conf, ts))
    conn.commit()
    changed = conn.total_changes
    conn.close()
    return jsonify({"ok": True, "id": changed})


@app.route("/api/logs")
def get_logs():
    conn = sqlite3.connect(DB)
    rows = conn.execute(
        "SELECT gesture, confidence, ts FROM logs ORDER BY id DESC LIMIT 500"
    ).fetchall()
    conn.close()
    return jsonify([
        {"gesture": g, "confidence": c, "ts": t} for g, c, t in rows])


@app.route("/")
def index():
    return render_template("index.html")


if __name__ == "__main__":
    init_db()
    print(f"手语识别日志服务启动: http://0.0.0.0:{PORT}")
    app.run(host="0.0.0.0", port=PORT, debug=False)
