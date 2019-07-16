#include "soc_init.h"

#ifndef _SENSOR_EVENT_H_
#define _SENSOR_EVENT_H_

//按键事件
#define EV_BUTTON 0x0010
#define CODE_BUTTON_0 GPIO_KEY_1
#define CODE_BUTTON_1 GPIO_KEY_2
#define CODE_BUTTON_2 GPIO_KEY_3

//acc传感器
#define EV_ACC 0x0011
#define CODE_AAC_X 0
#define CODE_AAC_Y 1
#define CODE_AAC_Z 2

//温度传感器
#define EV_TEMP 0x0012
#define CODE_TEMP_DATA 0

//湿度传感器
#define EV_HUMIDITY 0x0013
#define CODE_HUMIDITY_DATA 0

//气压传感器
#define EV_BAROMETER 0x0014
#define CODE_BAROMETER_DATA 0

void init_sensor(void);

#endif//_SENSOR_EVENT_H_
