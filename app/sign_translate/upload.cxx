/****************************************************************************
 * upload.cxx —— 识别日志上传桩
 *
 * TODO(Phase 5)：HTTP POST JSON。
 *   1. 网络：ESP32-C6（SDIO/SPI，硬件速查表待确认③）或 Ethernet RMII 兜底
 *   2. 协议：CONFIG_NETUTILS_HTTPD 或直接 socket POST
 *   3. 格式示例：
 *        POST /api/log HTTP/1.1
 *        Content-Type: application/json
 *        {"gesture":"5_五","confidence":87,"ts":1720000000}
 *   4. 降级预案：网络不可用时日志先本地写 LittleFS/SD，稍后补传
 ****************************************************************************/

#include "upload.h"

#include <cstdio>
#include <cstring>

static char s_host[64] = "127.0.0.1";
static int  s_port = 8080;
static char s_path[64] = "/api/log";

int upload_init(void)
{
  std::printf("upload: 桩模式（TODO Phase 5: HTTP POST）\n");
  return 0;
}

int upload_log(int class_index, int confidence)
{
  /* TODO(Phase 5)：socket 连接 → 构造 JSON → 发送 → 关闭 */

  std::printf("upload: [%s:%d%s] gesture=%d conf=%d%% (TODO)\n",
              s_host, s_port, s_path, class_index, confidence);

  /* 降级：本地持久化（TODO Phase 5） */
  return 0;
}

void upload_set_server(const char *host, int port, const char *path)
{
  if (host != nullptr)
    {
      std::strncpy(s_host, host, sizeof(s_host) - 1);
    }

  if (path != nullptr)
    {
      std::strncpy(s_path, path, sizeof(s_path) - 1);
    }

  s_port = port;
}
