//
// Created by xzl on 2019/2/22.
//

#ifndef MQTT_LOG_H
#define MQTT_LOG_H

#include <stdio.h>

#define CLEAR_COLOR "\033[0m"

extern const char *LOG_CONST_TABLE[][3];
extern void get_now_time_str(char *buf,int buf_size);

typedef enum {
    log_trace = 0,
    log_debug ,
    log_info ,
    log_warn ,
    log_error ,
} e_log_lev;

#ifdef __alios__

#include <aos/aos.h>

#define PRINT(lev,fmt,...) \
do{ \
    LOG("%s %d\r\n",__FILE__,__LINE__);\
    LOG("%s | %s " fmt,LOG_CONST_TABLE[lev][1],__FUNCTION__,##__VA_ARGS__);\
} while(0)

#else

#define PRINT(lev,fmt,...) \
do{ \
    printf("%s %d\r\n",__FILE__,__LINE__);\
    char time_str[32];\
    get_now_time_str(time_str,sizeof(time_str)); \
    printf("%s %s %s | %s " fmt CLEAR_COLOR "\r\n",\
            LOG_CONST_TABLE[lev][1],\
            time_str ,\
            LOG_CONST_TABLE[lev][2],\
            __FUNCTION__,\
            ##__VA_ARGS__);\
} while(0)

#endif

#define LOGT(...) PRINT(log_trace,__VA_ARGS__)
#define LOGD(...) PRINT(log_debug,__VA_ARGS__)
#define LOGI(...) PRINT(log_info,__VA_ARGS__)
#define LOGW(...) PRINT(log_warn,__VA_ARGS__)
#define LOGE(...) PRINT(log_error,__VA_ARGS__)


#endif //MQTT_LOG_H
