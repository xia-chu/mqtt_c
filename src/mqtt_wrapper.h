//
// Created by xzl on 2019/2/22.
//

#ifndef MQTT_MQTT_WRAPPER_H
#define MQTT_MQTT_WRAPPER_H

#include "mqtt.h"
#include "mqtt_buffer.h"
#include "buffer.h"
#include "log.h"
#include <stdio.h>

#define _STR(...) #__VA_ARGS__
#define STR(...) _STR(__VA_ARGS__)

#define CHECK_CTX(ctx) \
do{ \
    if(!ctx) { \
        LOGW("mqtt_context invalid :%p\r\n",ctx);\
        return -1; \
    } \
}while(0)

#define CHECK_RET(n,...) \
do{\
    int ret = __VA_ARGS__;\
    if(ret <= n){ \
        LOGW("%s <= %d\r\n",STR(__VA_ARGS__),n);\
        return -1;\
    } \
}while(0)


struct mqtt_context_t;

typedef int (*FUNC_send_sock)(struct mqtt_context_t *ctx, const struct iovec *iov, int iovcnt);


typedef struct mqtt_context_t{
    struct MqttContext _ctx;
    struct MqttBuffer _buffer;
    FUNC_send_sock _on_send;
    void *_user_data;
    buffer _remain_data;
    unsigned int _pkt_id;
} mqtt_context;

int mqtt_init_contex(mqtt_context *ctx,FUNC_send_sock on_send);

void mqtt_release_contex(mqtt_context *ctx);

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


int mqtt_send_publish_pkt(mqtt_context *ctx,
                          const char *topic,
                          const char *payload,
                          int payload_len,
                          enum MqttQosLevel qos,
                          int retain,
                          int dup);


int mqtt_send_subscribe_pkt(mqtt_context *ctx,
                            enum MqttQosLevel qos,
                            const char *const *topics,
                            int topics_len);

int mqtt_send_unsubscribe_pkt(mqtt_context *ctx,
                              const char *const *topics,
                              int topics_len);

int mqtt_send_ping_pkt(mqtt_context *ctx);

int mqtt_send_disconnect_pkt(mqtt_context *ctx);

#endif //MQTT_MQTT_WRAPPER_H
