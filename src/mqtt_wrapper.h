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

#define CHECK_PTR(ptr) \
do{ \
    if(!ptr) { \
        LOGW("invalid ptr:%s\r\n",#ptr);\
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


typedef struct {
    int (*mqtt_data_output)(void *arg, const struct iovec *iov, int iovcnt);
    int (*mqtt_handle_ping_resp)(void *arg);
    int (*mqtt_handle_conn_ack)(void *arg, char flags, char ret_code);
    int (*mqtt_handle_publish)(void *arg,
                              uint16_t pkt_id,
                              const char *topic,
                              const char *payload,
                              uint32_t payloadsize,
                              int dup,
                              enum MqttQosLevel qos);

    int (*mqtt_handle_pub_ack)(void *arg, uint16_t pkt_id);
    int (*mqtt_handle_pub_rec)(void *arg, uint16_t pkt_id);
    int (*mqtt_handle_pub_rel)(void *arg, uint16_t pkt_id);
    int (*mqtt_handle_pub_comp)(void *arg, uint16_t pkt_id);
    int (*mqtt_handle_sub_ack)(void *arg, uint16_t pkt_id,const char *codes, uint32_t count);
    int (*mqtt_handle_unsub_ack)(void *arg, uint16_t pkt_id);
} mqtt_callback;

typedef struct mqtt_context_t{
    //回调指针
    mqtt_callback _callback;
    //回调用户数据指针
    void *_user_data;
    //只读，最近包id
    unsigned int _pkt_id;
    //私有成员变量，请勿访问
    struct MqttContext _ctx;
    struct MqttBuffer _buffer;
    buffer _remain_data;
} mqtt_context;

int mqtt_init_contex(mqtt_context *ctx,mqtt_callback callback , void *user_data);

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
