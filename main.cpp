#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#include "mqtt.h"
#ifdef __cplusplus
}
#endif // __cplusplus

int main() {
    std::cout << "Hello, World!" << std::endl;

    MqttContext contex;
    Mqtt_InitContext(&contex,1024);
    return 0;
}