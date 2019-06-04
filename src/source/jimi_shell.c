//
// Created by xzl on 2019-06-04.
//

#include <stdlib.h>
#include <string.h>
#include "jimi_shell.h"
#include "jimi_buffer.h"
#include "mqtt_wrapper.h"

//最大支持32个参数
#define MAX_ARGV 32

typedef struct shell_context{
    buffer *_buf;
    on_shell_argv _callback;
    void *_user_data;
} shell_context;


shell_context* shell_context_alloc(on_shell_argv callback,void *user_data){
    shell_context *ret = (shell_context *)malloc(sizeof(shell_context));
    CHECK_PTR(ret,NULL);
    ret->_buf = buffer_alloc();
    if(!ret->_buf){
        free(ret);
        return NULL;
    }
    ret->_callback = callback;
    ret->_user_data = user_data;
    return ret;
}

int shell_context_free(shell_context *ctx){
    CHECK_PTR(ctx,-1);
    buffer_free(ctx->_buf);
    free(ctx);
    return 0;
}

static inline int is_blank(char ch){
    switch (ch){
        case '\t':
        case ' ':
            return 1;
        default:
            return 0;
    }
}
static inline void shell_context_cmd_line(shell_context *ctx,char *start,int len){
    start[len] = '\0';
    char *argv[MAX_ARGV];
    memset(argv,0,sizeof(argv));
    int argc = 0;
    char last = ' ';
    char last_last = ' ';

    int i = 0;
    for(i = 0 ; i < len ; ++i){
        if(is_blank(start[i]) && last != '\\'){
            //上个字节不是"\"且本字节是空格或制表符，那么认定该字节是分隔符号
            last_last = last;
            last = start[i];
            start[i] = '\0';
        } else{
            //不是空格
            if(is_blank(last) && last_last != '\\'){
                //上个字节是空格并且上上个字节不是"\"，那么说明是新起参数
                argv[argc++] = start + i;
            }
            last_last = last;
            last = start[i];
        }
    }

    //替换"\ "为空格
    for(i = 0 ; i < argc ; ++i ){
        while(1){
            char *pos = strstr(argv[i],"\\");
            if(!pos){
                break;
            }
            if(pos[1] == ' ' || pos[1] == '\t'){
                memmove(pos , pos + 1,strlen(pos + 1));
            }else{
                break;
            }
        }
    }
    if(ctx->_callback){
        ctx->_callback(ctx->_user_data,argc,argv);
    }
}

int shell_context_input(shell_context *ctx,const char *data,int len){
    CHECK_PTR(ctx,-1);
    CHECK_PTR(data,-1);
    if(len <= 0){
        len = strlen(data);
    }
    CHECK_RET(-1,buffer_append(ctx->_buf,data,len));
    char *start = ctx->_buf->_data;
    char *pos = NULL;
    while (1){
        pos = strstr(start,"\r\n");
        if(!pos){
            //等待更多数据
            break;
        }
        shell_context_cmd_line(ctx,start,pos - start);
        start = pos + 2;
    }

    if(start != ctx->_buf->_data){
        ctx->_buf->_len -=  (start - ctx->_buf->_data);
        memmove(ctx->_buf->_data,start,ctx->_buf->_len + 1);
    }
    return 0;
}

static void argv_test(void *user_data,int argc,char *argv[]){
    LOGI("cmd:");
    int i;
    for(i = 0 ; i < argc ; ++i ){
        LOGD("%s",argv[i]);
    }
}

void test_shell_context(){
    const char http_str[] = "setmqtt --id  iemi1234567890 --secret abcdefghijk  --server iot.\\ \\ jimax.com:1900\r\n"
                            "setmqtt -u \tiemi1234567890 -p   \tabcdefghijk -s \t\t  iot.\\ jimax.com:1900\r\n"
                            "getmqtt\r\n"
                            "setio --piont 1 --flag  0\r\n"
                            "setio -p 1 -f  0\r\n"
                            "getio --piont 1\r\n"
                            "getio -p 1\r\n"
                            "getadc\r\n"
                            "setpwm --hz 3000 --ratio 0.40\r\n"
                            "setpwm -z 3000 -r 0.40\r\n"
                            "getpwm\r\n";

    shell_context *ctx = shell_context_alloc(argv_test,NULL);

    int totalTestCount = 10;
    while (--totalTestCount) {
        int totalSize = sizeof(http_str) - 1;
        int slice = totalSize / totalTestCount;
        const char *ptr = http_str;
        while (ptr + slice < http_str + totalSize) {
            buffer slice_buf;
            buffer_init(&slice_buf);
            buffer_assign(&slice_buf, ptr, slice);
            shell_context_input(ctx, slice_buf._data, slice_buf._len);
            buffer_release(&slice_buf);
            ptr += slice;
        }
        shell_context_input(ctx, ptr, http_str + totalSize - ptr + 1);
    }

    shell_context_free(ctx);
}
