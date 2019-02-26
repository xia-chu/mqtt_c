#include "mqtt_wrapper.h"
#include "net.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include "iot_proto.h"

#ifdef __alios__
#include <netmgr.h>
#ifdef AOS_ATCMD
#include <atparser.h>
#endif
#else
#define application_start main
#endif

typedef struct {
    void *_ctx;
    int _fd;
} mqtt_user_data;

int data_output(void *arg, const struct iovec *iov, int iovcnt){
    mqtt_user_data *user_data = (mqtt_user_data *)arg;
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

void handle_pub_ack(void *arg,int time_out,pub_type type){
    LOGI("%d - %d",time_out,type);
    memset(arg,0,4);
}

void handle_sub_ack(void *arg,int time_out,const char *codes, uint32_t count){
    LOGI("%d",time_out);
    memset(arg,0,4);
}

void handle_conn_ack(void *arg, char flags, char ret_code){
    mqtt_user_data *user_data = (mqtt_user_data *)arg;
    LOGI("");
    if(ret_code == 0){
        //success
        const char *topics[] = {"/Service/JIMIMAX/publish","/Service/JIMIMAX/will"};

        mqtt_send_subscribe_pkt(user_data->_ctx,
                                MQTT_QOS_LEVEL2,
                                topics,
                                2,
                                handle_sub_ack,
                                malloc(4),
                                free,
                                10);

        mqtt_send_publish_pkt(user_data->_ctx,
                              "/Service/JIMIMAX/publish",
                              "publishPayload",0,
                              MQTT_QOS_LEVEL2,
                              1,
                              0,
                              handle_pub_ack,
                              malloc(4),
                              free,
                              10);
    }
}
void handle_ping_resp(void *arg){
    mqtt_user_data *user_data = (mqtt_user_data *)arg;
    LOGI("");
}
void handle_publish(void *arg,
                         uint16_t pkt_id,
                         const char *topic,
                         const char *payload,
                         uint32_t payloadsize,
                         int dup,
                         enum MqttQosLevel qos){
    mqtt_user_data *user_data = (mqtt_user_data *)arg;
    LOGI("");
}

void handle_publish_rel(void *arg,
                         uint16_t pkt_id){
    mqtt_user_data *user_data = (mqtt_user_data *)arg;
    LOGI("");
}

//////////////////////////////////////////////////////////////////////

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

    test_iot_packet();

    mqtt_user_data user_data;
    user_data._fd = net_connet_server("10.0.9.56",1883,3);
    if(user_data._fd  == -1){
        return -1;
    }

    mqtt_callback callback = {data_output,handle_conn_ack,handle_ping_resp,handle_publish,handle_publish_rel,&user_data};
    user_data._ctx = mqtt_alloc_contex(&callback);
    mqtt_send_connect_pkt(user_data._ctx,5,"JIMIMAX",1,"/Service/JIMIMAX/will","willPayload",0,MQTT_QOS_LEVEL1, 1,"admin","public");

    net_set_sock_timeout(user_data._fd ,1,1);
    char buffer[1024];
    int timeout = 10;
    while (1){
        int recv = read(user_data._fd,buffer, sizeof(buffer));
        if(recv == 0){
            LOGE("read eof\r\n");
            break;
        }
        if(recv == -1){
            mqtt_timer_schedule(user_data._ctx);
            if(--timeout == 0){
                break;
            }
            continue;
        }
        mqtt_input_data(user_data._ctx,buffer,recv);
    }

    mqtt_free_contex(user_data._ctx);
    return 0;
}