#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include "mqtt_wrapper.h"
#include "net.h"
#include <stdlib.h>
#include "buffer.h"
#include "base64.h"
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


#define CLIENT_ID "IMEI173283796361"
#define USER_NAME "JIMIMAX"
#define SECRET "d71b4e01c2d1fb299213f8856e7cea07"
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
    LOGI("time_out:%d ,pub_type:%d",time_out,type);
    memset(arg,0,4);
}

void handle_sub_ack(void *arg,int time_out,const char *codes, uint32_t count){
    LOGI("time_out:%d",time_out);
    memset(arg,0,4);
}

void handle_conn_ack(void *arg, char flags, char ret_code){
    mqtt_user_data *user_data = (mqtt_user_data *)arg;
    LOGI("");
    if(ret_code == 0){
        //success
        const char *topics[] = {TOPIC_LISTEN};
        mqtt_send_subscribe_pkt(user_data->_ctx,
                                MQTT_QOS_LEVEL1,
                                topics,
                                1,
                                handle_sub_ack,
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

    int buf_size = payloadsize * 3 / 4 +10;
    uint8_t *out = malloc(buf_size);
    int size = av_base64_decode(out,payload,buf_size);
    dump_iot_pack(out, size);
    free(out);
}

void handle_publish_rel(void *arg,
                         uint16_t pkt_id){
    mqtt_user_data *user_data = (mqtt_user_data *)arg;
    LOGI("");
}

//////////////////////////////////////////////////////////////////////
void publish_tag_switch(mqtt_user_data *user_data,int tag,int flag){
    static int req_id = 0;
    unsigned char iot_buf[1024] = {0};
    int iot_len = pack_iot_bool_packet(1,++req_id,tag,flag,iot_buf, sizeof(iot_buf));
    if(iot_len > 0){
        int out_size = AV_BASE64_SIZE(iot_len) + 10;
        char *out = malloc(out_size);
        av_base64_encode(out,out_size,iot_buf,iot_len);
        free(out);

        mqtt_send_publish_pkt(user_data->_ctx,
                              TOPIC_PUBLISH,
                              (const char *)out,
                              0,
                              MQTT_QOS_LEVEL1,
                              0,
                              0,
                              handle_pub_ack,
                              malloc(4),
                              free,
                              10);
    }

}

void on_timer_tick(mqtt_user_data *user_data){
    mqtt_timer_schedule(user_data->_ctx);
    static int flag = 1;
    if(flag){
//        flag = 0;
        publish_tag_switch(user_data,509998,1);
    }
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
    /*测试iot打包解包*/
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
        make_passwd(CLIENT_ID,SECRET,USER_NAME,&md5_str,0);
        mqtt_send_connect_pkt(user_data._ctx,60,CLIENT_ID,1,NULL,NULL,0,MQTT_QOS_LEVEL1, 0,USER_NAME,md5_str._data);
        buffer_release(&md5_str);
    }

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
        mqtt_input_data(user_data._ctx,buffer,recv);
    }

    mqtt_free_contex(user_data._ctx);
    return 0;
}

