//
// Created by xzl on 2019/2/22.
//

#include "mqtt_wrapper.h"
#include "mqtt_buffer.h"
#include "buffer.h"
#include "hash-table.h"
#include <memory.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////
typedef struct{
    //回调指针
    mqtt_callback _callback;
    //只读，最近包id
    unsigned int _pkt_id;
    //私有成员变量，请勿访问
    struct MqttContext _ctx;
    struct MqttBuffer _buffer;
    buffer _remain_data;
    //hash树回调列表
    HashTable *_req_cb_map;
    //心跳包相关
    int _keep_alive;
    time_t _last_ping;
} mqtt_context;


typedef enum {
    res_pub_ack = 0,
    res_sub_ack,
    res_unsub_ack,
} res_type;

typedef struct {
    void *_user_data;
    free_user_data _free_user_data;
    union {
        mqtt_handle_pub_ack _mqtt_handle_pub_ack;
        mqtt_handle_sub_ack _mqtt_handle_sub_ack;
        mqtt_handle_unsub_ack _mqtt_handle_unsub_ack;
    } _callback;
    res_type _cb_type;
    time_t _end_time_line;
} mqtt_req_cb_value;


//////////////////////////////////////////////////////////////////////
static mqtt_req_cb_value *lookup_req_cb_value(mqtt_context *ctx,uint16_t pkt_id){
    return (mqtt_req_cb_value *)hash_table_lookup(ctx->_req_cb_map,(HashTableKey)pkt_id);
}

/**
 * 获取hash表key的hash值
 * @param value
 * @return
 */
static unsigned int mqtt_HashTableHashFunc(HashTableKey value){
    unsigned int pkt_id = (unsigned int)value;
    return pkt_id;
}

/**
 * 判断两个key是否相等
 * @param value1
 * @param value2
 * @return
 */
static int mqtt_HashTableEqualFunc(HashTableKey value1, HashTableKey value2){
    return value1 == value2;
}

static void mqtt_HashTableValueFreeFunc(HashTableValue value){
    mqtt_req_cb_value *cb = (mqtt_req_cb_value *)value;
    if(cb->_free_user_data && cb->_user_data){
        cb->_free_user_data(cb->_user_data);
    }
    free(cb);
}

//////////////////////////////////////////////////////////////////////
static int mqtt_write_sock(void *arg, const struct iovec *iov, int iovcnt){
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    CHECK_PTR(ctx->_callback.mqtt_data_output);
    return ctx->_callback.mqtt_data_output(ctx->_callback._user_data,iov,iovcnt);
}

static int handle_ping_resp(void *arg){/**< 处理ping响应的回调函数，成功则返回非负数 */
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("");
    CHECK_PTR(ctx->_callback.mqtt_handle_ping_resp);
    ctx->_callback.mqtt_handle_ping_resp(ctx->_callback._user_data);
    ctx->_last_ping = time(NULL);
    return 0;
}


static int handle_conn_ack(void *arg, char flags, char ret_code){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("flags:%d , ret_code:%d",(int)flags,(int)(ret_code));
    CHECK_PTR(ctx->_callback.mqtt_handle_conn_ack);
    ctx->_callback.mqtt_handle_conn_ack(ctx->_callback._user_data,flags,ret_code);
    return 0;
}

static int handle_publish(void *arg,
                          uint16_t pkt_id,
                          const char *topic,
                          const char *payload,
                          uint32_t payloadsize,
                          int dup,
                          enum MqttQosLevel qos){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("pkt_id:%d , topic: %s , payload:%s , dup:%d , qos:%d",(int)pkt_id,topic,payload,dup,(int)qos);
    CHECK_PTR(ctx->_callback.mqtt_handle_publish);
    ctx->_callback.mqtt_handle_publish(ctx->_callback._user_data,pkt_id,topic,payload,payloadsize,dup,qos);
    return 0;
}

/**
 * 服务器发布消息给我，然后移除消息，最终态
 * @param arg
 * @param pkt_id
 * @return
 */
static int handle_pub_rel(void *arg, uint16_t pkt_id){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("pkt_id: %d",(int)pkt_id);
    CHECK_PTR(ctx->_callback.mqtt_handle_publish_rel);
    ctx->_callback.mqtt_handle_publish_rel(ctx->_callback._user_data,pkt_id);
    return 0;
}

//////////////////////////////////////////////////////////////////////
static int handle_pub_ack(void *arg, uint16_t pkt_id){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("pkt_id: %d",(int)pkt_id);

    mqtt_req_cb_value *value = lookup_req_cb_value(ctx,pkt_id);
    if(!value){
        LOGW("can not find callback!");
        return 0;
    }
    if(value->_callback._mqtt_handle_pub_ack){
        value->_callback._mqtt_handle_pub_ack(value->_user_data,0,pub_ack);
    }
    hash_table_remove(ctx->_req_cb_map,(HashTableKey)pkt_id);
    return 0;
}

/**
 * 服务器收到我发布的消息，中间态
 * @param arg
 * @param pkt_id
 * @return
 */
static int handle_pub_rec(void *arg, uint16_t pkt_id){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("pkt_id: %d",(int)pkt_id);

    mqtt_req_cb_value *value = lookup_req_cb_value(ctx,pkt_id);
    if(!value){
        LOGW("can not find callback!");
        return 0;
    }
    if(value->_callback._mqtt_handle_pub_ack){
        value->_callback._mqtt_handle_pub_ack(value->_user_data,0,pub_rec);
    }
    //中间态不移除监听
//    hash_table_remove(ctx->_req_cb_map,(HashTableKey)pkt_id);
    return 0;
}

/**
 * 服务器完成收到我发送的消息，最终态
 * @param arg
 * @param pkt_id
 * @return
 */
static int handle_pub_comp(void *arg, uint16_t pkt_id){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("pkt_id: %d",(int)pkt_id);

    mqtt_req_cb_value *value = lookup_req_cb_value(ctx,pkt_id);
    if(!value){
        LOGW("can not find callback!");
        return 0;
    }
    if(value->_callback._mqtt_handle_pub_ack){
        value->_callback._mqtt_handle_pub_ack(value->_user_data,0,pub_comp);
    }
    hash_table_remove(ctx->_req_cb_map,(HashTableKey)pkt_id);
    return 0;
}

static int handle_sub_ack(void *arg, uint16_t pkt_id,const char *codes, uint32_t count){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("pkt_id: %d , codes:%s , count:%d ",(int)pkt_id,codes,(int)count);

    mqtt_req_cb_value *value = lookup_req_cb_value(ctx,pkt_id);
    if(!value){
        LOGW("can not find callback!");
        return 0;
    }
    if(value->_callback._mqtt_handle_sub_ack){
        value->_callback._mqtt_handle_sub_ack(value->_user_data,0,codes,count);
    }
    hash_table_remove(ctx->_req_cb_map,(HashTableKey)pkt_id);
    return 0;
}

static int handle_unsub_ack(void *arg, uint16_t pkt_id){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("pkt_id: %d",(int)pkt_id);

    mqtt_req_cb_value *value = lookup_req_cb_value(ctx,pkt_id);
    if(!value){
        LOGW("can not find callback!");
        return 0;
    }
    if(value->_callback._mqtt_handle_unsub_ack){
        value->_callback._mqtt_handle_unsub_ack(value->_user_data,0);
    }
    hash_table_remove(ctx->_req_cb_map,(HashTableKey)pkt_id);
    return 0;
}

//////////////////////////////////////////////////////////////////////
void *mqtt_alloc_contex(mqtt_callback callback){
    mqtt_context *ctx = (mqtt_context *)malloc(sizeof(mqtt_context));
    if(!ctx){
        LOGE("malloc mqtt_context failed!");
        return NULL;
    }
    memset(ctx,0, sizeof(mqtt_context));
    memcpy(ctx,&callback, sizeof(callback));

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

    MqttBuffer_Init(&ctx->_buffer);
    buffer_init(&ctx->_remain_data);

    ctx->_req_cb_map = hash_table_new(mqtt_HashTableHashFunc,mqtt_HashTableEqualFunc);
    if(!ctx->_req_cb_map){
        LOGE("malloc hash_table_new failed!");
        mqtt_free_contex(ctx);
        return NULL;
    }
    hash_table_register_free_functions(ctx->_req_cb_map,NULL,mqtt_HashTableValueFreeFunc);
    return ctx;
}

void for_each_map(HashTable *hash_table,int flush);

int mqtt_free_contex(void *arg){
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    MqttBuffer_Destroy(&ctx->_buffer);
    buffer_release(&ctx->_remain_data);
    for_each_map(ctx->_req_cb_map,1);
    hash_table_free(ctx->_req_cb_map);
    free(ctx);
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
        LOGW("Mqtt_RecvPkt failed:%d", ret);
        return ret;
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

    ctx->_keep_alive = keep_alive;
    ctx->_last_ping = time(NULL);
    return 0;
}

int mqtt_send_publish_pkt(void *arg,
                          const char *topic,
                          const char *payload,
                          int payload_len,
                          enum MqttQosLevel qos,
                          int retain,
                          int dup,
                          mqtt_handle_pub_ack cb,
                          void *user_data,
                          free_user_data free_cb,
                          int timeout_sec){
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

    mqtt_req_cb_value *value = malloc(sizeof(mqtt_req_cb_value));
    if(value){
        value->_user_data = user_data;
        value->_free_user_data = free_cb;
        value->_callback._mqtt_handle_pub_ack = cb;
        value->_cb_type = res_pub_ack;
        value->_end_time_line = time(NULL) + timeout_sec;
        hash_table_insert(ctx->_req_cb_map,(HashTableKey)ctx->_pkt_id,value);
    }
    return 0;
}


int mqtt_send_subscribe_pkt(void *arg,
                            enum MqttQosLevel qos,
                            const char *const *topics,
                            int topics_len,
                            mqtt_handle_sub_ack cb,
                            void *user_data,
                            free_user_data free_cb,
                            int timeout_sec){
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    CHECK_RET(-1,Mqtt_PackSubscribePkt(&ctx->_buffer,
                                       ++ctx->_pkt_id,
                                       qos,
                                       topics,
                                       topics_len));
    CHECK_RET(-1,mqtt_send_packet(ctx));

    mqtt_req_cb_value *value = malloc(sizeof(mqtt_req_cb_value));
    if(value){
        value->_user_data = user_data;
        value->_free_user_data = free_cb;
        value->_callback._mqtt_handle_sub_ack = cb;
        value->_cb_type = res_sub_ack;
        value->_end_time_line = time(NULL) + timeout_sec;
        hash_table_insert(ctx->_req_cb_map,(HashTableKey)ctx->_pkt_id,value);
    }
    return 0;
}

int mqtt_send_unsubscribe_pkt(void *arg,
                              const char *const *topics,
                              int topics_len,
                              mqtt_handle_unsub_ack cb,
                              void *user_data,
                              free_user_data free_cb,
                              int timeout_sec){
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    CHECK_RET(-1,Mqtt_PackUnsubscribePkt(&ctx->_buffer,
                                         ++ctx->_pkt_id,
                                         topics,
                                         topics_len));
    CHECK_RET(-1,mqtt_send_packet(ctx));

    mqtt_req_cb_value *value = malloc(sizeof(mqtt_req_cb_value));
    if(value){
        value->_user_data = user_data;
        value->_free_user_data = free_cb;
        value->_callback._mqtt_handle_unsub_ack = cb;
        value->_cb_type = res_unsub_ack;
        value->_end_time_line = time(NULL) + timeout_sec;
        hash_table_insert(ctx->_req_cb_map,(HashTableKey)ctx->_pkt_id,value);
    }
    return 0;
}


/**
 * 发送心跳包
 * @param ctx mqtt客户端对象
 * @return 0代表成功，否则为错误代码，@see MqttError
 */
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


void for_each_map(HashTable *hash_table,int flush){
    time_t now = time(NULL);
    HashTableIterator it;
    hash_table_iterate(hash_table,&it);
    while(hash_table_iter_has_more(&it)) {
        HashTablePair pr = hash_table_iter_next(&it);
        mqtt_req_cb_value *value = (mqtt_req_cb_value *)pr.value;
        if(value->_end_time_line > now && !flush){
            continue;
        }

        LOGT("wait response callback timeouted:%d",value->_cb_type);
        //触发超时回调
        switch (value->_cb_type){
            case res_pub_ack:
                value->_callback._mqtt_handle_pub_ack(value->_user_data,1,pub_invalid);
                break;
            case res_sub_ack:
                value->_callback._mqtt_handle_sub_ack(value->_user_data,1,NULL,0);
                break;
            case res_unsub_ack:
                value->_callback._mqtt_handle_unsub_ack(value->_user_data,1);
                break;

            default:
                LOGE("bad response callback type:%d",(int)value->_cb_type);
                break;
        }
        hash_table_remove(hash_table,pr.key);
    }
}

int mqtt_timer_schedule(void *arg){
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_PTR(ctx);
    for_each_map(ctx->_req_cb_map,0);

    time_t now = time(NULL);
    if(now - ctx->_last_ping > ctx->_keep_alive){
        return mqtt_send_ping_pkt(arg);
    }
    return 0;
}