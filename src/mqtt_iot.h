//
// Created by xzl on 2019/3/4.
//

#ifndef MQTT_MQTT_IOT_H
#define MQTT_MQTT_IOT_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "config.h"
#include "iot_type.h"
#include "buffer.h"

typedef struct {

    iot_data_type _type;
    union {
        int _bool;
        buffer _enum;
        buffer _string;
        double _double;
    } _data;
} iot_data;

typedef struct {
    /**
     * 对象输出协议数据，请调用send发送给服务器
     * @param arg 用户数据指针
     * @param iov 数据块
     * @param iovcnt 数据块个数
     */
    int (*iot_on_output)(void *arg, const struct iovec *iov, int iovcnt);

    /**
     * 登录服务器结果回调
     * @param arg 用户数据指针
     * @param flags @see MqttConnectFlag
     * @param ret_code 0代表成功，@see MqttRetCode
     */
    void (*iot_on_connect)(void *arg, char ret_code);


    /**
     * 收到服务器主题发布的消息
     * @param arg
     * @param data_aar
     * @param data_count
     */
    void (*iot_on_message)(void *arg,iot_data *data_aar, int data_count);

    /**
     * 回调用户数据指针，本结构体回调函数第一个参数即此参数
     */
    void *_user_data;
} iot_callback;


void *iot_context_alloc(iot_callback *cb);
int iot_context_free(void *arg);

int iot_connect_pkt(void *iot_ctx,const char *client_id,const char *secret,const char *user_name);


int iot_publish_bool_pkt(void *iot_ctx,int tag,int flag);
int iot_publish_double_pkt(void *iot_ctx,int tag,double double_num);
int iot_publish_enum_pkt(void *iot_ctx,int tag,const char *enum_str);
int iot_publish_string_pkt(void *iot_ctx,int tag,const char *str);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif //MQTT_MQTT_IOT_H
