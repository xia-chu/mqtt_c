//
// Created by xzl on 2019/2/22.
//

#ifndef MQTT_BUFFER_H
#define MQTT_BUFFER_H
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct {
    char *_data;
    int _len;
} buffer;

int buffer_init(buffer *buf);
void buffer_release(buffer *buf);

int buffer_append(buffer *buf,const char *data,int len);
int buffer_assign(buffer *buf,const char *data,int len);
int buffer_move(buffer *dst,buffer *src);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
#endif //MQTT_BUFFER_H
