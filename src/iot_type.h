//
// Created by xzl on 2019/3/5.
//

#ifndef MQTT_IOT_TYPE_H
#define MQTT_IOT_TYPE_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum {
    iot_bool = 0x01,//占用一个字节
    iot_enum = 0x02,//本质同于string
    iot_string = 0x03,//可变长度
    iot_double = 0x05,//8个字节
} iot_data_type;

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
#endif //MQTT_IOT_TYPE_H
