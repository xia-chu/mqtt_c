//
// Created by xzl on 2019/2/22.
//

#ifndef JIMI_LOG_H
#define JIMI_LOG_H

#include <stdio.h>
#define CLEAR_COLOR "\033[0m"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * 日志等级
 */
typedef enum {
    log_trace = 0,
    log_debug ,
    log_info ,
    log_warn ,
    log_error ,
} e_log_lev;

extern const char *LOG_CONST_TABLE[][3];


/**
 * 获取当前时间字符串
 * @param buf 存放地址
 * @param buf_size 存放地址长度
 */
void get_now_time_str(char *buf,int buf_size);

/**
 * 设置日志等级
 * @param lev 日志等级
 */
void set_log_level(e_log_lev lev);

/**
 * 获取日志等级
 * @return 日志等级
 */
e_log_lev get_log_level();


#define _PRINT_(encble_color,print,lev,file,line,func,fmt,...) \
do{ \
    if(lev < get_log_level()){ \
        break; \
    } \
    print("%s %d\r\n",file,line);\
    char time_str[26];\
    get_now_time_str(time_str,sizeof(time_str)); \
    if(!encble_color){\
        print("%s %s | %s " fmt "\r\n",\
                time_str ,\
                LOG_CONST_TABLE[lev][2],\
                func,\
                ##__VA_ARGS__);\
    }else{\
        print("%s %s %s | %s " fmt CLEAR_COLOR "\r\n",\
                LOG_CONST_TABLE[lev][1],\
                time_str ,\
                LOG_CONST_TABLE[lev][2],\
                func,\
                ##__VA_ARGS__);\
    }\
} while(0)

#ifdef __alios__
#include <aos/aos.h>
    extern int csp_printf(const char *fmt, ...);
    #define PRINT(lev,file,line,func,fmt,...)  _PRINT_(0,csp_printf,lev,file,line,func,fmt,##__VA_ARGS__)
#else
#define PRINT(lev,file,line,func,fmt,...) _PRINT_(1,printf,lev,file,line,func,fmt,##__VA_ARGS__)
#endif


//以下宏都是写日志宏
#define LOGT(...) PRINT(log_trace,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define LOGD(...) PRINT(log_debug,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define LOGI(...) PRINT(log_info,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define LOGW(...) PRINT(log_warn,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)
#define LOGE(...) PRINT(log_error,__FILE__,__LINE__,__FUNCTION__,##__VA_ARGS__)


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
#endif //JIMI_LOG_H
