/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <fcntl.h>
#include "aos/kernel.h"
#include <k_api.h>
#include "sensor/sensor.h"
#include "soc_init.h"
#include "jimi_log.h"
#include "senson_event.h"

/* sensor fd */
int fd_acc  = -1;
int fd_temp = -1;
int fd_humi = -1;
int fd_baro = -1;

int timer_ms = 3000;
aos_timer_t timer;

void on_timer(void *timer, void *arg);

void init_sensor(void)
{
    /* open acc sensor */
    fd_acc = aos_open(DEV_ACC_PATH(0), O_RDWR);

    if (fd_acc < 0) {
        LOGW("acc sensor open failed !\n");
    }

    /* open temp sensor */
    fd_temp = aos_open(DEV_TEMP_PATH(0), O_RDWR);

    if (fd_temp < 0) {
        LOGW("temp sensor open failed !\n");
    }

    /* open temp sensor */
    fd_humi = aos_open(DEV_HUMI_PATH(0), O_RDWR);

    if (fd_humi < 0) {
        LOGW("humi sensor open failed !\n");
    }

    fd_baro = aos_open(DEV_BARO_PATH(0), O_RDWR);

    if (fd_baro < 0) {
        LOGW("baro sensor open failed !\n");
    }

    key_init();

    aos_timer_new(&timer,on_timer,NULL,timer_ms,1);
}

int get_acc_data(float *x, float *y, float *z)
{
    accel_data_t data = { 0 };
    ssize_t      size = 0;

    size = aos_read(fd_acc, &data, sizeof(data));
    if (size != sizeof(data)) {
        return -1;
    }

    *x = ((float)data.data[0] / 1000.0f) * 9.8f;
    *y = ((float)data.data[1] / 1000.0f) * 9.8f;
    *z = ((float)data.data[2] / 1000.0f) * 9.8f;

    return 0;
}
 
int get_temperature_data(float *dataT)
{
    temperature_data_t data = { 0 };
    ssize_t            size = 0;

    size = aos_read(fd_temp, &data, sizeof(data));
    if (size != sizeof(data)) {
        return -1;
    }

    *dataT = (float)data.t / 10;

    return 0;
}

int get_humidity_data(float *dataH)
{
    humidity_data_t data = { 0 };
    ssize_t         size = 0;

    size = aos_read(fd_humi, &data, sizeof(data));
    if (size != sizeof(data)) {
        return -1;
    }

    *dataH = (float)data.h / 10;

    return 0;
}

int get_barometer_data(float *dataP)
{
    barometer_data_t data = { 0 };
    ssize_t          size = 0;

    size = aos_read(fd_baro, &data, sizeof(data));
    if (size != sizeof(data)) {
        return -1;
    }

    *dataP = (float)data.p / 1000;

    return 0;
}


void key_handle(void *ptr)
{
    uint32_t value;
    hal_gpio_input_get(&brd_gpio_table[(int)ptr],&value);
    aos_post_event(EV_BUTTON,(int)ptr,(unsigned long)value);
}

void on_timer(void *timer, void *arg){
    {
        float x,y,z;
        if(!get_acc_data(&x,&y,&z)){
            unsigned long value;
            value = 100 * x;
            aos_post_event(EV_ACC,CODE_AAC_X,value);
            value = 100 * y;
            aos_post_event(EV_ACC,CODE_AAC_Y,value);
            value = 100 * z;
            aos_post_event(EV_ACC,CODE_AAC_Z,value);
        }
    }
    {
        float temp;
        if(!get_temperature_data(&temp)){
            unsigned long value;
            value = 100 * temp;
            aos_post_event(EV_TEMP,CODE_TEMP_DATA,value);
        }
    }

    {
        float humi;
        if(!get_humidity_data(&humi)){
            unsigned long value;
            value = 100 * humi;
            aos_post_event(EV_HUMIDITY,CODE_HUMIDITY_DATA,value);
        }
    }


    {
        float barometer;
        if(!get_barometer_data(&barometer)){
            unsigned long value;
            value = 100 * barometer;
            aos_post_event(EV_BAROMETER,CODE_BAROMETER_DATA,value);
        }
    }

}
void key_init(void)
{
    int ret = 0;

    ret |= hal_gpio_enable_irq(&brd_gpio_table[GPIO_KEY_1],IRQ_TRIGGER_BOTH_EDGES, key_handle, GPIO_KEY_1);
    ret |= hal_gpio_enable_irq(&brd_gpio_table[GPIO_KEY_2],IRQ_TRIGGER_BOTH_EDGES, key_handle, GPIO_KEY_2);
    ret |= hal_gpio_enable_irq(&brd_gpio_table[GPIO_KEY_3],IRQ_TRIGGER_BOTH_EDGES, key_handle, GPIO_KEY_3);
    if (ret != 0) {
        LOGW("hal_gpio_enable_irq key return failed.");
    }
}


#ifdef __alios__
#include "aos/kernel.h"
#include "aos/hal/gpio.h"
#include "soc_init.h"
void set_gpio(int port, int config, int type){
    switch (type){
        case 0:
            hal_gpio_output_low(&brd_gpio_table[port]);
            break;
        case 1:
            hal_gpio_output_high(&brd_gpio_table[port]);
            break;
        default:
            hal_gpio_output_toggle(&brd_gpio_table[port]);
            break;
    }
}
#endif