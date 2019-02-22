//
// Created by xzl on 2019/2/22.
//

#ifndef MQTT_MQTT_WRAPPER_H
#define MQTT_MQTT_WRAPPER_H

#include "mqtt.h"
#include "mqtt_buffer.h"
#include "buffer.h"
#include <stdio.h>

typedef enum {
    log_trace = 0,
    log_debug ,
    log_info ,
    log_warn ,
    log_error ,
} e_log_lev;

extern const char *g_log_lev_str[];

#define LOG(lev,...) \
do{ \
    printf("%s %d\r\n",__FILE__,__LINE__);\
    printf("%s | %s ",g_log_lev_str[lev],__FUNCTION__);\
    printf(__VA_ARGS__);\
} while(0)

#define LOGT(...) LOG(log_trace,__VA_ARGS__)
#define LOGD(...) LOG(log_debug,__VA_ARGS__)
#define LOGI(...) LOG(log_info,__VA_ARGS__)
#define LOGW(...) LOG(log_warn,__VA_ARGS__)
#define LOGE(...) LOG(log_error,__VA_ARGS__)

#define CHECK_CTX(ctx) \
do{ \
    if(!ctx || -1 == ctx->_fd) { \
        LOGW("mqtt_context invalid :%p\r\n",ctx);\
        return -1; \
    } \
}while(0)

#define CHECK_RET(n,...) \
do{\
    int ret = __VA_ARGS__;\
    if(ret <= n){ \
        LOGW("%s <= %d\r\n",#__VA_ARGS__,n);\
        return -1;\
    } \
}while(0)


struct mqtt_context_t;

typedef int (*FUNC_send_sock)(struct mqtt_context_t *ctx, const struct iovec *iov, int iovcnt);


typedef struct mqtt_context_t{
    struct MqttContext _ctx;
    struct MqttBuffer _buffer;

    FUNC_send_sock _on_send;
    int _fd;
    void *_user_data;
    buffer _remain_data;
} mqtt_context;


int mqtt_init_contex(mqtt_context *ctx,FUNC_send_sock on_send);
void mqtt_release_contex(mqtt_context *ctx);

int mqtt_connect_host(mqtt_context *ctx,
                      const char *host,
                      unsigned short port,int timeout);

int mqtt_send_connect_pkt(mqtt_context *ctx,
                          int keep_alive,
                          const char *id,
                          int clean_session,
                          const char *will_topic,
                          const char *will_payload,
                          int will_payload_len,
                          enum MqttQosLevel qos,
                          int will_retain,
                          const char *user,
                          const char *password);

int mqtt_input_data(mqtt_context *ctx,char *data,int len);




#endif //MQTT_MQTT_WRAPPER_H
