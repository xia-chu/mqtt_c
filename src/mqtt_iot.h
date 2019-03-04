//
// Created by xzl on 2019/3/4.
//

#ifndef MQTT_MQTT_IOT_H
#define MQTT_MQTT_IOT_H
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int iot_connect_pkt(void *iot_ctx,const char *client_id,const char *secret,const char *user_name);

int iot_publish_bool_pkt(void *iot_ctx,int tag,int flag);
int iot_publish_double_pkt(void *iot_ctx,int tag,double double_num);
int iot_publish_enum_pkt(void *iot_ctx,int tag,const char *enum_str);
int iot_publish_string_pkt(void *iot_ctx,int tag,const char *str);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif //MQTT_MQTT_IOT_H
