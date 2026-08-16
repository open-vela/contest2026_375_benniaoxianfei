# 在电脑上先验证你的 DeepSeek API Key 能不能用（不需要手表）
#
# 用法：
#   python scripts/test_balance.py sk-你的key
#
# 输出示例：
#   200
#   {
#     "is_available": true,
#     "balance_infos": [
#       { "currency": "CNY", "total_balance": "110.00",
#         "granted_balance": "10.00", "topped_up_balance": "100.00" }
#     ]
#   }

import sys
import json
import urllib.request

def main():
    key = sys.argv[1] if len(sys.argv) > 1 else "sk-在这里填你的key"
    req = urllib.request.Request(
        "https://api.deepseek.com/user/balance",
        headers={
            "Authorization": "Bearer " + key,
            "Accept": "application/json",
        },
    )
    try:
        with urllib.request.urlopen(req, timeout=15) as r:
            print("HTTP", r.status)
            print(json.dumps(json.load(r), ensure_ascii=False, indent=2))
    except Exception as e:
        print("ERR:", e)
        print("请检查：网络、Key 是否正确、账户是否已充值/实名")

if __name__ == "__main__":
    main()
