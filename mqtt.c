#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "net.h"
#include "jimi_iot.h"
#include "jimi_log.h"

#ifdef __alios__
#include <netmgr.h>
#include <network/network.h>
#include "app_entry.h"
#ifdef AOS_ATCMD
#include <atparser.h>
#endif
#else
#include <string.h>
#include <errno.h>
#endif


#define CLIENT_ID "IMEI17328379636"
#define USER_NAME "JIMIMAX"
#define SECRET "e145fadffebb304fd87f8502db72f234"
#define SERVER_IP "172.16.0.106"
#define SERVER_PORT 7883

typedef struct {
    //iot_cxt对象
    void *_ctx;
    //套接字描述符
    int _fd;
} iot_user_data;

/**
 * 发送数据至网络
 * @param arg 用户数据指针，为iot_user_data指针
 * @param iov 数据块数组指针
 * @param iovcnt 数据块个数
 * @return -1为失败，>=0 为成功
 */
static int send_data_to_sock(void *arg, const struct iovec *iov, int iovcnt){
    iot_user_data *user_data = (iot_user_data *)arg;
#ifdef __alios__
    int size = 0;
    for (int i = 0; i < iovcnt; ++i) {
        size += (iov + i)->iov_len;
    }
    char *buf = jimi_malloc(size);
    int pos = 0;
    for (int i = 0; i < iovcnt; ++i) {
        memcpy(buf + pos , (iov + i)->iov_base , (iov + i)->iov_len);
        pos += (iov + i)->iov_len;
    }
    int ret = write(user_data->_fd, buf, size);
    jimi_free(buf);
#else
    int ret = writev(user_data->_fd,iov,iovcnt);
#endif
    if(ret == 0){
        LOGW("send failed:%d %s",errno,strerror(errno));
    }
    return ret;
}


//////////////////////////////////////////////////////////////////////
static int flag = 1;
static double db = 0;
/**
 * 定时器触发回调
 * @param user_data 用户数据指针
 */
void on_timer_tick(iot_user_data *user_data){
    iot_timer_schedule(user_data->_ctx);

    flag = !flag;
    db += 0.01 ;
    if(db == 100){
        db = 0;
    }
    char time_str[26];
    get_now_time_str(time_str,sizeof(time_str));

    if(flag){
         iot_send_bool_pkt(user_data->_ctx,210038,1);
         iot_send_enum_pkt(user_data->_ctx,410498,"1");
         iot_send_string_pkt(user_data->_ctx,410499,time_str);
         iot_send_double_pkt(user_data->_ctx,410500,db);
    }else{
        buffer buffer;
        buffer_init(&buffer);
        iot_buffer_start(&buffer,1,iot_get_request_id(user_data->_ctx));
        iot_buffer_append_bool(&buffer,210038,0);
        iot_buffer_append_enum(&buffer,410498,"2");
        iot_buffer_append_string(&buffer,410499,time_str);
        iot_buffer_append_double(&buffer,410500,db);
        iot_send_buffer(user_data->_ctx,&buffer);
        buffer_release(&buffer);
    }
}

/**
 * 登录iot服务器成功后回调
 * @param arg 用户数据指针
 * @param ret_code 错误代码，0为成功
 */
void iot_on_connect(void *arg, char ret_code){
    LOGD("ret_code :%d",ret_code);
}

/**
 * 收到服务器下发的端点数据回调
 * @param arg 用户数据指针,即本结构体的_user_data参数
 * @param req_flag 数据类型，最后一位为0则代表回复，为1代表请求
 * @param req_id 本次请求id
 * @param data 端点数据，只读
 */
void iot_on_message(void *arg,int req_flag, uint32_t req_id, iot_data *data){
    switch (data->_type){
        case iot_bool:
            LOGD("req_flag:%d , req_id:%d , tag_id:%d , type:%d , bool:%d",req_flag,req_id,data->_tag_id,data->_type,data->_data._bool);
            break;
        case iot_string:
            LOGD("req_flag:%d , req_id:%d , tag_id:%d , type:%d , string:%s",req_flag,req_id,data->_tag_id,data->_type,data->_data._string._data);
            break;
        case iot_enum:
            LOGD("req_flag:%d , req_id:%d , tag_id:%d , type:%d , enum:%s",req_flag,req_id,data->_tag_id,data->_type,data->_data._enum._data);
            break;
        case iot_double:
            LOGD("req_flag:%d , req_id:%d , tag_id:%d , type:%d , double:%f",req_flag,req_id,data->_tag_id,data->_type,data->_data._double);
            break;
    }
}

int s_exit_flag  = 0;
void on_stop(int sig){
    s_exit_flag = 1;
}

/**
 * 运行主函数
 */
void run_main(){
    //设置日志等级
    set_log_level(log_trace);

    //数据结构体
    iot_user_data user_data;

    //网络层连接服务器
    user_data._fd = net_connet_server(SERVER_IP,SERVER_PORT,3);
    if(user_data._fd  == -1){
        return ;
    }
    //回调函数列表
    iot_callback callback = {send_data_to_sock,iot_on_connect,iot_on_message,&user_data};

    //创建iot对象
    user_data._ctx = iot_context_alloc(&callback);
    //开始登陆iot服务器
    iot_send_connect_pkt(user_data._ctx,CLIENT_ID,SECRET,USER_NAME);

    //设置socket接收超时时间，两秒超时一次，目的是产生一个定时器
    net_set_sock_timeout(user_data._fd ,1,2);

    //socket接收buffer
    char buffer[1024];
    int timeout = 0x7FFFFFFF;
    signal(SIGINT,on_stop);
    while (!s_exit_flag){
        //接收数据
        int recv = read(user_data._fd,buffer, sizeof(buffer));
        if(recv == 0){
            //服务器断开连接
            LOGE("read eof\r\n");
            break;
        }
        if(recv == -1 ){
            //接收超时，触发定时器
            on_timer_tick(&user_data);
            if(--timeout == 0){
                //程序到了需要退出的时刻
                break;
            }
            continue;
        }
        //收到数据，输入到iot对象
        iot_input_data(user_data._ctx,buffer,recv);
    }
    //是否iot对象
    iot_context_free(user_data._ctx);
}

#ifdef __alios__
int linkkit_main(void *paras){
    int ret;
    int argc = 0;
    char **argv = NULL;

    if (paras != NULL) {
        app_main_paras_t *p = (app_main_paras_t *)paras;
        argc = p->argc;
        argv = p->argv;
    }
    run_main();
}
#else
int main(int argc,char *argv[]){
    run_main();
    return 0;
}

#endif



