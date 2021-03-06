#ifndef ONENET_MQTT_H
#define ONENET_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#include <stdint.h>
#include <time.h>
#include "jimi_type.h"
#include "mqtt_buffer.h"

/** MQTT错误码 */
enum MqttError {
    MQTTERR_NOERROR                  = 0,  /**< 成功，无错误*/
    MQTTERR_OTHER                    = -1, /**其他错误*/
    MQTTERR_OUTOFMEMORY              = -2, /**< 内存不足 */
    MQTTERR_ILLEGAL_PKT              = -3, /**< 非法的数据包 */
    MQTTERR_ILLEGAL_CHARACTER        = -4, /**< 非法的字符 */
    MQTTERR_NOT_UTF8                 = -5, /**< 字符编码不是UTF-8 */
    MQTTERR_INVALID_PARAMETER        = -6, /**< 参数错误 */
    MQTTERR_PKT_TOO_LARGE            = -7, /**< 数据包过大 */
    MQTTERR_INTERNAL                 = -8,/**< 系统内部错误 */
    MQTTERR_FAILED_SEND_RESPONSE     = -9 /**< 处理publish系列消息后，发送响应包失败 */
};

/** MQTT数据包类型 */
enum MqttPacketType {
    MQTT_PKT_CONNECT = 1, /**< 连接请求数据包 */
    MQTT_PKT_CONNACK,     /**< 连接确认数据包 */
    MQTT_PKT_PUBLISH,     /**< 发布数据数据包 */
    MQTT_PKT_PUBACK,      /**< 发布确认数据包 */
    MQTT_PKT_PUBREC,      /**< 发布数据已接收数据包，Qos 2时，回复MQTT_PKT_PUBLISH */
    MQTT_PKT_PUBREL,      /**< 发布数据释放数据包， Qos 2时，回复MQTT_PKT_PUBREC */
    MQTT_PKT_PUBCOMP,     /**< 发布完成数据包， Qos 2时，回复MQTT_PKT_PUBREL */
    MQTT_PKT_SUBSCRIBE,   /**< 订阅数据包 */
    MQTT_PKT_SUBACK,      /**< 订阅确认数据包 */
    MQTT_PKT_UNSUBSCRIBE, /**< 取消订阅数据包 */
    MQTT_PKT_UNSUBACK,    /**< 取消订阅确认数据包 */
    MQTT_PKT_PINGREQ,     /**< ping 数据包 */
    MQTT_PKT_PINGRESP,    /**< ping 响应数据包 */
    MQTT_PKT_DISCONNECT   /**< 断开连接数据包 */
};

/** MQTT QOS等级 */
enum MqttQosLevel {
    MQTT_QOS_LEVEL0,  /**< 最多发送一次 */
    MQTT_QOS_LEVEL1,  /**< 最少发送一次  */
    MQTT_QOS_LEVEL2   /**< 只发送一次 */
};

/** MQTT 连接请求标志位，内部使用 */
enum MqttConnectFlag {
    MQTT_CONNECT_CLEAN_SESSION  = 0x02,
    MQTT_CONNECT_WILL_FLAG      = 0x04,
    MQTT_CONNECT_WILL_QOS0      = 0x00,
    MQTT_CONNECT_WILL_QOS1      = 0x08,
    MQTT_CONNECT_WILL_QOS2      = 0x10,
    MQTT_CONNECT_WILL_RETAIN    = 0x20,
    MQTT_CONNECT_PASSORD        = 0x40,
    MQTT_CONNECT_USER_NAME      = 0x80
};

/** 连接确认标志位 */
enum MqttConnAckFlag {
    MQTT_CONNACK_SP = 0x01 /**< 保留原来会话，以原会话登陆 */
};

/** MQTT 返回码 */
enum MqttRetCode {
    MQTT_CONNACK_ACCEPTED                  = 0, /**< 连接已建立 */
    MQTT_CONNACK_UNACCEPTABLE_PRO_VERSION  = 1, /**< 服务器不支持该版本的MQTT协议*/
    MQTT_CONNACK_IDENTIFIER_REJECTED       = 2, /**< 不允许的客户端ID */
    MQTT_CONNACK_SERVER_UNAVAILABLE        = 3, /**< 服务器不可用 */
    MQTT_CONNACK_BAD_USER_NAME_OR_PASSWORD = 4, /**< 用户名或密码不合法 */
    MQTT_CONNACK_NOT_AUTHORIZED            = 5, /**< 鉴权失败 */

    MQTT_SUBACK_QOS0    = 0x00,  /**< 订阅确认， QoS等级0 */
    MQTT_SUBACK_QOS1    = 0x01,  /**< 订阅确认， QoS等级1 */
    MQTT_SUBACK_QOS2    = 0x02,  /**< 订阅确认， QoS等级2 */
    MQTT_SUBACK_FAILUER = 0x80   /**< 订阅失败 */
};

/**
 * MQTT长度字段所占长度
 */
enum MqttPacketLength{
    kLengthOutOfRange = -1,
    kLengthNotEnough = -2,
};

/** MQTT 运行时上下文 */
struct MqttContext {
    void *user_data;
    int (*writev_func)(void *arg, const struct iovec *iov, int iovcnt);
        /**<  发送数据的回调函数，其行为类似于 unix中的writev，
              arg是回调函数的关联参数，iovcnt为iov对象的个数，iovec定义如下：
              struct iovec {
                  void *iov_base;
                  size_t iov_len;
              }
			  返回发送的字节数，如果失败返回-1.
        */

    int (*handle_ping_resp)(void *arg); /**< 处理ping响应的回调函数，成功则返回非负数 */

    int (*handle_conn_ack)(void *arg, char flags, char ret_code);
        /**< 处理连接响应的回调函数， flags取值@see MqttConnAckFlag，
             ret_code的值为 @see MqttRetCode， 成功则返回非负数
         */

    int (*handle_publish)(void *arg, uint16_t pkt_id, const char *topic,
                          const char *payload, uint32_t payloadsize,
                          int dup, enum MqttQosLevel qos);
        /**< 处理发布数据的回调函数， pkt_id为数据包的ID，topic为
             数据所属的Topic， payload为数据的起始地址， payloadsize为
             payload的字节数， dup为是否重发状态， qos为QoS等级，成功返回非负数，
			 SDK将会自动发送对应的响应包。
         */

    int (*handle_pub_ack)(void *arg, uint16_t pkt_id);
        /**< 处理发布数据确认的回调，pkt_id为被确认的发布数据数据包的ID，成功则返回非负数 */

    int (*handle_pub_rec)(void *arg, uint16_t pkt_id);
        /**< 处理发布数据已接收的回调函数，pkt_id为布数据已接收数据包的ID，成功则返回非负数 */

    int (*handle_pub_rel)(void *arg, uint16_t pkt_id);
        /**< 处理发布数据已释放的回调函数， pkt_id为发布数据已释放数据包的ID，成功则返回非负数 */

    int (*handle_pub_comp)(void *arg, uint16_t pkt_id);
        /**< 处理发布数据已完成的回调函数，pkt_id为发布数据已完成数据包的ID，成功则返回非负数 */

    int (*handle_sub_ack)(void *arg, uint16_t pkt_id,
                          const char *codes, uint32_t count);
        /**< 处理订阅确认的回调数据， pkt_id为订阅数据包的ID，
             codes为@see MqttRetCode，按顺序对应订阅数据包中的Topic，
             count为codes的个数，成功则返回非负数
         */

    int (*handle_unsub_ack)(void *arg, uint16_t packet_id);
        /**< 处理取消订阅确认的回调函数, pkt_id为取消订阅数据包的ID，成功则返回非负数 */
};


/**
 * 接收数据包，并调用ctx中响应的数据处理函数
 * @param ctx MQTT运行时上下文
 * @param customed 已消费数据长度
 * @return 成功则返回MQTTERR_NOERROR
 */
int Mqtt_RecvPkt(struct MqttContext *ctx,char *data,int len,int *customed);
/**
 * 发送数据包
 * @param buf 保存将要发送数据包的缓冲区对象
 * @param offset 从缓冲区的offset字节处开始发送
 * @return 成功则返回MQTTERR_NOERROR
 */
int Mqtt_SendPkt(struct MqttContext *ctx, const struct MqttBuffer *buf, uint32_t offset);


/**
 * 封装连接请求数据包
 * @param buf 存储数据包的缓冲区对象
 * @param keep_alive 保活时间间隔，单位秒
 * @param id 客户端标识码
 * @param clean_session 为0时，继续使用上一次的会话，若无上次会话则创建新的会话，
 *        为1时，删除上一次的会话，并建立新的会话
 * @param will_topic "遗言"消息发送到的topic
 * @param will_msg "遗言"消息，当服务器发现设备下线时，自动将该消息发送到will_topic
 * @param msg_len "遗言"消息的长度
 * @param qos Qos等级
 * @param will_retain 为0时，服务器发送will_msg后，将删除will_msg，否则将保存will_msg
 * @param user 用户名
 * @param password 密码
 * @param pswd_len 密码长度（字节数）
 * @return 成功则返回MQTTERR_NOERROR
 */
int Mqtt_PackConnectPkt(struct MqttBuffer *buf, uint16_t keep_alive, const char *id,
                        int clean_session, const char *will_topic,
                        const char *will_msg, uint16_t msg_len,
                        enum MqttQosLevel qos, int will_retain, const char *user,
                        const char *password, uint16_t pswd_len);

/**
 * 封装发布数据数据包
 * @param buf 存储数据包的缓冲区对象
 * @param pkt_id 数据包ID，非0
 * @param topic 数据发送到哪个topic
 * @param payload 将要被发布的数据块的起始地址
 * @param size 数据块大小（字节数）
 * @param qos QoS等级
 * @param retain 非0时，服务器将该publish消息保存到topic下，并替换已有的publish消息
 * @param own 非0时，拷贝payload到缓冲区
 * @return 成功则返回MQTTERR_NOERROR
 * @remark 当own为0时，payload必须在buf被销毁或重置前保持有效
 */
int Mqtt_PackPublishPkt(struct MqttBuffer *buf, uint16_t pkt_id, const char *topic,
                        const char *payload, uint32_t size,
                        enum MqttQosLevel qos, int retain, int own);

/**
 * 设置发布数据数据包为重发的发布数据数据包
 * @param buf 存储有PUBLISH数据包的缓冲区
 * @return 成功则返回MQTTERR_NOERROR
 */
int Mqtt_SetPktDup(struct MqttBuffer *buf);

/**
 * 封装订阅数据包
 * @param buf 存储数据包的缓冲区对象
 * @param pkt_id 数据包ID， 非0
 * @param qos QoS等级
 * @param topics 订阅的topic
 * @param topics_len  订阅的topic 个数
 * @return 成功返回MQTTERR_NOERROR
 */
int Mqtt_PackSubscribePkt(struct MqttBuffer *buf, uint16_t pkt_id,
                          enum MqttQosLevel qos, const char *const *topics, int topics_len);

/**
 * 添加需要订阅的Topic到已有的订阅数据包中
 * @param buf 存储订阅数据包的缓冲区对象
 * @param topic  订阅的Topic
 * @param qos QoS等级
 * @return 成功返回MQTTERR_NOERROR
 */

int Mqtt_AppendSubscribeTopic(struct MqttBuffer *buf, const char *topic, enum MqttQosLevel qos);
/**
 * 封装取消订阅数据包
 * @param buf 存储数据包的缓冲区对象
 * @param pkt_id 数据包ID
 * @param topics 将要取消订阅的Topic，不能包含'#'和'+'
 * @param topics_len
 * @return 成功返回MQTTERR_NOERROR
 */
int Mqtt_PackUnsubscribePkt(struct MqttBuffer *buf, uint16_t pkt_id, const char *const *topics, int topics_len);

/**
 * 添加需要取消订阅的Topic到已有的取消订阅数据包中
 * @param buf 存储取消订阅数据包的缓冲区对象
 * @param topic 需要取消的Topic
 * @return 成功返回MQTTERR_NOERROR
 */
int Mqtt_AppendUnsubscribeTopic(struct MqttBuffer *buf, const char *topic);

/**
 * 封装ping数据包
 * @param buf 存储数据包的缓冲区对象
 * @return 成功返回MQTTERR_NOERROR
 */
int Mqtt_PackPingReqPkt(struct MqttBuffer *buf);

/**
 * 封装断开连接数据包
 * @param buf 存储数据包的缓冲区对象
 * @return 成功返回MQTTERR_NOERROR
 */
int Mqtt_PackDisconnectPkt(struct MqttBuffer *buf);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // ONENET_MQTT_H
