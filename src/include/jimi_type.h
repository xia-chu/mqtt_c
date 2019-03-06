#ifndef JIMI_TYPE_H
#define JIMI_TYPE_H

#include <stddef.h>

#if  defined(WIN32) || defined(__alios__)
struct iovec {
    void *iov_base;
    size_t iov_len;
};
#else
#include <sys/uio.h>
#endif // _WIN32

typedef enum {
    iot_bool = 0x01,//布尔型，占用一个字节
    iot_enum = 0x02,//本质同于string
    iot_string = 0x03,//字符串类型，可变长度
    iot_double = 0x05,//双精度浮点型，8个字节
} iot_data_type;

#endif // JIMI_TYPE_H
