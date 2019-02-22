//
// Created by xzl on 2019/2/22.
//

#include "mqtt_wrapper.h"
#include "buffer.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/errno.h>
#include <memory.h>
#include <unistd.h>

#ifndef INADDR_NONE
#define INADDR_NONE ((uint32_t)0xffffffffUL)
#endif

const char *g_log_lev_str[] = {"T","D","I","W","E"};

in_addr_t get_host_addr(const char *host){
    in_addr_t ret = inet_addr(host);
    struct hostent *hostinfo = NULL;

    if(INADDR_NONE != ret){
        return ret;
    }

    if ((hostinfo = gethostbyname(host)) == NULL) {
        LOGW("gethostbyname failed, errno: %d domain %s \r\n", errno, host);
        return INADDR_NONE;
    }

    ret = (*((struct in_addr *)hostinfo->h_addr)).s_addr;
    return ret;
}

int get_server_addr(const char *host, unsigned short port, struct sockaddr_in *server_addr){
    in_addr_t addr = get_host_addr(host);
    if(INADDR_NONE == addr){
        return -1;
    }
    memset(server_addr, 0, sizeof(struct sockaddr_in));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    server_addr->sin_addr.s_addr = addr;
    return 0;
}

int connet_server(const char *host, unsigned short port,int second){
    int sockfd ;
    struct timeval timeout;
    struct sockaddr_in server_addr;
    if(-1 == get_server_addr(host,port,&server_addr)){
        LOGW("get_server_addr failed, domain %s \r\n", host);
        return -1;
    }

    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1){
        LOGW("create socket failed, errno %d \r\n", errno);
        return -1;
    }

    timeout.tv_sec = second;
    timeout.tv_usec = 0;

    do {
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout)) == -1) {
            LOGW("setsockopt failed, errno: %d\r\n", errno);
            break;
        }

        if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
            LOGW("connect failed, errno = %d, host %s port %d \r\n", errno, host, port);
            break;
        }

        //connect success!
        LOGI("connect server %s %d success!\r\n",host,port);
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout));
        return sockfd;
    }while (0);

    close(sockfd);
    return -1;
}

static int mqtt_write_sock(void *arg, const struct iovec *iov, int iovcnt){
    mqtt_context *ctx = (mqtt_context *)arg;
    CHECK_CTX(ctx);
    return ctx->_on_send(ctx,iov,iovcnt);
}

static int handle_ping_resp(void *arg){/**< 处理ping响应的回调函数，成功则返回非负数 */
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("\r\n");
    return 0;
}


static int handle_conn_ack(void *arg, char flags, char ret_code){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("\r\n");
    return 0;
}

static int handle_publish(void *arg, uint16_t pkt_id, const char *topic,
                   const char *payload, uint32_t payloadsize,
                   int dup, enum MqttQosLevel qos){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("%s %s\r\n",topic,payload);
    return 0;
}

static int handle_pub_ack(void *arg, uint16_t pkt_id){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("\r\n");
    return 0;
}

static int handle_pub_rec(void *arg, uint16_t pkt_id){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("\r\n");
    return 0;
}

static int handle_pub_rel(void *arg, uint16_t pkt_id){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("\r\n");
    return 0;
}

static int handle_pub_comp(void *arg, uint16_t pkt_id){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("\r\n");
    return 0;
}

static int handle_sub_ack(void *arg, uint16_t pkt_id,const char *codes, uint32_t count){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("\r\n");
    return 0;
}

static int handle_unsub_ack(void *arg, uint16_t pkt_id){
    mqtt_context *ctx = (mqtt_context *)arg;
    LOGD("\r\n");
    return 0;
}


int mqtt_init_contex(mqtt_context *ctx,FUNC_send_sock on_send){
    memset(ctx,0, sizeof(mqtt_context));
    ctx->_fd = -1;

    MqttBuffer_Init(&ctx->_buffer);
    ctx->_ctx.user_data = ctx;
    ctx->_ctx.writev_func = mqtt_write_sock;
    ctx->_ctx.handle_ping_resp = handle_ping_resp;
    ctx->_ctx.handle_conn_ack = handle_conn_ack;
    ctx->_ctx.handle_publish = handle_publish;
    ctx->_ctx.handle_pub_ack = handle_pub_ack;
    ctx->_ctx.handle_pub_rec = handle_pub_rec;
    ctx->_ctx.handle_pub_rel = handle_pub_rel;
    ctx->_ctx.handle_pub_comp = handle_pub_comp;
    ctx->_ctx.handle_sub_ack = handle_sub_ack;
    ctx->_ctx.handle_unsub_ack = handle_unsub_ack;
    ctx->_on_send = on_send;
    buffer_init(&ctx->_remain_data);
    return 0;
}

void mqtt_release_contex(mqtt_context *ctx){
    MqttBuffer_Destroy(&ctx->_buffer);
    if(ctx->_fd != -1){
        close(ctx->_fd);
    }
    memset(ctx,0, sizeof(mqtt_context));
    ctx->_fd = -1;
}
int mqtt_connect_host(mqtt_context *ctx,
                      const char *host,
                      unsigned short port,int timeout){
    ctx->_fd = connet_server(host,port,timeout);
    if(ctx->_fd == -1){
        return -1;
    }
    return 0;
}


int mqtt_send_packet(mqtt_context *ctx){
    CHECK_CTX(ctx);
    CHECK_RET(0,Mqtt_SendPkt(&ctx->_ctx,&ctx->_buffer,0));
    MqttBuffer_Reset(&ctx->_buffer);
    return 0;
}

int mqtt_send_connect_pkt(mqtt_context *ctx,
                          int keep_alive,
                          const char *id,
                          int clean_session,
                          const char *will_topic,
                          const char *will_payload,
                          int will_payload_len,
                          enum MqttQosLevel qos,
                          int will_retain,
                          const char *user,
                          const char *password){
    if(will_payload_len <= 0){
        will_payload_len = strlen(will_payload);
    }
    CHECK_CTX(ctx);
    CHECK_RET(-1,Mqtt_PackConnectPkt(&ctx->_buffer,
                                     keep_alive,
                                     id,
                                     clean_session,
                                     will_topic,
                                     will_payload,
                                     will_payload_len,
                                     qos,
                                     will_retain,
                                     user,
                                     password,
                                     strlen(password)));
    CHECK_RET(-1,mqtt_send_packet(ctx));
    return 0;
}

int mqtt_input_data_l(mqtt_context *ctx,char *data,int len) {
    int customed = 0;
    int ret = Mqtt_RecvPkt(&ctx->_ctx,data,len,&customed);
    if(ret != MQTTERR_NOERROR){
        LOGW("Mqtt_RecvPkt failed:%d\r\n", ret);
        return -1;
    }

    if(customed == 0){
        //完全为消费数据
        if(!ctx->_remain_data._len){
            buffer_assign(&ctx->_remain_data,data,len);
        }
        return 0;
    }
    if(customed < len){
        //消费部分数据
        buffer buffer;
        buffer_assign(&buffer,data + customed,len - customed);
        buffer_move(&ctx->_remain_data,&buffer);
        return 0;
    }
    //消费全部数据
    buffer_release(&ctx->_remain_data);
    return 0;
}

int mqtt_input_data(mqtt_context *ctx,char *data,int len){
    if(!ctx->_remain_data._len){
        return mqtt_input_data_l(ctx,data,len);
    }
    buffer_append(&ctx->_remain_data,data,len);
    return mqtt_input_data_l(ctx,ctx->_remain_data._data,ctx->_remain_data._len);
}