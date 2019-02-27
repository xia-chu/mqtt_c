#include "mqtt_wrapper.h"
#include "net.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <buffer.h>
#include "iot_proto.h"
#include "md5.h"
#include "buffer.h"

#ifdef __alios__
#include <netmgr.h>
#ifdef AOS_ATCMD
#include <atparser.h>
#endif
#else
#define application_start main
#endif


#define CLIENT_ID "IEMI:16666222211111"
#define USER_NAME "JIMIMAX"
#define SECRET "2ea259c374a9432a9bd4269b5ab7f61b"
#define TOPIC_LISTEN ("/terminal/" CLIENT_ID)
#define TOPIC_PUBLISH ("/service/" USER_NAME "/" CLIENT_ID)

typedef struct {
    void *_ctx;
    int _fd;
} mqtt_user_data;

void make_passwd(const char *client_id,
                 const char *secret,
                 const char *user_name,
                 buffer *md5_str_buf,
                 int upCase){
    char passwd[33];
    uint8_t md5_digst[16];
    buffer buf_plain ;
    buffer_init(&buf_plain);
    buffer_append(&buf_plain,client_id,strlen(client_id));
    buffer_append(&buf_plain,secret,strlen(secret));
    buffer_append(&buf_plain,user_name,strlen(user_name));
    LOGT("plain passwd:%s",buf_plain._data);
    md5((uint8_t*)buf_plain._data,buf_plain._len,md5_digst);
    hexdump(md5_digst,MD5_HEX_LEN,passwd, sizeof(passwd),upCase);
    LOGT("md5 str:%s",passwd);
    buffer_assign(md5_str_buf,passwd, sizeof(passwd));
}


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
        const char *topics[] = {TOPIC_LISTEN};
        mqtt_send_subscribe_pkt(user_data->_ctx,
                                MQTT_QOS_LEVEL2,
                                topics,
                                1,
                                handle_sub_ack,
                                malloc(4),
                                free,
                                10);
        mqtt_send_publish_pkt(user_data->_ctx,
                              TOPIC_PUBLISH,
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
    user_data._fd = net_connet_server("172.16.10.115",1883,3);
    if(user_data._fd  == -1){
        return -1;
    }

    mqtt_callback callback = {data_output,handle_conn_ack,handle_ping_resp,handle_publish,handle_publish_rel,&user_data};
    user_data._ctx = mqtt_alloc_contex(&callback);

    {
        buffer md5_str;
        buffer_init(&md5_str);
        make_passwd(CLIENT_ID,SECRET,USER_NAME,&md5_str,1);
        mqtt_send_connect_pkt(user_data._ctx,5,CLIENT_ID,1,NULL,NULL,0,MQTT_QOS_LEVEL1, 0,USER_NAME,md5_str._data);
        buffer_release(&md5_str);
    }

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