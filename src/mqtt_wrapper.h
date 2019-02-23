//
// Created by xzl on 2019/2/22.
//

#ifndef MQTT_MQTT_WRAPPER_H
#define MQTT_MQTT_WRAPPER_H
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mqtt.h"
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


void *mqtt_alloc_contex(mqtt_callback callback , void *user_data);

int mqtt_free_contex(void *ctx);

int mqtt_send_connect_pkt(void *ctx,
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

int mqtt_input_data(void *ctx,char *data,int len);


int mqtt_send_publish_pkt(void *ctx,
                          const char *topic,
                          const char *payload,
                          int payload_len,
                          enum MqttQosLevel qos,
                          int retain,
                          int dup);


int mqtt_send_subscribe_pkt(void *ctx,
                            enum MqttQosLevel qos,
                            const char *const *topics,
                            int topics_len);

int mqtt_send_unsubscribe_pkt(void *ctx,
                              const char *const *topics,
                              int topics_len);

int mqtt_send_ping_pkt(void *ctx);

int mqtt_send_disconnect_pkt(void *ctx);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
#endif //MQTT_MQTT_WRAPPER_H