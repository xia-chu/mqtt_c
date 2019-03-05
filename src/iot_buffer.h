//
// Created by xzl on 2019/3/5.
//

#ifndef MQTT_IOT_BUFFER_H
#define MQTT_IOT_BUFFER_H
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "buffer.h"

int iot_buffer_set_header(buffer *buffer, int req_flag, uint32_t req_id);
int iot_buffer_append_bool(buffer *buffer, uint32_t tag_id , uint8_t bool_flag);
int iot_buffer_append_enum(buffer *buffer, uint32_t tag_id, const char *enum_str);
int iot_buffer_append_string(buffer *buffer, uint32_t tag_id, const char *str);
int iot_buffer_append_double(buffer *buffer, uint32_t tag_id , double double_num);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif //MQTT_IOT_BUFFER_H
