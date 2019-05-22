//
// Created by xzl on 2019-05-21.
//
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "jimi_http.h"
#include "jimi_buffer.h"
#include "avl-tree.h"
#include "mqtt_wrapper.h"

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
    http_request *ctx = (http_request*)malloc(sizeof(http_request));
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
    free(ctx);
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
    avl_tree_insert(ctx->_header,"Content-Type",strdup(content_type),NULL,free);
    char *len_str = malloc(8);
    CHECK_PTR(len_str,-1);
    sprintf(len_str,"%d",content_len);
    avl_tree_insert(ctx->_header,"Content-Length",len_str,NULL,free);
    return 0;
}

int http_request_add_header(http_request *ctx,const char *key,const char *value){
    CHECK_PTR(ctx,-1);
    CHECK_PTR(key,-1);
    CHECK_PTR(value,-1);
    avl_tree_insert(ctx->_header,strdup(key),strdup(value),free,free);
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
        free(nodes);
    }
    buffer_append(out,"\r\n",0);
    buffer_append_buffer(out,&ctx->_body);
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
    int _body_offset;
    int _body_len;
    AVLTree *_header;
    on_split_response _split_cb;
    void *_user_data;
    char _http_version[16];
    char _status_str[16];
    int _status_code;
} http_response;


http_response *http_response_alloc(on_split_response cb,void *user_data){
    http_response *ctx = (http_response *)malloc(sizeof(http_response));
    CHECK_PTR(ctx,NULL);
    memset(ctx,0, sizeof(http_response));
    ctx->_header = avl_tree_new(AVLTreeCompare);
    ctx->_split_cb = cb;
    ctx->_user_data = user_data;
    return ctx;
}

http_response *http_response_split(http_response *src){
    http_response *ctx = (http_response *)malloc(sizeof(http_response));
    CHECK_PTR(ctx,NULL);
    memset(ctx,0, sizeof(http_response));
    buffer_move(&ctx->_data,&src->_data);
    ctx->_body_offset = src->_body_offset;
    ctx->_body_len = src->_body_len;
    ctx->_header = src->_header;
    ctx->_user_data = src->_user_data;
    ctx->_split_cb = src->_split_cb;
    strcpy(ctx->_status_str , src->_status_str);
    strcpy(ctx->_http_version , src->_http_version);
    ctx->_status_code = src->_status_code;

    int remain_size = ctx->_data._len - ctx->_body_offset - ctx->_body_len;
    if(remain_size > 0){
        //阶段多余的数据
        buffer_assign(&src->_data,ctx->_data._data + ctx->_body_offset + ctx->_body_len,remain_size);
        *(ctx->_data._data + ctx->_body_offset + ctx->_body_len) = '\0';
    }

    src->_body_offset = 0;
    src->_body_len = 0;
    src->_header = avl_tree_new(AVLTreeCompare);
    return ctx;
}


int http_response_free(http_response *ctx){
    CHECK_PTR(ctx,-1);
    buffer_release(&ctx->_data);
    avl_tree_free(ctx->_header);
    free(ctx);
    return 0;
}

void print_tree(AVLTree *tree){
    AVLTreeNode **nodes = avl_tree_to_array_node(tree);
    unsigned int i;
    for (i = 0 ; i < avl_tree_num_entries(tree) ; ++i){
        AVLTreeKey key = avl_tree_node_key(nodes[i]);
        AVLTreeValue value = avl_tree_node_value(nodes[i]);
        LOGD("%s = %s",key,value);
    }
    free(nodes);
}


int http_response_input(http_response *ctx,const char *data,int len){
    CHECK_PTR(ctx,-1);
    CHECK_PTR(data,-1);
    if (len <= 0){
        len = strlen(data);
    }
    buffer_append(&ctx->_data,data,len);

parse_response:

    if(ctx->_body_offset != 0){
        //已经接收http头完毕
        if(ctx->_body_len > 0){
            if(ctx->_data._len <  ctx->_body_offset + ctx->_body_len){
                //http body尚未接收完毕
                return len;
            }
        }

        //收到一个完整的http回复包
        http_response *ret = http_response_split(ctx);
        if(ctx->_split_cb){
            ctx->_split_cb(ctx->_user_data,ret);
        }
        http_response_free(ret);
        //看看剩余数据是否还能split出http回复包
    }

    if(!ctx->_data._len){
        //没有剩余数据了
        return len;
    }
    char *pos = strstr(ctx->_data._data,"\r\n\r\n");
    if(!pos){
        return len;
    }

    ctx->_body_offset = pos + 4 - ctx->_data._data;
    char *line_start ,*line_end ;
    for(line_end = ctx->_data._data ; line_end < ctx->_data._data + ctx->_body_offset - 2; ){
        line_start = line_end;
        line_end = strstr(line_start,"\r\n");
        if(line_start == ctx->_data._data){
            sscanf(line_start,"%15s %d %15[^\r]",ctx->_http_version,&ctx->_status_code,ctx->_status_str);
            continue;
        }

        *line_end = '\0';
        line_end += 2;
        pos = strstr(line_start,": ");
        if(!pos){
            continue;
        }
        *pos = '\0';
        avl_tree_insert(ctx->_header,strdup(line_start),strdup(pos + 2),free,free);
    }

    const char *content_len = http_response_get_header(ctx,"Content-Length");
    if(content_len){
        ctx->_body_len = atoi(content_len);
    } else {
        ctx->_body_len = 0;
    }
    goto parse_response;
}

const char *http_response_get_header(http_response *ctx,const char *key){
    return (const char *)avl_tree_lookup(ctx->_header,(AVLTreeKey)key);
}


const char *http_response_get_body(http_response *ctx){
    if(ctx->_body_len){
        return NULL;
    }
    return ctx->_data._data + ctx->_body_offset;
}

int http_response_get_bodylen(http_response *ctx){
    return ctx->_body_len;
}

int http_response_get_status_code(http_response *ctx){
    return ctx->_status_code;
}

const char *http_response_get_status_str(http_response *ctx){
    return ctx->_status_str;
}

const char *http_response_get_http_version(http_response *ctx){
    return ctx->_http_version;
}

void on_split_http_response(void *user_data,http_response *res){
    print_tree(res->_header);
    LOGD("%s %d %s",res->_http_version,res->_status_code,res->_status_str);
    LOGD("body :%d %s",res->_body_len,res->_data._data + res->_body_offset);
}

void test_http_response(){
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
            "Server: ZLMediaKit-4.0\r\n\r\nasdadaad";

    http_response *res = http_response_alloc(on_split_http_response,NULL);
    int totalSize = sizeof(http_str) - 1;
    int slice = totalSize  / 13;
    const char *ptr = http_str;
    while(ptr + slice < http_str + totalSize){
        http_response_input(res,ptr,slice);
        ptr += slice;
    }
    http_response_input(res,ptr,http_str + totalSize - ptr + 1);
//    http_response_input(res,http_str, sizeof(http_str) - 1);
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
    char *http = malloc(16), *host = malloc(128);
    char *path = malloc(strlen(url));
    unsigned short port = 0;
    int is_https = 0;

    if (4 == sscanf(url, "%15[^://]://%127[^:]:%hd%s", http, host, &port, path)) {
    } else if(3 == sscanf(url, "%15[^://]://%127[^/]%s", http, host, path)) {
    } else if(3 == sscanf(url,"%15[^://]://%127[^:]:%hd",http,host,&port)){
        free(path);
        path = NULL;
    } else if(2 == sscanf(url,"%15[^://]://%127[^/]",http,host)){
        free(path);
        path = NULL;
    } else{
        free(path);
        free(http);
        free(host);
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
        free(path);
        free(http);
        free(host);
        return NULL;
    }
    free(http);

    http_url *ctx = (http_url *) malloc(sizeof(http_url));
    CHECK_PTR(ctx,NULL);
    ctx->_port = port;
    ctx->_is_https = is_https;
    ctx->_path = path ? path : strdup("/");
    ctx->_host = host;
    return ctx;
}

int http_url_free(http_url *ctx){
    CHECK_PTR(ctx,-1);
    free(ctx->_path);
    free(ctx->_host);
    free(ctx);
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