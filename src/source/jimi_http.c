//
// Created by xzl on 2019-05-21.
//
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "jimi_http.h"
#include "jimi_log.h"
#include "jimi_buffer.h"
#include "jimi_memory.h"
#include "avl-tree.h"

typedef struct http_request
{
    buffer _method;
    buffer _path;
    buffer _body;
    AVLTree *_header;
} http_request;

static int AVLTreeCompare(AVLTreeKey key1, AVLTreeKey key2){
    return strcasecmp(key1,key2);
}

http_request *http_request_alloc(){
    http_request *ctx = (http_request*)jimi_malloc(sizeof(http_request));
    CHECK_PTR(ctx,NULL);
    memset(ctx,0, sizeof(http_request));
    ctx->_header = avl_tree_new(AVLTreeCompare);
    return ctx;
}

int http_request_free(http_request *ctx){
    CHECK_PTR(ctx,-1);
    buffer_release(&ctx->_method);
    buffer_release(&ctx->_path);
    buffer_release(&ctx->_body);
    avl_tree_free(ctx->_header);
    jimi_free(ctx);
    return 0;
}

int http_request_set_method(http_request *ctx,const char *method){
    CHECK_PTR(ctx,-1);
    CHECK_PTR(method,-1);
    buffer_assign(&ctx->_method,method,0);
    return 0;
}

int http_request_set_path(http_request *ctx,const char *url){
    CHECK_PTR(ctx,-1);
    CHECK_PTR(url,-1);
    buffer_assign(&ctx->_path,url,0);
    return 0;
}

int http_request_set_body(http_request *ctx,const char *content_type,const char *content_data,int content_len){
    CHECK_PTR(ctx,-1);
    CHECK_PTR(content_type,-1);
    CHECK_PTR(content_data,-1);
    if(content_len <= 0){
        content_len = strlen(content_data);
    }
    buffer_assign(&ctx->_body,content_data,content_len);
    avl_tree_insert(ctx->_header,"Content-Type",jimi_strdup(content_type),NULL,jimi_free);
    char *len_str = jimi_malloc(8);
    CHECK_PTR(len_str,-1);
    sprintf(len_str,"%d",content_len);
    avl_tree_insert(ctx->_header,"Content-Length",len_str,NULL,jimi_free);
    return 0;
}

int http_request_add_header(http_request *ctx,const char *key,const char *value){
    CHECK_PTR(ctx,-1);
    CHECK_PTR(key,-1);
    CHECK_PTR(value,-1);
    avl_tree_insert(ctx->_header,jimi_strdup(key),jimi_strdup(value),jimi_free,jimi_free);
    return 0;
}

int http_request_add_header_array(http_request *ctx,...){
    CHECK_PTR(ctx,-1);
    va_list list;
    va_start(list,ctx);
    const char *key ,*value;
    do{
        key = va_arg(list, const char *);
        if(!key || key[0] == '\0') {
            break;
        }
        value = va_arg(list, const char *);
        if(!value || value[0] == '\0'){
            break;
        }
        http_request_add_header(ctx,key,value);
    }while (1);
    va_end(list);
    return 0;
}

int http_request_dump_to_buffer(http_request *ctx,buffer *out){
    CHECK_PTR(ctx,-1);
    CHECK_PTR(out,-1);
    buffer_init(out);

    buffer_move(out,&ctx->_method);
    buffer_append(out," ",1);
    buffer_append_buffer(out,&ctx->_path);
    buffer_append(out," HTTP/1.1\r\n",0);

    AVLTreeNode **nodes = avl_tree_to_array_node(ctx->_header);
    unsigned int i;
    for (i = 0 ; i < avl_tree_num_entries(ctx->_header) ; ++i){
        AVLTreeKey key = avl_tree_node_key(nodes[i]);
        AVLTreeValue value = avl_tree_node_value(nodes[i]);
        buffer_append(out,key,0);
        buffer_append(out,": ",0);
        buffer_append(out,value,0);
        buffer_append(out,"\r\n",0);
    }
    if(nodes){
        jimi_free(nodes);
    }
    buffer_append(out,"\r\n",0);
    if(ctx->_body._len){
        buffer_append_buffer(out,&ctx->_body);
    }
    return 0;
}


void test_http_request(){
    http_request *ctx = http_request_alloc();
    http_request_set_method(ctx,"POST");
    http_request_set_path(ctx,"/index.html");
    http_request_set_body(ctx,"text/plain","this is a test body",0);
    //测试大小写键覆盖
    http_request_add_header(ctx,"host","baidu.com");
    http_request_add_header(ctx,"Host","baidu.com");
    http_request_add_header(ctx,"Connection","close");
    http_request_add_header(ctx,"Tools","mqtt_c");
    http_request_add_header(ctx,"Accept","*/*");
    http_request_add_header(ctx,"Accept-Language","zh-CN,zh;q=0.8");
    http_request_add_header(ctx,"User-Agent","Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/57.0.2987.133 Safari/537.36");

    http_request_add_header_array(ctx,"key1","value1","key2","value2",NULL);
    buffer out;
    buffer_init(&out);
    http_request_dump_to_buffer(ctx,&out);
    LOGD("\r\n%s",out._data);
    buffer_release(&out);
    http_request_free(ctx);
}

typedef struct http_response{
    buffer _data;
    AVLTree *_header;
    on_split_response _split_cb;
    void *_user_data;

    int _header_len;
    int _body_len;
    int _content_received;
    char _http_version[16];
    char _status_str[16];
    int _status_code;
} http_response;


http_response *http_response_alloc(on_split_response cb,void *user_data){
    http_response *ctx = (http_response *)jimi_malloc(sizeof(http_response));
    CHECK_PTR(ctx,NULL);
    memset(ctx,0, sizeof(http_response));
    ctx->_header = avl_tree_new(AVLTreeCompare);
    ctx->_split_cb = cb;
    ctx->_user_data = user_data;
    return ctx;
}


int http_response_free(http_response *ctx){
    CHECK_PTR(ctx,-1);
    buffer_release(&ctx->_data);
    avl_tree_free(ctx->_header);
    jimi_free(ctx);
    return 0;
}

void print_tree(AVLTree *tree){
    int i;
    printf("####### header #########\r\n");
    for(i = 0 ; i < avl_tree_num_entries(tree) ; ++i){
        AVLTreeNode *node = avl_tree_get_node_by_index(tree,i);
        printf("%s = %s\r\n",avl_tree_node_key(node),avl_tree_node_value(node));
    }
    printf("\r\n");
}

#define OFFSET(type,item) (int)(&(((type *)(NULL))->item))
#define CLEAR_OFFSET(ctx,type,item)   memset((void*)ctx + OFFSET(type,item),0, sizeof(type) - OFFSET(type,item));



int http_response_input(http_response *ctx,const char *data,int len){
    CHECK_PTR(ctx,-1);
    CHECK_PTR(data,-1);
    if (len <= 0){
        len = strlen(data);
    }

    while(1) {
        if (ctx->_header_len) {
            //接收所有http头完毕,现在接收body部分
            int slice_len = len;
            if (ctx->_content_received + slice_len > ctx->_body_len) {
                slice_len = ctx->_body_len - ctx->_content_received;
            }

            if(ctx->_split_cb){
                ctx->_split_cb(ctx->_user_data, ctx, data, slice_len, ctx->_content_received, ctx->_body_len);
            }

            ctx->_content_received += slice_len;
            data += slice_len;
            len -= slice_len;
            if(ctx->_content_received != ctx->_body_len){
                //body尚未接收完毕
                return 0;
            }
        }

        if(ctx->_content_received == ctx->_body_len && ctx->_header_len){
            //上个包处理完毕，重置
            if(len) {
                //有剩余数据
                //新建一个临时buffer，用于保存ctx->_data，目的是为了保存data指针有效
                buffer buffer_tmp;
                buffer_init(&buffer_tmp);
                buffer_move(&buffer_tmp, &ctx->_data);
                //把data指针拷贝至ctx->_data，用于处理下一个包
                buffer_assign(&ctx->_data, data, len);
                //data指针已经使用完毕，可以释放了
                buffer_release(&buffer_tmp);
                len = 0;
            }else{
                buffer_release(&ctx->_data);
            }
            //重置对象
            avl_tree_free(ctx->_header);
            ctx->_header = avl_tree_new(AVLTreeCompare);
            CLEAR_OFFSET(ctx, http_response, _header_len);
            if(!ctx->_data._len){
                return 0;
            }
        }

        if(len){
            buffer_append(&ctx->_data, data, len);
        }

        //处理http回复头
        char *pos = strstr(ctx->_data._data, "\r\n\r\n");
        if (!pos) {
            //还未接收完毕http头
            return 0;
        }

        ctx->_header_len = pos + 4 - ctx->_data._data;
        char *line_start, *line_end;
        for (line_end = ctx->_data._data; line_end < ctx->_data._data + ctx->_header_len - 2;) {
            line_start = line_end;
            line_end = strstr(line_start, "\r\n");
            if (line_start == ctx->_data._data) {
                sscanf(line_start, "%15s %d %15[^\r]", ctx->_http_version, &ctx->_status_code, ctx->_status_str);
                continue;
            }

            *line_end = '\0';
            line_end += 2;
            pos = strstr(line_start, ": ");
            if (!pos) {
                continue;
            }
            *pos = '\0';
            avl_tree_insert(ctx->_header, jimi_strdup(line_start), jimi_strdup(pos + 2), jimi_free, jimi_free);
        }

        const char *content_len = http_response_get_header(ctx, "Content-Length");
        if (content_len) {
            ctx->_body_len = atoi(content_len);
        } else {
            ctx->_body_len = 0;
        }

        len = ctx->_data._len - ctx->_header_len;
        data = ctx->_data._data + ctx->_header_len;
    }
}

const char *http_response_get_header(http_response *ctx,const char *key){
    CHECK_PTR(ctx,NULL);
    return (const char *)avl_tree_lookup(ctx->_header,(AVLTreeKey)key);
}

int http_response_get_header_count(http_response *ctx){
    CHECK_PTR(ctx,-1);
    return avl_tree_num_entries(ctx->_header);
}

int http_response_get_header_pair(http_response *ctx,int index ,const char **key,const char **value){
    CHECK_PTR(ctx,-1);
    AVLTreeNode *node = avl_tree_get_node_by_index(ctx->_header,index);
    if(!node){
        return -1;
    }
    *key = avl_tree_node_key(node);
    *value = avl_tree_node_value(node);
    return 0;
}

int http_response_get_bodylen(http_response *ctx){
    CHECK_PTR(ctx,-1);
    return ctx->_body_len;
}

int http_response_get_status_code(http_response *ctx){
    CHECK_PTR(ctx,-1);
    return ctx->_status_code;
}

const char *http_response_get_status_str(http_response *ctx){
    CHECK_PTR(ctx,NULL);
    return ctx->_status_str;
}

const char *http_response_get_http_version(http_response *ctx){
    CHECK_PTR(ctx,NULL);
    return ctx->_http_version;
}

void on_split_http_response(void *user_data,
                            http_response *ctx,
                            const char *content_slice,
                            int content_slice_len,
                            int content_received_len,
                            int content_total_len){
    if(content_received_len == 0){
        //开始接收到http头
        print_tree(ctx->_header);
        printf("######## body ##########\r\n");
    }
    if(content_slice_len){
        //这个是body
        buffer buffer;
        buffer_init(&buffer);
        buffer_assign(&buffer,content_slice,content_slice_len);
        printf("%s",buffer._data);
        buffer_release(&buffer);
    }
    if(content_total_len == content_slice_len + content_received_len){
        printf("\r\n\r\n");
    }
}

void test_http_response() {
    static const char http_str[] =
            "HTTP/1.1 206 Partial Content\r\n"
            "Connection: keep-alive\r\n"
            "Content-Length: 19\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Date: Tue, May 21 2019 07:16:17 GMT\r\n"
            "Keep-Alive: timeout=10, max=100\r\n"
            "Server: ZLMediaKit-4.0\r\n\r\n"
            "this is a test body"
            "HTTP/1.1 200 OK\r\n"
            "Connection: keep-alive\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Date: Tue, May 21 2019 07:16:17 GMT\r\n"
            "Keep-Alive: timeout=10, max=100\r\n"
            "Server: ZLMediaKit-4.0\r\n\r\n";

    http_response *res = http_response_alloc(on_split_http_response, NULL);
    http_response_input(res, http_str, sizeof(http_str) - 1);
    http_response_input(res, http_str, sizeof(http_str) - 1);
    http_response_input(res, http_str, sizeof(http_str) - 1);

    int totalTestCount = 10;
    while (--totalTestCount) {
        int totalSize = sizeof(http_str) - 1;
        int slice = totalSize / totalTestCount;
        const char *ptr = http_str;
        while (ptr + slice < http_str + totalSize) {
            buffer slice_buf;
            buffer_init(&slice_buf);
            buffer_assign(&slice_buf, ptr, slice);
            http_response_input(res, slice_buf._data, slice_buf._len);
            buffer_release(&slice_buf);
            ptr += slice;
        }
        http_response_input(res, ptr, http_str + totalSize - ptr + 1);
    }
    http_response_free(res);
}

//////////////////////////////////////HTTP url解析工具///////////////////////////////////////////
typedef struct http_url{
    char *_path;
    char *_host;
    unsigned short _port;
    int _is_https;
}http_url;

http_url *http_url_parse(const char *url) {
    CHECK_PTR(url, NULL);
    char *http = jimi_malloc(16), *host = jimi_malloc(128);
    char *path = jimi_malloc(strlen(url));
    unsigned short port = 0;
    int is_https = 0;

    if (4 == sscanf(url, "%15[^://]://%127[^:]:%hd%s", http, host, &port, path)) {
    } else if(3 == sscanf(url, "%15[^://]://%127[^/]%s", http, host, path)) {
    } else if(3 == sscanf(url,"%15[^://]://%127[^:]:%hd",http,host,&port)){
        jimi_free(path);
        path = NULL;
    } else if(2 == sscanf(url,"%15[^://]://%127[^/]",http,host)){
        jimi_free(path);
        path = NULL;
    } else{
        jimi_free(path);
        jimi_free(http);
        jimi_free(host);
        return NULL;
    }

    if(strcasecmp(http,"https") == 0){
        is_https = 1;
        if(port == 0){
            port = 443;
        }
    }else if(strcasecmp(http,"http") == 0){
        is_https = 0;
        if(port == 0){
            port = 80;
        }
    }else{
        LOGW("%s is not a http url!",url);
        jimi_free(path);
        jimi_free(http);
        jimi_free(host);
        return NULL;
    }
    jimi_free(http);

    http_url *ctx = (http_url *) jimi_malloc(sizeof(http_url));
    CHECK_PTR(ctx,NULL);
    ctx->_port = port;
    ctx->_is_https = is_https;
    ctx->_path = path ? path : jimi_strdup("/");
    ctx->_host = host;
    return ctx;
}

int http_url_free(http_url *ctx){
    CHECK_PTR(ctx,-1);
    jimi_free(ctx->_path);
    jimi_free(ctx->_host);
    jimi_free(ctx);
    return 0;
}

unsigned short http_url_get_port(http_url *ctx){
    CHECK_PTR(ctx,0);
    return ctx->_port;
}

int http_url_is_https(http_url *ctx){
    CHECK_PTR(ctx,-1);
    return ctx->_is_https;
}

const char *http_url_get_host(http_url *ctx){
    CHECK_PTR(ctx,NULL);
    return ctx->_host;
}

const char *http_url_get_path(http_url *ctx){
    CHECK_PTR(ctx,NULL);
    return ctx->_path;
}

void http_url_parse_and_print(const char *url){
    http_url *ctx = http_url_parse(url);
    if(ctx){
        LOGD("%s://%s:%d%s",ctx->_is_https ? "https" : "http",ctx->_host,ctx->_port,ctx->_path);
        http_url_free(ctx);
    }
}
void test_http_url(){
    http_url_parse_and_print("HTTP://127.0.0.1:80/");
    http_url_parse_and_print("http://127.0.0.1:80");
    http_url_parse_and_print("http://127.0.0.1/");
    http_url_parse_and_print("http://127.0.0.1");
    http_url_parse_and_print("https://127.0.0.1/");
    http_url_parse_and_print("https://127.0.0.1");
    http_url_parse_and_print("http://127.0.0.1/live/0");
    http_url_parse_and_print("httpsss://127.0.0.1:80/live/0");
    http_url_parse_and_print("https://127.0.0.1/live/0");
    http_url_parse_and_print("https://127.0.0.1:80/live/0");
    http_url_parse_and_print("httpsxx://127.0.0.1:80/live/0");
    http_url_parse_and_print("httpsxx://127.0.0.1/live/0");
}