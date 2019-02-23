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

//////////////////////////////////////////////////////////////////////
#define CHECK_PTR(ptr) \
do{ \
    if(!ptr) { \
        LOGW("invalid ptr:%s",#ptr);\
        return -1; \
    } \
}while(0)


#define CHECK_RET(n,...) \
do{\
    int ret = __VA_ARGS__;\
    if(ret <= n){ \
        LOGW("%s <= %d",#__VA_ARGS__,n);\
        return -1;\
    } \
}while(0)

//////////////////////////////////////////////////////////////////////
/**
 * mqtt事件回调列表
 */
typedef struct {
    /**
     * 对象输出协议数据，请调用send发送给服务器
     * @param arg 用户数据指针
     * @param iov 数据块
     * @param iovcnt 数据块个数
     */
    int (*mqtt_data_output)(void *arg, const struct iovec *iov, int iovcnt);

    /**
     * 登录服务器结果回调
     * @param arg 用户数据指针
     * @param flags @see MqttConnectFlag
     * @param ret_code 0代表成功，@see MqttRetCode
     */
    void (*mqtt_handle_conn_ack)(void *arg, char flags, char ret_code);

    /**
     * 心跳包回复
     * @param arg 用户数据指针
     */
    void (*mqtt_handle_ping_resp)(void *arg);

    /**
     * 收到服务器主题发布的消息
     * @param arg 用户数据指针
     * @param pkt_id 数据包号
     * @param topic 主题
     * @param payload 消息负载
     * @param payloadsize 消息长度
     * @param dup 是否为重复包
     * @param qos qos等级，如果是'只发送一次'类型，那么还将触发mqtt_handle_publish_rel回调
     */
    void (*mqtt_handle_publish)(void *arg,
                                uint16_t pkt_id,
                                const char *topic,
                                const char *payload,
                                uint32_t payloadsize,
                                int dup,
                                enum MqttQosLevel qos);

    /**
     * 服务器释放消息，qos类型为'只发送一次'
     * @param arg 用户数据指针
     * @param pkt_id pkt_id 数据包号
     */
    void (*mqtt_handle_publish_rel)(void *arg,
                                    uint16_t pkt_id);

    /**
     * 回调用户数据指针，本结构体回调函数第一个参数即此参数
     */
    void *_user_data;
} mqtt_callback;
//////////////////////////////////////////////////////////////////////
typedef enum {
  pub_ack = 0,
  pub_rec,
  pub_comp,
} pub_type;

typedef void(*free_user_data)(void *arg);
typedef void (*mqtt_handle_pub_ack)(void *arg,pub_type type);
typedef void (*mqtt_handle_sub_ack)(void *arg,const char *codes, uint32_t count);
typedef void (*mqtt_handle_unsub_ack)(void *arg);

//////////////////////////////////////////////////////////////////////
void *mqtt_alloc_contex(mqtt_callback callback);

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
                          int dup,
                          mqtt_handle_pub_ack cb,
                          void *user_data,
                          free_user_data free_cb,
                          int timeout_sec  );

int mqtt_send_subscribe_pkt(void *ctx,
                            enum MqttQosLevel qos,
                            const char *const *topics,
                            int topics_len,
                            mqtt_handle_sub_ack cb,
                            void *user_data,
                            free_user_data free_cb,
                            int timeout_sec);

int mqtt_send_unsubscribe_pkt(void *ctx,
                              const char *const *topics,
                              int topics_len,
                              mqtt_handle_unsub_ack cb,
                              void *user_data,
                              free_user_data free_cb,
                              int timeout_sec);

int mqtt_send_ping_pkt(void *ctx);

int mqtt_send_disconnect_pkt(void *ctx);
//////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
#endif //MQTT_MQTT_WRAPPER_H
