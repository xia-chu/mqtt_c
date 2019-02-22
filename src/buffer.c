//
// Created by xzl on 2019/2/22.
//

#include <memory.h>
#include <stdlib.h>
#include "buffer.h"

int buffer_init(buffer *buf){
    memset(buf,0, sizeof(buffer));
    return 0;
}
void buffer_release(buffer *buf){
    if(buf->_len && buf->_data){
        free(buf->_data);
    }
    buffer_init(buf);
}

int buffer_append(buffer *buf,const char *data,int len){
    if(buf->_len && buf->_data){
        char *old = buf->_data;
        buf->_data = malloc(len + buf->_len);
        if(!buf->_data){
            buf->_data = old;
            return -1;
        }
        memcpy(buf->_data,old,buf->_len);
        memcpy(buf->_data + buf->_len ,data,len);
        buf->_len += len;
        free(old);
        return 0;
    }

    buf->_data = malloc(len);
    if(!buf->_data){
        return -1;
    }
    memcpy(buf->_data,data,len);
    buf->_len = len;
    return 0;
}
int buffer_assign(buffer *buf,const char *data,int len){
    buffer_release(buf);
    return buffer_append(buf,data,len);
}

int buffer_move(buffer *dst,buffer *src){
    buffer_release(dst);
    dst->_data = src->_data;
    dst->_len = src->_len;
    memset(src,0, sizeof(buffer));
    return 0;
}