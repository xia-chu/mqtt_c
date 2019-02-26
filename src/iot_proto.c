//
// Created by xzl on 2019/2/26.
//

#include "iot_proto.h"
#include "log.h"
#include "mqtt_wrapper.h"
#include <arpa/inet.h>
#include <memory.h>

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

int pack_iot_packet(int req_flag,
                    uint32_t req_id,
                    uint32_t tag_id,
                    iot_data_type type,
                    const unsigned char *data_in,
                    int in_len,
                    unsigned char *data_out,
                    int out_len){
    unsigned char *cur_ptr = data_out;
    unsigned char *data_out_tail = data_out + out_len;

    //控制位，表明是请求还是回复包
    CHECK_LEN(1,data_out_tail);
    cur_ptr[0] = 0x01 & req_flag;
    cur_ptr += 1;

    //request id，4个字节
    CHECK_LEN(4,data_out_tail);
    req_id = htonl(req_id);
    memcpy(cur_ptr,&req_id, sizeof(req_id));
    cur_ptr += 4;

    //tag
    CHECK_LEN(4,data_out_tail);
    cur_ptr += Mqtt_DumpLength(tag_id,(char *)cur_ptr);

    //type
    CHECK_LEN(4,data_out_tail);
    cur_ptr += Mqtt_DumpLength((uint32_t)type,(char *)cur_ptr);

    //消息长度
    CHECK_PTR(data_in,-1);
    if(in_len <= 0){
        in_len = strlen((char *)data_in);
    }
    CHECK_LEN(4,data_out_tail);
    cur_ptr += Mqtt_DumpLength((uint32_t)in_len,(char *)cur_ptr);

    //消息体
    CHECK_LEN(in_len,data_out_tail);
    memcpy(cur_ptr,data_in,in_len);
    cur_ptr += in_len;
    return cur_ptr - data_out;
}


int pack_iot_bool_packet(int req_flag,
                         uint32_t req_id,
                         uint32_t tag_id,
                         uint8_t bool_flag,
                         unsigned char *data_out,
                         int out_len){
    bool_flag &= 0x01;
    return pack_iot_packet(req_flag,req_id,tag_id,iot_bool,&bool_flag, 1,data_out,out_len);
}

int pack_iot_enum_packet(int req_flag,
                         uint32_t req_id,
                         uint32_t tag_id,
                         const char *enum_str,
                         unsigned char *data_out,
                         int out_len){
    return pack_iot_packet(req_flag,req_id,tag_id,iot_enum,(unsigned char *)enum_str, strlen(enum_str),data_out,out_len);
}

int pack_iot_string_packet(int req_flag,
                           uint32_t req_id,
                           uint32_t tag_id,
                           const char *str,
                           unsigned char *data_out,
                           int out_len){
    return pack_iot_packet(req_flag,req_id,tag_id,iot_string,(unsigned char *)str, strlen(str),data_out,out_len);
}

int pack_iot_double_packet(int req_flag,
                           uint32_t req_id,
                           uint32_t tag_id,
                           double double_num,
                           unsigned char *data_out,
                           int out_len){
    double_num = htonll(double_num);
    return pack_iot_packet(req_flag,req_id,tag_id,iot_string,(unsigned char *)&double_num, 8,data_out,out_len);
}


int unpack_iot_packet(int *req_flag,
                      uint32_t *req_id,
                      uint32_t *tag_id,
                      iot_data_type *type,
                      const unsigned char *data_in,
                      int in_len,
                      const unsigned char **content){

    const unsigned char *cur_ptr = data_in;
    const unsigned char *data_in_tail = data_in + in_len;

    //控制位，表明是请求还是回复包
    CHECK_LEN(1,data_in_tail);
    *req_flag = cur_ptr[0] & 0x1F;
    cur_ptr += 1;

    //request id，4个字节
    CHECK_LEN(4,data_in_tail);
    memcpy(req_id,cur_ptr, sizeof(uint32_t));
    *req_id = ntohl(*req_id);
    cur_ptr += 4;

    //tag
    do {
        CHECK_LEN(4,data_in_tail);
        int tag_len = Mqtt_ReadLength((const char *) cur_ptr, 4, tag_id);
        if (tag_len <= 0) {
            LOGW("invalid TAG field:%d %d", tag_len, *tag_id);
            return -1;
        }
        cur_ptr += tag_len;
    }while(0);

    //type
    do {
        CHECK_LEN(4,data_in_tail);
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
    uint32_t content_len = 0;
    do {
        CHECK_LEN(4,data_in_tail);
        int content_len_len = Mqtt_ReadLength((const char *) cur_ptr, 4, &content_len);
        if (content_len_len <= 0) {
            LOGW("invalid content_len field:%d %d", content_len_len, content_len);
            return -1;
        }
        cur_ptr += content_len_len;
    }while (0);
    *content = cur_ptr;
    return content_len;
}

void test_iot_packet(){
    unsigned char out_buffer[29] = {0};
    int out_len = pack_iot_packet(1,1234,4567,iot_string,(const uint8_t *)"this is a iot packet",0,out_buffer, sizeof(out_buffer));
    LOGD("pack_iot_packet ret:%d",out_len);

    int req_flag;
    uint32_t req_id;
    uint32_t tag_id;
    iot_data_type type;
    const unsigned char *content;

    int content_len = unpack_iot_packet(&req_flag,&req_id,&tag_id,&type,out_buffer,out_len,&content);
    LOGD("req_flag:%d , req_id:%d , tag_id:%d , type:%d , content_len:%d , content:%s",req_flag,req_id,tag_id,type,content_len,content);
}