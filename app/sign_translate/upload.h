/****************************************************************************
 * upload.h —— 识别日志上传接口（HTTP + JSON）
 ****************************************************************************/

#pragma once

/* 初始化上传模块（TODO Phase 5：网络栈 + 服务器地址配置） */
int upload_init(void);

/* 上传一条识别记录（阻塞；失败返回负值，调用方可忽略） */
int upload_log(int class_index, int confidence);

/* 上传模块配置 */
void upload_set_server(const char *host, int port, const char *path);
