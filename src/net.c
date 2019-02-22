//
// Created by xzl on 2019/2/22.
//

#include "net.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/errno.h>
#include <memory.h>
#include <unistd.h>

#ifndef INADDR_NONE
#define INADDR_NONE ((uint32_t)0xffffffffUL)
#endif

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

int net_connet_server(const char *host, unsigned short port,int second){
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