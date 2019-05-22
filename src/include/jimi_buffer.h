//
// Created by xzl on 2019/2/22.
//

#ifndef JIMI_BUFFER_H
#define JIMI_BUFFER_H
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct {
    char *_data;
    int _len;
    int _capacity;
} buffer;

/**
 * 初始化一个buffer对象
 * @param buf buffer对象指针
 * @return 0代表成功，否则为-1
 */
int buffer_init(buffer *buf);

/**
 * 释放buffer对象
 * @param buf buffer对象指针
 */
int buffer_release(buffer *buf);

/**
 * 追加数据至buffer对象末尾
 * @param buf 对象指针
 * @param data 数据指针
 * @param len 数据长度，可以为0，为0时内部使用strlen获得长度
 * @return 0为成功
 */
int buffer_append(buffer *buf,const char *data,int len);


/**
 * 追加数据至buffer对象末尾
 * @param buf 对象指针
 * @param from 子buffer
 * @return 0为成功
 */
int buffer_append_buffer(buffer *buf,buffer *from);

/**
 * 复制数据至buffer对象
 * @param buf 对象指针
 * @param data 数据指针
 * @param len 数据长度，可以为0，为0时内部使用strlen获得长度
 * @return 0为成功
 */
int buffer_assign(buffer *buf,const char *data,int len);

/**
 * 把src对象里面的数据移动至dst，内部有无memcpy
 * @param dst 目标对象
 * @param src 源对象
 * @return 0为成功
 */
int buffer_move(buffer *dst,buffer *src);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
#endif //JIMI_BUFFER_H
