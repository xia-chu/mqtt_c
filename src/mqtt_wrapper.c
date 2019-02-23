//
// Created by xzl on 2019/2/22.
//

#include "mqtt_wrapper.h"
#include "mqtt_buffer.h"
#include "buffer.h"
#include <memory.h>
#include <stdlib.h>

typedef struct{
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

static int mqtt_write_sock(void *arg, const struct iovec *iov, int iovcnt){
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    CHECK_PTR(ctx->_callback.mqtt_data_output);
    return ctx->_callback.mqtt_data_output(ctx->_user_data,iov,iovcnt);
}

static int handle_ping_resp(void *arg){/**< 处理ping响应的回调函数，成功则返回非负数 */
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("\r\n");
    CHECK_PTR(ctx->_callback.mqtt_handle_ping_resp);
    return ctx->_callback.mqtt_handle_ping_resp(ctx->_user_data);
}


static int handle_conn_ack(void *arg, char flags, char ret_code){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("flags:%d , ret_code:%d\r\n",(int)flags,(int)(ret_code));
    CHECK_PTR(ctx->_callback.mqtt_handle_conn_ack);
    return ctx->_callback.mqtt_handle_conn_ack(ctx->_user_data,flags,ret_code);
}

static int handle_publish(void *arg,
                          uint16_t pkt_id,
                          const char *topic,
                          const char *payload,
                          uint32_t payloadsize,
                          int dup,
                          enum MqttQosLevel qos){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("pkt_id:%d , topic: %s , payload:%s , dup:%d , qos:%d\r\n",(int)pkt_id,topic,payload,dup,(int)qos);
    CHECK_PTR(ctx->_callback.mqtt_handle_publish);
    return ctx->_callback.mqtt_handle_publish(ctx->_user_data,pkt_id,topic,payload,payloadsize,dup,qos);
}

static int handle_pub_ack(void *arg, uint16_t pkt_id){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("pkt_id: %d\r\n",(int)pkt_id);
    CHECK_PTR(ctx->_callback.mqtt_handle_pub_ack);
    return ctx->_callback.mqtt_handle_pub_ack(ctx->_user_data,pkt_id);
}

static int handle_pub_rec(void *arg, uint16_t pkt_id){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("pkt_id: %d\r\n",(int)pkt_id);
    CHECK_PTR(ctx->_callback.mqtt_handle_pub_rec);
    return ctx->_callback.mqtt_handle_pub_rec(ctx->_user_data,pkt_id);
}

static int handle_pub_rel(void *arg, uint16_t pkt_id){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("pkt_id: %d\r\n",(int)pkt_id);
    CHECK_PTR(ctx->_callback.mqtt_handle_pub_rel);
    return ctx->_callback.mqtt_handle_pub_rel(ctx->_user_data,pkt_id);
}

static int handle_pub_comp(void *arg, uint16_t pkt_id){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("pkt_id: %d\r\n",(int)pkt_id);
    CHECK_PTR(ctx->_callback.mqtt_handle_pub_comp);
    return ctx->_callback.mqtt_handle_pub_comp(ctx->_user_data,pkt_id);
}

static int handle_sub_ack(void *arg, uint16_t pkt_id,const char *codes, uint32_t count){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("pkt_id: %d , codes:%s , count:%d \r\n",(int)pkt_id,codes,(int)count);
    CHECK_PTR(ctx->_callback.mqtt_handle_sub_ack);
    return ctx->_callback.mqtt_handle_sub_ack(ctx->_user_data,pkt_id,codes,count);
}

static int handle_unsub_ack(void *arg, uint16_t pkt_id){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("pkt_id: %d\r\n",(int)pkt_id);
    CHECK_PTR(ctx->_callback.mqtt_handle_unsub_ack);
    return ctx->_callback.mqtt_handle_unsub_ack(ctx->_user_data,pkt_id);
}

void *mqtt_alloc_contex(mqtt_callback callback,void *user_data){
    mqtt_context *ctx = (mqtt_context *)malloc(sizeof(mqtt_context));
    if(!ctx){
        return NULL;
    }
    memset(ctx,0, sizeof(mqtt_context));
    memcpy(ctx,&callback, sizeof(callback));
    ctx->_user_data = user_data;

    MqttBuffer_Init(&ctx->_buffer);
    ctx->_ctx.user_data = ctx;
    ctx->_ctx.writev_func = mqtt_write_sock;
    ctx->_ctx.handle_ping_resp = handle_ping_resp;
    ctx->_ctx.handle_conn_ack = handle_conn_ack;
    ctx->_ctx.handle_publish = handle_publish;
    ctx->_ctx.handle_pub_ack = handle_pub_ack;
    ctx->_ctx.handle_pub_rec = handle_pub_rec;
    ctx->_ctx.handle_pub_rel = handle_pub_rel;
    ctx->_ctx.handle_pub_comp = handle_pub_comp;
    ctx->_ctx.handle_sub_ack = handle_sub_ack;
    ctx->_ctx.handle_unsub_ack = handle_unsub_ack;
    buffer_init(&ctx->_remain_data);
    return ctx;
}

int mqtt_free_contex(void *arg){
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    MqttBuffer_Destroy(&ctx->_buffer);
    memset(ctx,0, sizeof(mqtt_context));
    return 0;
}

int mqtt_send_packet(void *arg){
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    CHECK_RET(0,Mqtt_SendPkt(&ctx->_ctx,&ctx->_buffer,0));
    MqttBuffer_Reset(&ctx->_buffer);
    return 0;
}


int mqtt_input_data_l(void *arg,char *data,int len) {
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    int customed = 0;
    int ret = Mqtt_RecvPkt(&ctx->_ctx,data,len,&customed);
    if(ret != MQTTERR_NOERROR){
        LOGW("Mqtt_RecvPkt failed:%d\r\n", ret);
        return -1;
    }

    if(customed == 0){
        //完全为消费数据
        if(!ctx->_remain_data._len){
            buffer_assign(&ctx->_remain_data,data,len);
        }
        return 0;
    }
    if(customed < len){
        //消费部分数据
        buffer buffer;
        buffer_assign(&buffer,data + customed,len - customed);
        buffer_move(&ctx->_remain_data,&buffer);
        return 0;
    }
    //消费全部数据
    buffer_release(&ctx->_remain_data);
    return 0;
}

int mqtt_input_data(void *arg,char *data,int len){
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    if(!ctx->_remain_data._len){
        return mqtt_input_data_l(ctx,data,len);
    }
    buffer_append(&ctx->_remain_data,data,len);
    return mqtt_input_data_l(ctx,ctx->_remain_data._data,ctx->_remain_data._len);
}

int mqtt_send_connect_pkt(void *arg,
                          int keep_alive,
                          const char *id,
                          int clean_session,
                          const char *will_topic,
                          const char *will_payload,
                          int will_payload_len,
                          enum MqttQosLevel qos,
                          int will_retain,
                          const char *user,
                          const char *password){
    if(will_payload_len <= 0){
        will_payload_len = strlen(will_payload);
    }
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    CHECK_RET(-1,Mqtt_PackConnectPkt(&ctx->_buffer,
                                     keep_alive,
                                     id,
                                     clean_session,
                                     will_topic,
                                     will_payload,
                                     will_payload_len,
                                     qos,
                                     will_retain,
                                     user,
                                     password,
                                     strlen(password)));
    CHECK_RET(-1,mqtt_send_packet(ctx));
    return 0;
}

int mqtt_send_publish_pkt(void *arg,
                          const char *topic,
                          const char *payload,
                          int payload_len,
                          enum MqttQosLevel qos,
                          int retain,
                          int dup){
    if(payload_len <= 0){
        payload_len = strlen(payload);
    }
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    CHECK_RET(-1,Mqtt_PackPublishPkt(&ctx->_buffer,
                                     ++ctx->_pkt_id,
                                     topic,
                                     payload,
                                     payload_len,
                                     qos,
                                     retain,
                                     1));
    if(dup){
        CHECK_RET(-1,Mqtt_SetPktDup(&ctx->_buffer));
    }
    CHECK_RET(-1,mqtt_send_packet(ctx));
    return 0;
}


int mqtt_send_subscribe_pkt(void *arg,
                            enum MqttQosLevel qos,
                            const char *const *topics,
                            int topics_len){
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    CHECK_RET(-1,Mqtt_PackSubscribePkt(&ctx->_buffer,
                                       ++ctx->_pkt_id,
                                       qos,
                                       topics,
                                       topics_len));
    CHECK_RET(-1,mqtt_send_packet(ctx));
    return 0;
}

int mqtt_send_unsubscribe_pkt(void *arg,
                            const char *const *topics,
                            int topics_len){
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    CHECK_RET(-1,Mqtt_PackUnsubscribePkt(&ctx->_buffer,
                                         ++ctx->_pkt_id,
                                         topics,
                                         topics_len));
    CHECK_RET(-1,mqtt_send_packet(ctx));
    return 0;
}


int mqtt_send_ping_pkt(void *arg){
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    CHECK_RET(-1,Mqtt_PackPingReqPkt(&ctx->_buffer));
    CHECK_RET(-1,mqtt_send_packet(ctx));
    return 0;
}

int mqtt_send_disconnect_pkt(void *arg){
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    CHECK_RET(-1,Mqtt_PackDisconnectPkt(&ctx->_buffer));
    CHECK_RET(-1,mqtt_send_packet(ctx));
    return 0;
}
