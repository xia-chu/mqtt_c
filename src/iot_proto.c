//
// Created by xzl on 2019/2/26.
//

#include "iot_proto.h"
#include "log.h"
#include "mqtt_wrapper.h"
#ifndef __alios__
#include <arpa/inet.h>
#else
#include <aos/network.h>
#endif
#include <memory.h>
#include <stdlib.h>

#define CHECK_LEN(n,tail) \
do{ \
    if(cur_ptr + n >= tail + 1){ \
        LOGW("not enough data buffer,need more bytes: %ld", (cur_ptr + n) - (tail)); \
        return -1; \
    } \
}while(0);


/**
 * 获取数据包大小
 * @param stream 数据流
 * @param size 数据流大小
 * @param len 返回数据包大小
 * @return 返回长度字段占用字节数
 */
extern int Mqtt_ReadLength(const char *stream, int size, uint32_t *len);
extern int Mqtt_DumpLength(size_t len, char *buf);

#ifndef ntohll
unsigned long long ntohll(unsigned long long val){
    if (1 != htons(1)){
        return (((unsigned long long )htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)htonl((int)(val >> 32));
    }
    return val;
}
#endif

#ifndef htonll
unsigned long long htonll(unsigned long long val){
    if (1 != htons(1)){
        return (((unsigned long long )htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)htonl((int)(val >> 32));
    }
    return val;
}
#endif

int static_length_of_type(iot_data_type type){
    switch (type){
        case iot_double:
            return 8;
        case iot_enum:
            return 0;
        case iot_string:
            return 0;
        case iot_bool:
            return 1;
        default:
            LOGE("unsupported iot_data_type: %d",(int)type);
            return 0;
    }
}

#define PUT_HEADER(cur_ptr,data_out_tail,req_id,req_flag) \
     //控制位，表明是请求还是回复包 \
    CHECK_LEN(1, data_out_tail); \
    cur_ptr[0] = 0x01 & req_flag; \
    cur_ptr += 1; \

    //request id，4个字节 \
    CHECK_LEN(4, data_out_tail);\
    req_id = htonl(req_id);\
    memcpy(cur_ptr, &req_id, 4);\
    cur_ptr += 4;

int pack_iot_packet(int with_head,
                    uint8_t req_flag,
                    uint32_t req_id,
                    uint32_t tag_id,
                    iot_data_type type,
                    const unsigned char *data_in,
                    int in_len,
                    unsigned char *data_out,
                    int out_len) {
    unsigned char *cur_ptr = data_out;
    unsigned char *data_out_tail = data_out + out_len;

    if(with_head) {
        PUT_HEADER(cur_ptr,data_out_tail,req_id,req_flag);
    }

    //tag
    CHECK_LEN(4, data_out_tail);
    cur_ptr += Mqtt_DumpLength(tag_id, (char *) cur_ptr);

    //type
    CHECK_LEN(4, data_out_tail);
    cur_ptr += Mqtt_DumpLength((uint32_t) type, (char *) cur_ptr);

    //消息长度
    if (!static_length_of_type(type)) {
        CHECK_PTR(data_in, -1);
        if (in_len <= 0) {
            in_len = strlen((char *) data_in) + 1;
        }
        CHECK_LEN(4, data_out_tail);
        cur_ptr += Mqtt_DumpLength((uint32_t) in_len, (char *) cur_ptr);
    }

    //消息体
    CHECK_LEN(in_len,data_out_tail);
    memcpy(cur_ptr,data_in,in_len);
    cur_ptr += in_len;
    return cur_ptr - data_out;
}

int pack_iot_bool_packet(int with_head,
                         int req_flag,
                         uint32_t req_id,
                         uint32_t tag_id,
                         uint8_t bool_flag,
                         unsigned char *data_out,
                         int out_len){
    bool_flag &= 0x01;
    return pack_iot_packet(with_head,req_flag,req_id,tag_id,iot_bool,&bool_flag, 1,data_out,out_len);
}

int pack_iot_enum_packet(int with_head,
                         int req_flag,
                         uint32_t req_id,
                         uint32_t tag_id,
                         const char *enum_str,
                         unsigned char *data_out,
                         int out_len){
    return pack_iot_packet(with_head,req_flag,req_id,tag_id,iot_enum,(unsigned char *)enum_str, strlen(enum_str) + 1,data_out,out_len);
}

int pack_iot_string_packet(int with_head,
                           int req_flag,
                           uint32_t req_id,
                           uint32_t tag_id,
                           const char *str,
                           unsigned char *data_out,
                           int out_len){
    return pack_iot_packet(with_head,req_flag,req_id,tag_id,iot_string,(unsigned char *)str, strlen(str) + 1,data_out,out_len);
}

int pack_iot_double_packet(int with_head,
                           int req_flag,
                           uint32_t req_id,
                           uint32_t tag_id,
                           double double_num,
                           unsigned char *data_out,
                           int out_len){
    uint64_t buf;
    memcpy(&buf,&double_num,8);
    buf = htonll(buf);
    return pack_iot_packet(with_head,req_flag,req_id,tag_id,iot_double,(unsigned char *)&buf, 8,data_out,out_len);
}


int unpack_iot_packet(uint8_t *req_flag,
                      uint32_t *req_id,
                      uint32_t *tag_id,
                      iot_data_type *type,
                      const unsigned char *data_in,
                      int in_len,
                      const unsigned char **content){

    const unsigned char *cur_ptr = data_in;
    const unsigned char *data_in_tail = data_in + in_len;

    if(req_flag){
        //控制位，表明是请求还是回复包
        CHECK_LEN(1,data_in_tail);
        *req_flag = cur_ptr[0] & 0x1F;
        cur_ptr += 1;
    }

    if(req_id){
        //request id，4个字节
        CHECK_LEN(4,data_in_tail);
        memcpy(req_id,cur_ptr, 4);
        *req_id = ntohl(*req_id);
        cur_ptr += 4;
    }

    //tag
    do {
        CHECK_LEN(1,data_in_tail);
        int tag_len = Mqtt_ReadLength((const char *) cur_ptr, 4, tag_id);
        if (tag_len <= 0) {
            LOGW("invalid TAG field:%d %d", tag_len, *tag_id);
            return -1;
        }
        cur_ptr += tag_len;
    }while(0);

    //type
    do {
        CHECK_LEN(1,data_in_tail);
        uint32_t type_i = 0;
        int type_len = Mqtt_ReadLength((const char *) cur_ptr, 4, &type_i);
        if (type_len <= 0) {
            LOGW("invalid TYPE field:%d %d", type_len, type_i);
            return -1;
        }
        cur_ptr += type_len;
        *type = (iot_data_type)type_i;
    }while (0);

    //消息长度
    uint32_t content_len = static_length_of_type(*type);
    if (!content_len) {
        do {
            CHECK_LEN(1,data_in_tail);
            int content_len_len = Mqtt_ReadLength((const char *) cur_ptr, 4, &content_len);
            if (content_len_len <= 0) {
                LOGW("invalid content_len field:%d %d", content_len_len, content_len);
                return -1;
            }
            cur_ptr += content_len_len;
        }while (0);
    }
    *content = cur_ptr;
    return content_len;
}

#define CHECK_TYPE_LEN(a,b) \
    do{ \
        if(a != b){ \
            LOGW("invalid length:%d != %d",a,b); \
            return; \
        } \
    }while(0);


double to_double(const unsigned char *data_in){
    uint64_t buf;
    memcpy(&buf,data_in,8);
    buf = ntohll(buf);
    double db;
    memcpy(&db,&buf,8);
    return db;
}
void dump_iot_pack(const uint8_t *in,int size){
    uint8_t req_flag;
    uint32_t req_id;
    uint32_t tag_id;
    iot_data_type type;
    const unsigned char *content;
    int content_len;

    const uint8_t *ptr = in;
    while (ptr < in + size){
        int remain = in + size - ptr;
        if(ptr == in){
            content_len = unpack_iot_packet(&req_flag,&req_id,&tag_id,&type,ptr,remain,&content);
        }else{
            content_len = unpack_iot_packet(NULL,NULL,&tag_id,&type,ptr,remain,&content);
        }
        if(content_len == -1){
            LOGE("unpack_iot_packet failed!");
            break;
        }
        ptr = content + content_len;

        switch (type){
            case iot_bool:
                CHECK_TYPE_LEN(content_len , 1);
                LOGD("req_flag:%d , req_id:%d , tag_id:%d , type:%d , bool:%d",req_flag,req_id,tag_id,type,*((uint8_t*)content));
                break;
            case iot_string:
                LOGD("req_flag:%d , req_id:%d , tag_id:%d , type:%d , string:%s",req_flag,req_id,tag_id,type,(const char *)content);
                break;
            case iot_enum:
                LOGD("req_flag:%d , req_id:%d , tag_id:%d , type:%d , enum:%s",req_flag,req_id,tag_id,type,(const char *)content);
                break;
            case iot_double:
                CHECK_TYPE_LEN(content_len , 8);
                LOGD("req_flag:%d , req_id:%d , tag_id:%d , type:%d , double:%f",req_flag,req_id,tag_id,type,to_double(content));
                break;
        }
    }

}
void test_iot_packet(){
    do {
        unsigned char out_buffer[1024] = {0};
        int out_len = 0;
        out_len += pack_iot_string_packet(1,1, 1234, 4567, "this is a iot packet", out_buffer + out_len, sizeof(out_buffer) - out_len);
        out_len += pack_iot_bool_packet(0,1, 1234, 4567, 1, out_buffer + out_len, sizeof(out_buffer) - out_len);
        out_len += pack_iot_enum_packet(0,1, 1234, 4567, "enum 0", out_buffer + out_len, sizeof(out_buffer) - out_len);
        out_len += pack_iot_double_packet(0,1, 1234, 4567, 3.1415, out_buffer + out_len, sizeof(out_buffer) - out_len);

        LOGD("pack_iot_packet ret:%d", out_len);
        dump_iot_pack(out_buffer, out_len);
    }while (0);

}



int iot_buffer_header(buffer *buffer,
                      int req_flag,
                      uint32_t req_id){

    uint8_t data_out[8];
    unsigned char *cur_ptr = data_out;
    unsigned char *data_out_tail = data_out + sizeof(data_out);

    PUT_HEADER(cur_ptr,data_out_tail,req_id,req_flag);
    buffer_assign(buffer,(const char *)data_out,cur_ptr - data_out);
    return 0;
}


int iot_buffer_bool(buffer *buffer, uint32_t tag_id , uint8_t bool_flag){
    unsigned char iot_buf[16] = {0};
    int iot_len = pack_iot_bool_packet(0,0,0,tag_id,bool_flag,iot_buf, sizeof(iot_buf));
    CHECK_RET(iot_len,-1);
    CHECK_RET(buffer_append(buffer,(const char *)iot_buf,iot_len),-1);
    return 0;
}

int iot_buffer_double(buffer *buffer, uint32_t tag_id , double double_num){
    unsigned char iot_buf[32] = {0};
    int iot_len = pack_iot_double_packet(0,0,0,tag_id,double_num,iot_buf, sizeof(iot_buf));
    CHECK_RET(iot_len,-1);
    CHECK_RET(buffer_append(buffer,(const char *)iot_buf,iot_len),-1);
    return 0;
}
int iot_buffer_enum(buffer *buffer, uint32_t tag_id, const char *enum_str){
    unsigned char *iot_buf = malloc(16 + strlen(enum_str));
    if(!iot_buf){
        LOGE("malloc failed!");
        return -1;
    }
    int iot_len = pack_iot_enum_packet(0,0,0,tag_id,enum_str,iot_buf, sizeof(iot_buf));
    if(buffer_append(buffer,(const char *)iot_buf,iot_len) == -1){
        free(iot_buf);
        return -1;
    }
    free(iot_buf);
    return 0;
}
int iot_buffer_string(buffer *buffer, uint32_t tag_id, const char *str){
    unsigned char *iot_buf = malloc(16 + strlen(str));
    if(!iot_buf){
        LOGE("malloc failed!");
        return -1;
    }
    int iot_len = pack_iot_string_packet(0,0,0,tag_id,str,iot_buf, sizeof(iot_buf));
    if(buffer_append(buffer,(const char *)iot_buf,iot_len) == -1){
        free(iot_buf);
        return -1;
    }
    free(iot_buf);
    return 0;
}

