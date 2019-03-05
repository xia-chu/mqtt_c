#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include "net.h"
#include <stdlib.h>
#include <mqtt_iot.h>
#include <buffer.h>
#include "mqtt_iot.h"
#ifdef __alios__
#include <netmgr.h>
#include <aos/network.h>
#ifdef AOS_ATCMD
#include <atparser.h>
#endif
#else
#define application_start main
#endif


#define CLIENT_ID "IMEI173283796362"
#define USER_NAME "JIMIMAX"
#define SECRET "a40a97ebe8dd73868b0f13c17752c27b"

extern void test_iot_packet();

typedef struct {
    void *_ctx;
    int _fd;
} iot_user_data;


int data_output(void *arg, const struct iovec *iov, int iovcnt){
    iot_user_data *user_data = (iot_user_data *)arg;
#ifdef __alios__
    int ret = 0;
    int sent;
    while(iovcnt--){
        const struct iovec *ptr = iov++;
        sent = send(user_data->_fd,ptr->iov_base , ptr->iov_len,0);
        if(-1 == sent){
            LOGE("failed:%s",strerror(errno));
            return -1;
        }
        ret += sent;
    }
    return ret;
#else
    return writev(user_data->_fd,iov,iovcnt);
#endif
}


//////////////////////////////////////////////////////////////////////

void on_timer_tick(iot_user_data *user_data){
    iot_timer_schedule(user_data->_ctx);
    static int flag = 1;
    flag = !flag;
    if(flag){
        iot_send_bool_pkt(user_data->_ctx,509998,1);
        iot_send_enum_pkt(user_data->_ctx,609995,"e1");
        iot_send_string_pkt(user_data->_ctx,609996,"str1");
        iot_send_double_pkt(user_data->_ctx,609997,3.14);
    }else{
        buffer buffer;
        buffer_init(&buffer);
        iot_buffer_start(&buffer,1,iot_get_request_id(user_data->_ctx));
        iot_buffer_append_bool(&buffer,509998,0);
        iot_buffer_append_enum(&buffer,609995,"e0");
        iot_buffer_append_string(&buffer,609996,"str0");
        iot_buffer_append_double(&buffer,609997,1.23);
        iot_send_buffer(user_data->_ctx,&buffer);
        buffer_release(&buffer);
    }
}

void iot_on_connect(void *arg, char ret_code){
    LOGD("ret_code :%d",ret_code);
}
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

void run_main(){
    /*测试iot打包解包*/
//    test_iot_packet();

    iot_user_data user_data;
    user_data._fd = net_connet_server("172.16.10.115",1883,3);
    if(user_data._fd  == -1){
        return ;
    }

    iot_callback callback = {data_output,iot_on_connect,iot_on_message,&user_data};
    user_data._ctx = iot_context_alloc(&callback);
    iot_send_connect_pkt(user_data._ctx,CLIENT_ID,SECRET,USER_NAME);

    net_set_sock_timeout(user_data._fd ,1,2);
    char buffer[1024];
    int timeout = 30;
    while (1){
        int recv = read(user_data._fd,buffer, sizeof(buffer));
        if(recv == 0){
            LOGE("read eof\r\n");
            break;
        }
        if(recv == -1){
            on_timer_tick(&user_data);
            if(--timeout == 0){
                break;
            }
            continue;
        }
        iot_input_data(user_data._ctx,buffer,recv);
    }

    iot_context_free(user_data._ctx);
}
int application_start(int argc, char *argv[]){
#ifdef __alios__
    #if AOS_ATCMD
    at.set_mode(ASYN);
    at.init(AT_RECV_PREFIX, AT_RECV_SUCCESS_POSTFIX,
            AT_RECV_FAIL_POSTFIX, AT_SEND_DELIMITER, 1000);
#endif

#ifdef WITH_SAL
    sal_init();
#endif

    netmgr_init();
    netmgr_start(false);
#endif

#ifdef __alios__
    aos_post_delayed_action(10 * 1000,run_main,NULL);
    aos_loop_run();
#else
    run_main();
#endif
    return 0;
}

