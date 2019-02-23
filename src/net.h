//
// Created by xzl on 2019/2/22.
//

#ifndef MQTT_NET_H
#define MQTT_NET_H

#include "log.h"

int net_connet_server(const char *host, unsigned short port,float second);
int net_set_sock_timeout(int fd, int recv, float second);

#endif //MQTT_NET_H
