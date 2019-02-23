#include "mqtt_wrapper.h"
#include "net.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>

#ifdef __alios__
#include <netmgr.h>
#ifdef AOS_ATCMD
#include <atparser.h>
#endif
#else
#define application_start main
#endif


int data_output(void *user_data, const struct iovec *iov, int iovcnt){
    int ret = 0;
    int sent;
    while(iovcnt--){
        const struct iovec *ptr = iov++;
        sent = write((int)user_data,ptr->iov_base , ptr->iov_len);
        if(-1 == sent){
            return -1;
        }
        ret += sent;
    }
    return ret;
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

    int fd = net_connet_server("10.0.9.56",1883,3);
    if(fd == -1){
        return -1;
    }
    net_set_sock_timeout(fd,1,3);

    mqtt_callback callback = {data_output};
    void *mqtt = mqtt_alloc_contex(callback,(void *)fd);
    mqtt_send_connect_pkt(mqtt,120,"JIMIMAX",1,"/Service/JIMIMAX/will","willPayload",0,MQTT_QOS_LEVEL1, 1,"admin","public");

    char buffer[1024];
    while (1){
        int recv = read(fd,buffer, sizeof(buffer));
        if(recv == 0){
            LOGE("read eof\r\n");
            break;
        }
        if(recv == -1){
            LOGE("read interupted :%d %s\r\n",errno,strerror(errno));
            break;
        }
        mqtt_input_data(mqtt,buffer,recv);
    }

    mqtt_free_contex(mqtt);
    return 0;
}