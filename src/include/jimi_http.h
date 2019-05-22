//
// Created by xzl on 2019-05-21.
//

#ifndef MQTT_JIMI_HTTP_H
#define MQTT_JIMI_HTTP_H

#include "jimi_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

///////////////////////////////////////HTTP请求包生成对象//////////////////////////////////////////

typedef struct http_request http_request;

/**
 * 创建http_request
 * @return 对象指针
 */
http_request *http_request_alloc();

/**
 * 释放http_request对象
 * @param ctx 对象指针
 * @return 0成功，-1失败
 */
int http_request_free(http_request *ctx);

/**
 * 设置HTTP方法名，譬如GET，POST等
 * @param ctx 对象指针
 * @return 0成功，-1失败
 */
int http_request_set_method(http_request *ctx,const char *method);

/**
 * 设置url，譬如 /index.html
 * @param ctx 对象指针
 * @return 0成功，-1失败
 */
int http_request_set_path(http_request *ctx,const char *url);

/**
 * 设置body
 * @param ctx 对象指针
 * @param content_type 譬如 application/json
 * @param content_data body内容
 * @param content_len body长度，如果<=0 ,那么通过strlen(content_data)获取
 * @return 0成功，-1失败
 */
int http_request_set_body(http_request *ctx,const char *content_type,const char *content_data,int content_len);

/**
 * 设置http头，可以覆盖http头，不区分大小写
 * @param ctx 对象指针
 * @param key http头字段名，不区分大小写
 * @param value http头字段内容
 * @return 0成功，-1失败
 */
int http_request_add_header(http_request *ctx,const char *key,const char *value);

/**
 * 批量添加http头，譬如 http_request_add_header_array(ctx,"Host","127.0.0.1","Connection","close",NULL)
 * 以NULL结尾标记参数结束
 * @param ctx 对象指针
 * @param ... 参数列表
 * @return
 */
int http_request_add_header_array(http_request *ctx,...);

/**
 * 把http_request输出到buffer对象(连续的内存指针)
 * @param ctx 对象指针
 * @param out buffer对象指针
 * @return 0成功，-1失败
 */
int http_request_dump_to_buffer(http_request *ctx,buffer *out);

/**
 * 功能测试
 */
void test_http_request();


//////////////////////////////////////HTTP回复split以及解析对象///////////////////////////////////////////
/**
 * http_response对象定义
 */
typedef struct http_response http_response;

/**
 * 解析出http回复包回调
 * @param user_data 用户数据指针
 * @param http_response 对象本身指针
 * @param content_slice body分片数据
 * @param content_slice_len body分片数据大小
 * @param content_received_len 已接收到body数据长度
 * @param content_total_len body数据总长度,如果content_received_len + content_slice_len == content_total_len说明本次http回复接收完毕
 */
typedef void (*on_split_response)(void *user_data,
                                  http_response *ctx,
                                  const char *content_slice,
                                  int content_slice_len,
                                  int content_received_len,
                                  int content_total_len);


/**
 * 创建http_response解析对象
 * @param cb split回调函数指针
 * @param user_data 回调用户数据指针
 * @return 对象指针
 */
http_response *http_response_alloc(on_split_response cb,void *user_data);

/**
 * 释放http_response解析对象
 * @param ctx 对象指针
 * @return 0成功，-1失败
 */
int http_response_free(http_response *ctx);

/**
 * 输入数据到对象中解析http回复，该对象支持处理粘包和包分片
 * @param ctx 对象指针
 * @param data 数据指针
 * @param len 数据长度
 * @return -1失败，否则返回输入的数据长度
 */
int http_response_input(http_response *ctx,const char *data,int len);

/**
 * 查找http头值，无拷贝的(请勿free)
 * @param ctx 对象本身指针
 * @param key 键名
 * @return 键值，未找到则返回NULL
 */
const char *http_response_get_header(http_response *ctx,const char *key);

/**
 * 获取http头总个数，用于遍历用
 * @see http_response_get_header_pair
 * @param ctx 对象本身指针
 * @return http头总个数
 */
int http_response_get_header_count(http_response *ctx);

/**
 * 根据索引获取http头键值对
 * @see http_response_get_header_count
 * @param ctx 对象本身指针
 * @param index 索引
 * @param key 键指针
 * @param value 值指针
 * @return 0成功，-1失败
 */
int http_response_get_header_pair(http_response *ctx,int index ,const char **key,const char **value);


/**
 * 获取body大小
 * @param ctx 对象本身指针
 * @return body大小
 */
int http_response_get_bodylen(http_response *ctx);

/**
 * 获取http回复状态码
 * @param ctx 对象本身指针
 * @return 状态码，例如200，404
 */
int http_response_get_status_code(http_response *ctx);

/**
 * 获取http回复状态码对应的状态字符串
 * @param ctx 对象本身指针
 * @return 状态字符串，例如OK ，Not Found
 */
const char *http_response_get_status_str(http_response *ctx);

/**
 * 获取http回复HTTP版本号
 * @param ctx 对象本身指针
 * @return 例如HTTP/1.1
 */
const char *http_response_get_http_version(http_response *ctx);

/**
 * 测试http回复包解析
 */
void test_http_response();


//////////////////////////////////////HTTP url解析工具///////////////////////////////////////////
typedef struct http_url http_url;

/**
 * 解析http url
 * @param url url字符串
 * @return http_url对象，失败返回null
 */
http_url *http_url_parse(const char *url) ;

/**
 * 释放http_url对象
 * @param ctx http_url对象
 * @return 0成功,-1失败
 */
int http_url_free(http_url *ctx);

/**
 * 获取端口号
 * @param ctx http_url对象
 * @return 端口号，0代表失败
 */
unsigned short http_url_get_port(http_url *ctx);

/**
 * 判断是否为https协议
 * @param ctx http_url对象
 * @return 是否为https协议，1代表是，0代表不是
 */
int http_url_is_https(http_url *ctx);

/**
 * 获取主机地址
 * @param ctx http_url对象
 * @return 主机地址
 */
const char *http_url_get_host(http_url *ctx);

/**
 * 获取http访问路径,例如 /index.html
 * @param ctx http_url对象
 * @return 访问路径
 */
const char *http_url_get_path(http_url *ctx);

/**
 * 功能测试
 */
void test_http_url();
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
#endif //MQTT_JIMI_HTTP_H
