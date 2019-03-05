#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include "net.h"
#include <stdlib.h>
#include "mqtt_iot.h"
#ifdef __alios__
#include <netmgr.h>
#ifdef AOS_ATCMD
#include <atparser.h>
#endif
#else
#define application_start main
#endif


//#define CLIENT_ID "IMEI173283796362"
//#define USER_NAME "JIMIMAX"
//#define SECRET "a40a97ebe8dd73868b0f13c17752c27b"

#define CLIENT_ID "IMEI173283796361"
#define USER_NAME "JIMIMAX"
#define SECRET "d71b4e01c2d1fb299213f8856e7cea07"


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
        sent = write(user_data->_fd,ptr->iov_base , ptr->iov_len);
        if(-1 == sent){
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
    if(flag){
        flag = 0;
        iot_send_bool_pkt(user_data->_ctx,509998,1);
    }
}

void iot_on_connect(void *arg, char ret_code){
    LOGD("ret_code :%d",ret_code);
}
void iot_on_message(void *arg,iot_data *data_aar, int data_count){
}

void run_main(){
    /*测试iot打包解包*/
    test_iot_packet();

    iot_user_data user_data;
    user_data._fd = net_connet_server("172.16.10.115",1883,3);
    if(user_data._fd  == -1){
        return ;
    }

    iot_callback callback = {data_output,iot_on_connect,iot_on_message,&user_data};
    user_data._ctx = iot_context_alloc(&callback);
    iot_send_connect_pkt(user_data._ctx,CLIENT_ID,SECRET,USER_NAME);

    net_set_sock_timeout(user_data._fd ,1,3);
    char buffer[1024];
    int timeout = 1000;
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

