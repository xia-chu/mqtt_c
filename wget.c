#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include "net.h"
#include "jimi_iot.h"
#include "jimi_log.h"
#include "jimi_http.h"
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>

static int s_exit_flag  = 0;
static int s_last_received_len = 0;
static int s_speed = 0;
static uint64_t s_lastStamp = 0;
static uint64_t s_startTime = 0;

static inline uint64_t getCurrentStamp(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void on_response(void *user_data,
                 http_response *ctx,
                 const char *content_slice,
                 int content_slice_len,
                 int content_received_len,
                 int content_total_len){
    FILE *fp = (FILE *)user_data;
    fwrite(content_slice,1,content_slice_len,fp);
    if(content_received_len == 0){
        printf("http回复:\r\n%s %d %s\r\n",http_response_get_http_version(ctx),http_response_get_status_code(ctx),http_response_get_status_str(ctx));
        int i;
        for (i = 0; i < http_response_get_header_count(ctx); ++i) {
            const char *key,*value;
            http_response_get_header_pair(ctx,i , &key,&value);
            printf("%s = %s\r\n",key,value);
        }
        printf("\r\n");
    }

    if(content_received_len != 0){
        printf("\033[1A"); //先回到上一行
        printf("\033[K"); //清除该行
    }

    if(getCurrentStamp() - s_lastStamp > 500) {
        s_speed = (content_received_len - s_last_received_len) / (getCurrentStamp() - s_lastStamp);
        s_lastStamp = getCurrentStamp();
        s_last_received_len = content_received_len;
    }

    printf("已下载 : %.2f%% , 下载速度:%d KB/s \r\n ",
            100.0 * (content_slice_len + content_received_len) / content_total_len
            ,s_speed);

    if(content_slice_len + content_received_len == content_total_len){
        printf("\r\n文件接收完毕,总耗时:%llu秒,平均速度:%llu KB/s\r\n",(getCurrentStamp() - s_startTime) / 1000,content_total_len / (getCurrentStamp() - s_startTime) );
        s_exit_flag = 1;
    }
}


int main(int argc, char *argv[]){
    //设置日志等级
    set_log_level(log_trace);
    if(argc != 3){
        LOGE("使用方法: wget http://xxxxx/xxxxx /path/to/file");
        return -1;
    }
    http_url *url = http_url_parse(argv[1]);
    if(!url){
        LOGE("URL无效！");
        return -1;
    }

    if(http_url_is_https(url)){
        LOGE("不支持https下载！");
        http_url_free(url);
        return -1;
    }

    FILE *fp = fopen(argv[2],"wb");
    if(!fp){
        LOGE("打开文件失败:%s",argv[2]);
        return -1;
    }
    http_request *request = http_request_alloc();
    http_request_set_method(request,"GET");
    http_request_set_path(request,http_url_get_path(url));
    http_request_add_header_array(request,
                                  "Host",http_url_get_host(url),
                                  "Connection","close",
                                  "Accept","*/*",
                                  "Accept-Language","zh-CN,zh;q=0.8",
                                  "User-Agent","http_c",
                                  NULL);
    buffer req_dump;
    buffer_init(&req_dump);
    http_request_dump_to_buffer(request,&req_dump);
    http_request_free(request);

    //网络层连接服务器
    int fd = net_connet_server(http_url_get_host(url),http_url_get_port(url),5);
    http_url_free(url);
    if(fd == -1){
        LOGE("连接服务器失败！");
        fclose(fp);
        return -1;
    }
    printf("发送请求:\r\n%s\r\n",req_dump._data);
    send(fd,req_dump._data,req_dump._len,0);
    s_startTime = s_lastStamp = getCurrentStamp();
    buffer_release(&req_dump);

    net_set_sock_timeout(fd,1,30);
    //socket接收buffer
    char buffer[1024 * 32];
    http_response *response = http_response_alloc(on_response,fp);
    while (!s_exit_flag){
        //接收数据
        int recv = read(fd,buffer, sizeof(buffer));
        if(recv == 0){
            //服务器断开连接
            LOGE("服务器断开连接\r\n");
            break;
        }
        if(recv == -1 ){
            LOGE("接收超时！\r\n");
            break;
        }
        http_response_input(response,buffer,recv);
    }

    http_response_free(response);
    fclose(fp);
    close(fd);
    return 0;
}



