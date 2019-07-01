//
// Created by xzl on 2019/7/1.
//

#ifndef MQTT_JIMI_MEMORY_H
#define MQTT_JIMI_MEMORY_H
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void *jimi_malloc(int size);
void jimi_free(void *ptr);
void *jimi_realloc(void *ptr,int size);
char *jimi_strdup(const char *str);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
#endif //MQTT_JIMI_MEMORY_H
