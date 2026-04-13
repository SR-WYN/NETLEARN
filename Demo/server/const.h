#pragma once

// 消息缓冲区最大长度
#define MAX_LENGTH 1024*2
// 消息头部总长度（字节）
#define HEAD_TOTAL_LEN 4
// 消息ID长度（字节）
#define HEAD_ID_LEN 2
// 消息数据长度（字节）
#define HEAD_DATA_LEN 2
// 最大接收队列长度
#define MAX_RECVQUE 10000
// 最大发送队列长度
#define MAX_SENDQUE 1000

// 消息ID
enum MSG_ID
{
    MSG_HELLO_WORLD = 1001
};