#include "soc_init.h"

#ifndef _SENSOR_EVENT_H_
#define _SENSOR_EVENT_H_

//按键事件
#define EV_BUTTON 0x0010
#define CODE_BUTTON_0 GPIO_KEY_1
#define CODE_BUTTON_1 GPIO_KEY_2
#define CODE_BUTTON_2 GPIO_KEY_3

void init_sensor(void);
int get_acc_data(float *x, float *y, float *z);
int get_temperature_data(float *dataT);
int get_humidity_data(float *dataH);
int get_barometer_data(float *dataP);


#endif//_SENSOR_EVENT_H_
