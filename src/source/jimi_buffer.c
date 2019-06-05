//
// Created by xzl on 2019/2/22.
//

#include <memory.h>
#include <stdlib.h>
#include "jimi_buffer.h"
#include "jimi_log.h"
#include "mqtt_wrapper.h"

#define RESERVED_SIZE 32


buffer *buffer_alloc(){
    buffer *ret = (buffer *) malloc(sizeof(buffer));
    CHECK_PTR(ret,NULL);
    buffer_init(ret);
    return ret;
}

int buffer_free(buffer *buf){
    CHECK_PTR(buf,-1);
    buffer_release(buf);
    free(buf);
    return 0;
}

int buffer_init(buffer *buf){
    CHECK_PTR(buf,-1);
    memset(buf,0, sizeof(buffer));
    return 0;
}
int buffer_release(buffer *buf){
    CHECK_PTR(buf,-1);
    if(buf->_capacity && buf->_data){
        free(buf->_data);
        //LOGD("free:%d",buf->_capacity);
    }
    buffer_init(buf);
    return 0;
}

int buffer_append_buffer(buffer *buf,buffer *from){
    return buffer_append(buf,from->_data,from->_len);
}
int buffer_append(buffer *buf,const char *data,int len){
    CHECK_PTR(buf,-1);
    CHECK_PTR(data,-1);
    if(len <= 0){
        len = strlen(data);
    }

    if(!buf->_capacity){
        //内存尚未开辟
        buf->_data = malloc(len + RESERVED_SIZE);
        //LOGD("malloc:%d",len + RESERVED_SIZE);
        if(!buf->_data){
            LOGE("out of memory:%d",len);
            return -1;
        }
        memcpy(buf->_data,data,len);
        buf->_len = len;
        buf->_data[buf->_len] = '\0';
        buf->_capacity = len + RESERVED_SIZE;
        return 0;
    }

    if(buf->_capacity > buf->_len + len){
        //容量够
        memcpy(buf->_data + buf->_len ,data,len);
        buf->_len += len;
        buf->_data[buf->_len] = '\0';
        return 0;
    }

    //已经开辟的容量不够
    buf->_data = realloc(buf->_data,len + buf->_len + RESERVED_SIZE);
    //LOGD("realloc:%d",len + buf->_len + RESERVED_SIZE);

    if(!buf->_data){
        //out of memory
        LOGE("out of memory:%d",len + buf->_len);
        buf->_len = 0;
        return -1;
    }
    memcpy(buf->_data + buf->_len ,data,len);
    buf->_len += len;
    buf->_data[buf->_len] = '\0';
    buf->_capacity = buf->_len + RESERVED_SIZE;
    return 0;
}
int buffer_assign(buffer *buf,const char *data,int len){
    CHECK_PTR(buf,-1);
    CHECK_PTR(data,-1);
    if(len <= 0){
        len = strlen(data);
    }
    if(buf->_capacity > len){
        memcpy(buf->_data,data,len);
        buf->_len = len;
        buf->_data[buf->_len] = '\0';
        return 0;
    }
    buffer_release(buf);
    return buffer_append(buf,data,len);
}

int buffer_move(buffer *dst,buffer *src){
    buffer_release(dst);
    dst->_data = src->_data;
    dst->_len = src->_len;
    dst->_capacity = src->_capacity;
    memset(src,0, sizeof(buffer));
    return 0;
}