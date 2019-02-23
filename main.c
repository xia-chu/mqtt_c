#include "mqtt_wrapper.h"
#include "net.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>

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
}

void handle_pub_ack(void *arg,pub_type type){
    LOGT("%d",type);
    memset(arg,0,4);
}

void handle_sub_ack(void *arg,const char *codes, uint32_t count){
    LOGT("%d",count);
    memset(arg,0,4);
}

void mqtt_handle_conn_ack(void *arg, char flags, char ret_code){
    mqtt_user_data *user_data = (mqtt_user_data *)arg;
    LOGT("");
    if(ret_code == 0){
        //success
        const char *topics[] = {"/Service/JIMIMAX/publish","/Service/JIMIMAX/will"};

        mqtt_send_subscribe_pkt(user_data->_ctx,
                                MQTT_QOS_LEVEL2,
                                topics,
                                1,
                                handle_sub_ack,
                                malloc(4),
                                free,
                                10);

//        mqtt_send_publish_pkt(user_data->_ctx,
//                              "/Service/JIMIMAX/publish",
//                              "publishPayload",0,
//                              MQTT_QOS_LEVEL2,
//                              1,
//                              0,
//                              handle_pub_ack,
//                              malloc(4),
//                              free,
//                              10);
    }
}
void mqtt_handle_ping_resp(void *arg){
    mqtt_user_data *user_data = (mqtt_user_data *)arg;
    LOGT("");
}
void mqtt_handle_publish(void *arg,
                         uint16_t pkt_id,
                         const char *topic,
                         const char *payload,
                         uint32_t payloadsize,
                         int dup,
                         enum MqttQosLevel qos){
    mqtt_user_data *user_data = (mqtt_user_data *)arg;
    LOGT("");
}

void mqtt_handle_publish_rel(void *arg,
                         uint16_t pkt_id){
    mqtt_user_data *user_data = (mqtt_user_data *)arg;
    LOGT("");
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

    mqtt_user_data user_data;
    user_data._fd = net_connet_server("10.0.9.56",1883,3);
    if(user_data._fd  == -1){
        return -1;
    }
    net_set_sock_timeout(user_data._fd ,1,3);

    mqtt_callback callback = {data_output,mqtt_handle_conn_ack,mqtt_handle_ping_resp,mqtt_handle_publish,mqtt_handle_publish_rel};
    user_data._ctx = mqtt_alloc_contex(callback,(void *)&user_data);
    mqtt_send_connect_pkt(user_data._ctx,120,"JIMIMAX",1,"/Service/JIMIMAX/will","willPayload",0,MQTT_QOS_LEVEL1, 1,"admin","public");

    char buffer[1024];
    while (1){
        int recv = read(user_data._fd,buffer, sizeof(buffer));
        if(recv == 0){
            LOGE("read eof\r\n");
            break;
        }
        if(recv == -1){
            LOGE("read interupted :%d %s\r\n",errno,strerror(errno));
            break;
        }
        mqtt_input_data(user_data._ctx,buffer,recv);
    }

    mqtt_free_contex(user_data._ctx);
    return 0;
}