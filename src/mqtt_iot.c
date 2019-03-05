//
// Created by xzl on 2019/3/4.
//

#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include "mqtt_iot.h"
#include "buffer.h"
#include "md5.h"
#include "mqtt_wrapper.h"
#include "iot_proto.h"
#include "base64.h"

#define KEEP_ALIVE_SEC 60

extern void dump_iot_pack(const uint8_t *in,int size);


typedef struct {
    iot_callback _callback;
    void *_mqtt_context;
    buffer _topic_publish;
    buffer _topic_listen;
    int _req_id;
} iot_context;

static int iot_data_output(void *arg, const struct iovec *iov, int iovcnt){
    iot_context *ctx = (iot_context *)arg;
    if(ctx->_callback.iot_on_output){
        return ctx->_callback.iot_on_output(ctx->_callback._user_data,iov,iovcnt);
    }
    return -1;
}

static void iot_on_subscribe(void *arg,int time_out,const char *codes, uint32_t count){
    iot_context *ctx = (iot_context *)arg;
    int code = -2;
    do {
        if (time_out) {
            LOGE("wait subscribe response timeout!");
            break;
        }
        if (count != 1) {
            LOGE("bad subscribe response ,codes count=:%d",count);
            break;
        }
        code = codes[0];
    }while(0);

    //订阅成功了才认为登录成功
    if(ctx->_callback.iot_on_connect){
        ctx->_callback.iot_on_connect(ctx->_callback._user_data,code);
    }
}
static void iot_on_connect_cb(void *arg, char flags, char ret_code){
    iot_context *ctx = (iot_context *)arg;
    while (ret_code == 0){
        LOGI("connect mqtt server success!");
        const char *topics[] = {ctx->_topic_listen._data};
        if(-1 != mqtt_send_subscribe_pkt(ctx->_mqtt_context,
                                         MQTT_QOS_LEVEL1,
                                         topics,
                                         1,
                                         iot_on_subscribe,
                                         ctx,
                                         NULL,
                                         10)){
            //开始订阅成功！
            return;
        }
        LOGE("mqtt_send_subscribe_pkt failed!");
        //开始订阅失败，回调错误
        ret_code = -1;
        break;
    }

    if(ctx->_callback.iot_on_connect){
        ctx->_callback.iot_on_connect(ctx->_callback._user_data,ret_code);
    }
}

static void iot_on_ping_resp(void *arg){}
static void iot_on_publish_rel(void *arg, uint16_t pkt_id){}

static void iot_on_publish(void *arg,
                           uint16_t pkt_id,
                           const char *topic,
                           const char *payload,
                           uint32_t payloadsize,
                           int dup,
                           enum MqttQosLevel qos){
    iot_context *ctx = (iot_context *)arg;
    if(ctx->_callback.iot_on_message){
        int buf_size = payloadsize * 3 / 4 +10;
        uint8_t *out = malloc(buf_size);
        int size = av_base64_decode(out,buf_size,payload,payloadsize);
        dump_iot_pack(out, size);
        free(out);
    }
}


void *iot_context_alloc(iot_callback *cb){
    iot_context *ctx = (iot_context *)malloc(sizeof(iot_context));
    if(!ctx){
        LOGE("malloc iot_context failed!");
        return NULL;
    }
    memset(ctx,0, sizeof(iot_context));
    memcpy(ctx,cb, sizeof(mqtt_callback));

    mqtt_callback callback = {iot_data_output,iot_on_connect_cb,iot_on_ping_resp,iot_on_publish,iot_on_publish_rel,ctx};
    ctx->_mqtt_context = mqtt_alloc_contex(&callback);
    return ctx;
}

int iot_context_free(void *arg){
    iot_context *ctx = (iot_context *)arg;
    CHECK_PTR(ctx,-1);
    if(ctx->_mqtt_context){
        mqtt_free_contex(ctx->_mqtt_context);
        ctx->_mqtt_context = NULL;
    }
    buffer_release(&ctx->_topic_publish);
    buffer_release(&ctx->_topic_listen);
    free(ctx);
    return 0;
}


void make_passwd(const char *client_id,
                 const char *secret,
                 const char *user_name,
                 buffer *md5_str_buf,
                 int upCase){
    char passwd[33] = {0};
    uint8_t md5_digst[16];
    buffer buf_plain ;
    buffer_init(&buf_plain);
    buffer_append(&buf_plain,client_id,0);
    buffer_append(&buf_plain,secret,0);
    buffer_append(&buf_plain,user_name,0);
    md5((uint8_t*)buf_plain._data,buf_plain._len,md5_digst);
    hexdump(md5_digst,MD5_HEX_LEN,passwd, sizeof(passwd),upCase);
    buffer_assign(md5_str_buf,passwd, sizeof(passwd));
    buffer_release(&buf_plain);
}

int iot_send_connect_pkt(void *arg,const char *client_id,const char *secret,const char *user_name){
    iot_context *ctx = (iot_context *)arg;
    CHECK_PTR(ctx,-1);
    CHECK_PTR(client_id,-1);
    CHECK_PTR(secret,-1);
    CHECK_PTR(user_name,-1);

    buffer md5_str;
    buffer_init(&md5_str);
    make_passwd(client_id,secret,user_name,&md5_str,0);
    int ret = mqtt_send_connect_pkt(ctx->_mqtt_context,KEEP_ALIVE_SEC,client_id,1,NULL,NULL,0,MQTT_QOS_LEVEL1, 0,user_name,md5_str._data);
    buffer_release(&md5_str);

    CHECK_RET(-1,buffer_assign(&ctx->_topic_listen,"/terminal/",0));
    CHECK_RET(-1,buffer_append(&ctx->_topic_listen,client_id,0));

    CHECK_RET(-1,buffer_assign(&ctx->_topic_publish,"/service/",0));
    CHECK_RET(-1,buffer_append(&ctx->_topic_publish,user_name,0));
    CHECK_RET(-1,buffer_append(&ctx->_topic_publish,"/",0));
    CHECK_RET(-1,buffer_append(&ctx->_topic_publish,client_id,0));
    return ret;
}

static void mqtt_pub_ack(void *arg,int time_out,pub_type type){
    iot_context *ctx = (iot_context *)arg;
//    LOGT("time_out=%d,type=%d",time_out,type);
}

int iot_send_raw_bytes(iot_context *ctx,unsigned char *iot_buf,int iot_len){
    int base64_size = AV_BASE64_SIZE(iot_len) + 10;
    char *base64 = malloc(base64_size);
    if(!base64){
        LOGE("memory overflow!");
        return -1;
    }
    if(NULL == av_base64_encode(base64,base64_size,iot_buf,iot_len)){
        free(base64);
        LOGE("av_base64_encode failed!");
        return -1;
    }
    int ret = mqtt_send_publish_pkt(ctx->_mqtt_context,
                                    ctx->_topic_publish._data,//topic
                                    (const char *)base64,//payload
                                    0,//payload_len
                                    MQTT_QOS_LEVEL1,//qos
                                    0,//retain
                                    0,//dup
                                    mqtt_pub_ack,//mqtt_handle_pub_ack
                                    ctx,//user_data
                                    NULL,//free_user_data
                                    10);//timeout_sec
    free(base64);
    return ret;
}

int iot_send_bool_pkt(void *arg,int tag,int flag){
    iot_context *ctx = (iot_context *)arg;
    CHECK_PTR(ctx,-1);
    unsigned char iot_buf[16] = {0};
    int iot_len = pack_iot_bool_packet(1,1,++ctx->_req_id,tag,flag,iot_buf, sizeof(iot_buf));
    if(iot_len <= 0) {
        LOGE("pack_iot_bool_packet failed:%d",iot_len);
        return -1;
    }
    return iot_send_raw_bytes(ctx,iot_buf,iot_len);
}

int iot_send_double_pkt(void *arg,int tag,double double_num){
    iot_context *ctx = (iot_context *)arg;
    CHECK_PTR(ctx,-1);
    unsigned char iot_buf[32] = {0};
    int iot_len = pack_iot_double_packet(1,1,++ctx->_req_id,tag,double_num,iot_buf, sizeof(iot_buf));
    if(iot_len <= 0) {
        LOGE("pack_iot_double_packet failed:%d",iot_len);
        return -1;
    }
    return iot_send_raw_bytes(ctx,iot_buf,iot_len);
}

int iot_send_enum_pkt(void *arg,int tag,const char *enum_str){
    iot_context *ctx = (iot_context *)arg;
    CHECK_PTR(ctx,-1);
    int iot_buf_size = 16 + strlen(enum_str);
    unsigned char *iot_buf = malloc(16 + strlen(enum_str));
    int iot_len = pack_iot_enum_packet(1,1,++ctx->_req_id,tag,enum_str,iot_buf, iot_buf_size);
    if(iot_len <= 0) {
        LOGE("pack_iot_enum_packet failed:%d",iot_len);
        free(iot_buf);
        return -1;
    }
    int ret = iot_send_raw_bytes(ctx,iot_buf,iot_len);
    free(iot_buf);
    return ret;
}

int iot_send_string_pkt(void *arg,int tag,const char *str){
    iot_context *ctx = (iot_context *)arg;
    CHECK_PTR(ctx,-1);
    int iot_buf_size = 16 + strlen(str);
    unsigned char *iot_buf = malloc(iot_buf_size);
    int iot_len = pack_iot_string_packet(1,1,++ctx->_req_id,tag,str,iot_buf, iot_buf_size);
    if(iot_len <= 0) {
        LOGE("pack_iot_string_packet failed:%d",iot_len);
        free(iot_buf);
        return -1;
    }
    int ret = iot_send_raw_bytes(ctx,iot_buf,iot_len);
    free(iot_buf);
    return ret;
}

int iot_send_buffer(void *arg,buffer *buf){
    iot_context *ctx = (iot_context *)arg;
    CHECK_PTR(ctx,-1);
    CHECK_PTR(buf,-1);
    CHECK_PTR(buf->_data,-1);
    return iot_send_raw_bytes(ctx,(unsigned char *)buf->_data,buf->_len);
}


int iot_input_data(void *arg,char *data,int len){
    iot_context *ctx = (iot_context *)arg;
    CHECK_PTR(ctx,-1);
    return mqtt_input_data(ctx->_mqtt_context,data,len);
}

int iot_timer_schedule(void *arg){
    iot_context *ctx = (iot_context *)arg;
    CHECK_PTR(ctx,-1);
    return mqtt_timer_schedule(ctx->_mqtt_context);
}

int iot_get_request_id(void *arg){
    iot_context *ctx = (iot_context *)arg;
    CHECK_PTR(ctx,-1);
    return ++ctx->_req_id;
}