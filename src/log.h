//
// Created by xzl on 2019/2/22.
//

#ifndef MQTT_LOG_H
#define MQTT_LOG_H

#include <stdio.h>

typedef enum {
    log_trace = 0,
    log_debug ,
    log_info ,
    log_warn ,
    log_error ,
} e_log_lev;

extern const char *g_log_lev_str[];

#define LOG(lev,...) \
do{ \
    printf("%s %d\r\n",__FILE__,__LINE__);\
    printf("%s | %s ",g_log_lev_str[lev],__FUNCTION__);\
    printf(__VA_ARGS__);\
} while(0)

#define LOGT(...) LOG(log_trace,__VA_ARGS__)
#define LOGD(...) LOG(log_debug,__VA_ARGS__)
#define LOGI(...) LOG(log_info,__VA_ARGS__)
#define LOGW(...) LOG(log_warn,__VA_ARGS__)
#define LOGE(...) LOG(log_error,__VA_ARGS__)


#endif //MQTT_LOG_H
