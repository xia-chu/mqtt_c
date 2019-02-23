//
// Created by xzl on 2019/2/22.
//

#include "net.h"
#ifdef __alios__
#include <aos/network.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <sys/errno.h>
#include <memory.h>
#include <unistd.h>
#include <string.h>

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
        LOGW("gethostbyname failed, errno: %d(%s) domain %s \r\n", errno,strerror(errno), host);
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

int net_set_sock_timeout(int fd, int recv, float second){
    struct timeval timeout;
    unsigned int milli_sec = second * 1000;
    timeout.tv_sec = milli_sec / 1000;
    timeout.tv_usec = 1000 * (milli_sec % 1000);

    int opt = recv ? SO_RCVTIMEO : SO_SNDTIMEO;
    if (setsockopt(fd, SOL_SOCKET, opt , (char *) &timeout, sizeof(timeout)) == -1) {
        LOGW("setsockopt %d failed, errno: %d(%s)\r\n", opt , errno,strerror(errno));
        return -1;
    }
    return 0;
}
int net_connet_server(const char *host, unsigned short port,float second){
    int sockfd ;
    struct sockaddr_in server_addr;
    if(-1 == get_server_addr(host,port,&server_addr)){
        LOGW("get_server_addr failed, domain %s \r\n", host);
        return -1;
    }

    sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1){
        LOGW("create socket failed, errno %d(%s) \r\n", errno,strerror(errno));
        return -1;
    }

    do {
        if (net_set_sock_timeout(sockfd,1,second) == -1) {
            break;
        }

        if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
            LOGW("connect failed, errno = %d(%s), host %s port %d \r\n", errno,strerror(errno), host, port);
            break;
        }

        //connect success!
        LOGI("connect server %s %d success!\r\n",host,port);
        net_set_sock_timeout(sockfd,1,0);
        return sockfd;
    }while (0);

    close(sockfd);
    return -1;
}