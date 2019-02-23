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
        return ret;\
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

/**
 * 发布消息时，服务器回复的类型
 */
typedef enum {
  pub_ack = 0,
  pub_rec,
  pub_comp,
} pub_type;

/**
 * 用户数据指针销毁函数指针
 * @param arg 用户数据指针
 */
typedef void(*free_user_data)(void *arg);

/**
 * 用户发布消息时，服务器回复回调函数
 * @param arg 用户数据指针
 * @param arg 服务器回复类型
 */
typedef void (*mqtt_handle_pub_ack)(void *arg,pub_type type);

/**
 * 用户订阅主体时，服务器回复回调函数
 * @param arg 用户数据指针
 * @param codes @see MqttRetCode，按顺序对应订阅数据包中的Topic结果数组
 * @param count codes数组成员个数
 */
typedef void (*mqtt_handle_sub_ack)(void *arg,const char *codes, uint32_t count);

/**
 * 取消订阅回调
 * @param arg 用户数据指针
 */
typedef void (*mqtt_handle_unsub_ack)(void *arg);

//////////////////////////////////////////////////////////////////////
/**
 * 创建mqtt客户端对象
 * @param callback 回调监听函数列表
 * @return 对象指针
 */
void *mqtt_alloc_contex(mqtt_callback callback);

/**
 * 释放mqtt客户端对象
 * @param ctx mqtt客户端对象指针
 * @return 0代表成功
 */
int mqtt_free_contex(void *ctx);


/**
 * 网络层收到数据后请调用此方法把数据输入到mqtt客户端对象
 * @param ctx mqtt客户端对象
 * @param data 数据指针
 * @param len 数据长度
 * @return 0成功 @see MqttError
 */
int mqtt_input_data(void *ctx,char *data,int len);

/**
 * 定时器轮询，建议每1秒调用一次;内部会触发心跳包以及回复超时回调
 * @param ctx  mqtt客户端对象
 * @return 0成功 @see MqttError
 */
int mqtt_timer_schedule(void *ctx);

/**
 * 发送登录包给服务器
 * @param ctx mqtt客户端对象
 * @param keep_alive 心跳包间隔时间，单位秒
 * @param client_id 客户端类型
 * @param clean_session 为0时，继续使用上一次的会话，若无上次会话则创建新的会话，
 *                      为1时，删除上一次的会话，并建立新的会话
 * @param will_topic "遗言"消息发送到的topic
 * @param will_payload "遗言"消息，当服务器发现设备下线时，自动将该消息发送到will_topic
 * @param will_payload_len "遗言"消息的长度
 * @param will_qos 遗言Qos等级
 * @param will_retain 为0时，服务器发送will_msg后，将删除will_msg，否则将保存will_msg
 * @param user 用户名
 * @param password 密码
 * @return 0代表成功，否则为错误代码，@see MqttError
 */
int mqtt_send_connect_pkt(void *ctx,
                          int keep_alive,
                          const char *client_id,
                          int clean_session,
                          const char *will_topic,
                          const char *will_payload,
                          int will_payload_len,
                          enum MqttQosLevel qos,
                          int will_retain,
                          const char *user,
                          const char *password);

/**
 * 发布数据至服务器
 * @param ctx mqtt客户端对象
 * @param topic 主题
 * @param payload 数据指针
 * @param payload_len 数据长度
 * @param qos 数据qos等级
 * @param retain 非0时，服务器将该publish消息保存到topic下，并替换已有的publish消息
 * @param dup 是否为重复发布
 * @param cb 服务器回复回调函数指针
 * @param user_data 服务器回复回调用户数据指针
 * @param free_cb 服务器回复回调用户数据销毁回调函数指针
 * @param timeout_sec 最大等待回复的时间，单位秒
 * @return 0代表成功，否则为错误代码，@see MqttError
 */
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

/**
 * 订阅主题
 * @param ctx mqtt客户端对象
 * @param qos 接收该主题下消息时希望的qos等级
 * @param topics 监听的主题列表
 * @param topics_len 监听的主题列表个数
 * @param cb 服务器回复回调函数指针
 * @param user_data 服务器回复回调用户数据指针
 * @param free_cb 服务器回复回调用户数据销毁回调函数指针
 * @param timeout_sec 最大等待回复的时间，单位秒
 * @return 0代表成功，否则为错误代码，@see MqttError
 */
int mqtt_send_subscribe_pkt(void *ctx,
                            enum MqttQosLevel qos,
                            const char *const *topics,
                            int topics_len,
                            mqtt_handle_sub_ack cb,
                            void *user_data,
                            free_user_data free_cb,
                            int timeout_sec);

/**
 * 取消主题订阅
 * @param ctx mqtt客户端对象
 * @param topics 取消监听的主题列表
 * @param topics_len 取消监听的主题列表个数
 * @param cb 服务器回复回调函数指针
 * @param user_data 服务器回复回调用户数据指针
 * @param free_cb 服务器回复回调用户数据销毁回调函数指针
 * @param timeout_sec 最大等待回复的时间，单位秒
 * @return 0代表成功，否则为错误代码，@see MqttError
 */
int mqtt_send_unsubscribe_pkt(void *ctx,
                              const char *const *topics,
                              int topics_len,
                              mqtt_handle_unsub_ack cb,
                              void *user_data,
                              free_user_data free_cb,
                              int timeout_sec);
/**
 * 发送登出包
 * @param ctx mqtt客户端对象
 * @return 0代表成功，否则为错误代码，@see MqttError
 */
int mqtt_send_disconnect_pkt(void *ctx);

//////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
#endif //MQTT_MQTT_WRAPPER_H
