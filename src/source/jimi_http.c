//
// Created by xzl on 2019-05-21.
//
#include <stdlib.h>
#include <string.h>
#include "jimi_http.h"
#include "jimi_buffer.h"
#include "avl-tree.h"
#include "mqtt_wrapper.h"

typedef struct http_context
{
    buffer _method;
    buffer _url;
    buffer _body;
    AVLTree *_header;
} http_context;

static int AVLTreeCompare(AVLTreeKey key1, AVLTreeKey key2){
    return strcasecmp(key1,key2);
}

http_context *http_context_alloc(){
    http_context *ctx = (http_context*)malloc(sizeof(http_context));
    memset(ctx,0, sizeof(http_context));
    ctx->_header = avl_tree_new(AVLTreeCompare);
    return ctx;
}

int http_context_free(http_context *ctx){
    CHECK_PTR(ctx,-1);
    buffer_release(&ctx->_method);
    buffer_release(&ctx->_url);
    buffer_release(&ctx->_body);
    avl_tree_free(ctx->_header);
    free(ctx);
    return 0;
}

int http_context_set_method(http_context *ctx,const char *method){
    CHECK_PTR(ctx,-1);
    CHECK_PTR(method,-1);
    buffer_assign(&ctx->_method,method,0);
    return 0;
}

int http_context_set_url(http_context *ctx,const char *url){
    CHECK_PTR(ctx,-1);
    CHECK_PTR(url,-1);
    buffer_assign(&ctx->_url,url,0);
    return 0;
}

int http_context_set_body(http_context *ctx,const char *content_type,const char *content_data,int content_len){
    CHECK_PTR(ctx,-1);
    CHECK_PTR(content_type,-1);
    CHECK_PTR(content_data,-1);
    if(content_len <= 0){
        content_len = strlen(content_data);
    }
    buffer_assign(&ctx->_body,content_data,content_len);
    avl_tree_insert(ctx->_header,"Content-Type",strdup(content_type),NULL,free);
    char *len_str = malloc(8);
    sprintf(len_str,"%d",content_len);
    avl_tree_insert(ctx->_header,"Content-Length",len_str,NULL,free);
    return 0;
}

int http_context_add_header(http_context *ctx,const char *key,const char *value){
    CHECK_PTR(ctx,-1);
    CHECK_PTR(key,-1);
    CHECK_PTR(value,-1);
    avl_tree_insert(ctx->_header,strdup(key),strdup(value),free,free);
    return 0;
}

int http_context_dump_to_buffer(http_context *ctx,buffer *out){
    CHECK_PTR(ctx,-1);
    CHECK_PTR(out,-1);
    buffer_init(out);

    buffer_move(out,&ctx->_method);
    buffer_append(out," ",1);
    buffer_append_buffer(out,&ctx->_url);
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


void test_http_context(){
    http_context *ctx = http_context_alloc();
    http_context_set_method(ctx,"POST");
    http_context_set_url(ctx,"/index.html");
    http_context_set_body(ctx,"text/plain","this is a test body",0);
    //测试大小写键覆盖
    http_context_add_header(ctx,"host","baidu.com");
    http_context_add_header(ctx,"Host","baidu.com");
    http_context_add_header(ctx,"Connection","close");
    http_context_add_header(ctx,"Tools","mqtt_c");
    http_context_add_header(ctx,"Accept","*/*");
    http_context_add_header(ctx,"Accept-Language","zh-CN,zh;q=0.8");
    http_context_add_header(ctx,"User-Agent","Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/57.0.2987.133 Safari/537.36");

    buffer out;
    buffer_init(&out);
    http_context_dump_to_buffer(ctx,&out);
    LOGD("\r\n%s",out._data);
    buffer_release(&out);
    http_context_free(ctx);
}