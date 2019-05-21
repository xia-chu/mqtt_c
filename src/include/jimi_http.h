//
// Created by xzl on 2019-05-21.
//

#ifndef MQTT_JIMI_HTTP_H
#define MQTT_JIMI_HTTP_H

#include "jimi_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct http_context http_context;

/**
 * 创建http_context
 * @return 对象指针
 */
http_context *http_context_alloc();

/**
 * 释放http_context对象
 * @param ctx 对象指针
 * @return 0成功，-1失败
 */
int http_context_free(http_context *ctx);

/**
 * 设置HTTP方法名，譬如GET，POST等
 * @param ctx 对象指针
 * @return 0成功，-1失败
 */
int http_context_set_method(http_context *ctx,const char *method);

/**
 * 设置url，譬如 /index.html
 * @param ctx 对象指针
 * @return 0成功，-1失败
 */
int http_context_set_url(http_context *ctx,const char *url);

/**
 * 设置body
 * @param ctx 对象指针
 * @param content_type 譬如 application/json
 * @param content_data body内容
 * @param content_len body长度，如果<=0 ,那么通过strlen(content_data)获取
 * @return 0成功，-1失败
 */
int http_context_set_body(http_context *ctx,const char *content_type,const char *content_data,int content_len);

/**
 * 设置http头，可以覆盖http头，不区分大小写
 * @param ctx 对象指针
 * @param key http头字段名，不区分大小写
 * @param value http头字段内容
 * @return 0成功，-1失败
 */
int http_context_add_header(http_context *ctx,const char *key,const char *value);

/**
 * 把http_context输出到buffer对象(连续的内存指针)
 * @param ctx 对象指针
 * @param out buffer对象指针
 * @return 0成功，-1失败
 */
int http_context_dump_to_buffer(http_context *ctx,buffer *out);

/**
 * 功能测试
 */
void test_http_context();

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
#endif //MQTT_JIMI_HTTP_H
