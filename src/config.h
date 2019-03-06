#ifndef ONENET_CONFIG_H
#define ONENET_CONFIG_H

#include <stddef.h>

#if  defined(WIN32) || defined(__alios__)
struct iovec {
    void *iov_base;
    size_t iov_len;
};
#else
#include <sys/uio.h>
#endif // _WIN32


#endif // ONENET_CONFIG_H
