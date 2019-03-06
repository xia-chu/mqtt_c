//
// Created by xzl on 2019/3/4.
//

#ifndef MQTT_MQTT_IOT_H
#define MQTT_MQTT_IOT_H

#include "config.h"
#include "iot_type.h"
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * 收到服务器下发的数据端点数据
 */
typedef struct {
    //端点索引id
    uint32_t _tag_id;
    //数据类型
    iot_data_type _type;

    //数据枚举，该值为只读变量
    union {
        uint8_t _bool;
        buffer _enum;
        buffer _string;
        double _double;
    } _data;

} iot_data;

typedef struct {
    /**
     * 对象输出协议数据，请调用writev发送给服务器
     * @param arg 用户数据指针,即本结构体的_user_data参数
     * @param iov 数据块
     * @param iovcnt 数据块个数
     * @param 返回-1代表失败，大于0则为成功
     */
    int (*iot_on_output)(void *arg, const struct iovec *iov, int iovcnt);

    /**
     * 连接iot服务器成功后回调
     * @param arg 用户数据指针,即本结构体的_user_data参数
     * @param ret_code 失败时为错误代码，0代表成功
     */
    void (*iot_on_connect)(void *arg, char ret_code);

    /**
     * 收到服务器下发的端点数据回调
     * @param arg 用户数据指针,即本结构体的_user_data参数
     * @param req_flag 数据类型，最后一位为0则代表回复，为1代表请求
     * @param req_id 本次请求id
     * @param data 端点数据，只读
     */
    void (*iot_on_message)(void *arg,int req_flag, uint32_t req_id, iot_data *data);

    /**
     * 回调用户数据指针，本结构体回调函数第一个参数即此参数
     */
    void *_user_data;
} iot_callback;

/**
 * 创建iot客户端对象
 * @param cb 回调结构体参数
 * @return  对象指针
 */
void *iot_context_alloc(iot_callback *cb);

/**
 * 释放iot客户端对象
 * @param arg 对象指针
 * @return 0代表成功，-1为失败
 */
int iot_context_free(void *arg);

/**
 * 发送连接、登录包
 * @see iot_context_alloc
 * @param iot_ctx 对象指针
 * @param client_id 客户端id
 * @param secret 授权码
 * @param user_name 用户名，一般为JIMIMAX
 * @return 0为成功，其他为错误代码
 */
int iot_send_connect_pkt(void *iot_ctx,const char *client_id,const char *secret,const char *user_name);

/**
 * 发送布尔型的端点数据
 * @param iot_ctx 对象指针
 * @param tag_id 端点id
 * @param flag 端点值，0或1
 * @return 0为成功，其他为错误代码
 */
int iot_send_bool_pkt(void *iot_ctx,uint32_t tag_id,int flag);

/**
 * 发送双精度浮点型的端点数据
 * @param iot_ctx 对象指针
 * @param tag_id 端点id
 * @param double_num 端点值
 * @return 0为成功，其他为错误代码
 */
int iot_send_double_pkt(void *iot_ctx,uint32_t tag_id,double double_num);

/**
 * 发送枚举型的端点数据
 * @param iot_ctx 对象指针
 * @param tag_id 端点id
 * @param enum_str 枚举字符串，以'\0'结尾
 * @return 0为成功，其他为错误代码
 */
int iot_send_enum_pkt(void *iot_ctx,uint32_t tag_id,const char *enum_str);

/**
 * 发送字符串型的端点数据
 * @param iot_ctx 对象指针
 * @param tag_id 端点id
 * @param str 字符串，以'\0'结尾
 * @return 0为成功，其他为错误代码
 */
int iot_send_string_pkt(void *iot_ctx,uint32_t tag_id,const char *str);

///////////////////////////////////////////////////////////////////////////////
/**
 * 开始批量生成多个端点数据
 * @see iot_get_request_id
 * @param buffer 存放数据的对象buffer
 * @param req_flag 最后一位为0则代表回复，为1代表请求
 * @param req_id 本次请求序号,请调用iot_get_request_id函数生成
 * @return 0代表成功，-1为失败
 */
int iot_buffer_start(buffer *buffer, int req_flag, uint32_t req_id);

/**
 * 往buffer中添加布尔类型的端点数据
 * @param buffer 存放数据的对象buffer
 * @param tag_id 端点id
 * @param bool_flag 布尔值
 * @return 0代表成功，-1为失败
 */
int iot_buffer_append_bool(buffer *buffer, uint32_t tag_id , uint8_t bool_flag);

/**
 * 往buffer中添加枚举类型的端点数据
 * @param buffer 存放数据的对象buffer
 * @param tag_id 端点id
 * @param enum_str 枚举字符串，以'\0'结尾
 * @return 0代表成功，-1为失败
 */
int iot_buffer_append_enum(buffer *buffer, uint32_t tag_id, const char *enum_str);

/**
 * 往buffer中添加字符串类型的端点数据
 * @param buffer 存放数据的对象buffer
 * @param tag_id 端点id
 * @param str 字符串，以'\0'结尾
 * @return 0代表成功，-1为失败
 */
int iot_buffer_append_string(buffer *buffer, uint32_t tag_id, const char *str);

/**
 * 往buffer中添加双精度浮点型类型的端点数据
 * @param buffer 存放数据的对象buffer
 * @param tag_id 端点id
 * @param double_num 双精度浮点型数据
 * @return 0代表成功，-1为失败
 */
int iot_buffer_append_double(buffer *buffer, uint32_t tag_id , double double_num);
///////////////////////////////////////////////////////////////////////////////


/**
 * 批量发送多个数据端点数据
 * @see iot_buffer_start
 * @param iot_ctx 对象指针
 * @param buffer 一个或多个端点的数据
 * @return 0为成功，其他为错误代码
 */
int iot_send_buffer(void *iot_ctx,buffer *buffer);

/**
 * 网络层收到数据后请调用此函数输入给本对象处理
 * @param iot_ctx 对象指针
 * @param data 数据指针
 * @param len 数据长度
 * @return 0代表成功，其他为错误代码
 */
int iot_input_data(void *iot_ctx,char *data,int len);

/**
 * 请每隔一段时间触发此函数，建议1~3秒
 * 本对象内部需要定时器管理状态
 * @param iot_ctx 对象指针
 * @return 0为成功，-1为失败
 */
int iot_timer_schedule(void *iot_ctx);

/**
 * 获取本次请求req_id
 * @see iot_buffer_start
 * @param iot_ctx
 * @return
 */
int iot_get_request_id(void *iot_ctx);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif //MQTT_MQTT_IOT_H
