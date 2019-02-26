//
// Created by xzl on 2019/2/26.
//

#ifndef MQTT_IOT_PROTO_H
#define MQTT_IOT_PROTO_H


#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum {
    iot_bool = 0x01,//占用一个字节
    iot_enum = 0x02,//本质同于string
    iot_string = 0x03,//可变长度
    oit_double = 0x05,//8个字节
} iot_data_type;


int pack_iot_bool_packet(int req_flag,
                         uint32_t req_id,
                         uint32_t tag_id,
                         uint8_t bool_flag,
                         unsigned char *data_out,
                         int out_len);

int pack_iot_enum_packet(int req_flag,
                         uint32_t req_id,
                         uint32_t tag_id,
                         const char *enum_str,
                         unsigned char *data_out,
                         int out_len);

int pack_iot_string_packet(int req_flag,
                           uint32_t req_id,
                           uint32_t tag_id,
                           const char *str,
                           unsigned char *data_out,
                           int out_len);

int pack_iot_double_packet(int req_flag,
                           uint32_t req_id,
                           uint32_t tag_id,
                           double double_num,
                           unsigned char *data_out,
                           int out_len);

int unpack_iot_packet(int *req_flag,
                      uint32_t *req_id,
                      uint32_t *tag_id,
                      iot_data_type *type,
                      const unsigned char *data_in,
                      int in_len,
                      const unsigned char **content);

void test_iot_packet();


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif //MQTT_IOT_PROTO_H
