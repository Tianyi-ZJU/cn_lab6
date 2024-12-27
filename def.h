#ifndef DEF_H
#define DEF_H
#define MAX_DATA_LEN 1024
#include <stdint.h>
typedef enum {
    REQUEST_TIME = 1,
    REQUEST_NAME,
    REQUEST_LIST,
    REQUEST_MSG
} RequestType;

typedef enum {
    RESPONSE_OK = 1,
    RESPONSE_ERROR,
    RESPONSE_FORWARD_MSG
} ResponseType;

typedef struct {
    uint32_t length;        // 数据包总长度
    RequestType type;       // 请求类型
    uint32_t client_id;     // 客户端编号，发送消息时使用
    char data[MAX_DATA_LEN]; // 数据区，用于传输时间、名字、列表或消息内容
} Packet;

#endif
