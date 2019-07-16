/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <fcntl.h>
#include "aos/kernel.h"
#include <k_api.h>
#include "sensor/sensor.h"
#include "soc_init.h"


/* sensor fd */
int fd_acc  = -1;
int fd_temp = -1;
int fd_humi = -1;
int fd_baro = -1;

void app_init(void)
{
    /* open acc sensor */
    fd_acc = aos_open(DEV_ACC_PATH(0), O_RDWR);

    if (fd_acc < 0) {
        printf("acc sensor open failed !\n");
    }

    /* open temp sensor */
    fd_temp = aos_open(DEV_TEMP_PATH(0), O_RDWR);

    if (fd_temp < 0) {
        printf("temp sensor open failed !\n");
    }

    /* open temp sensor */
    fd_humi = aos_open(DEV_HUMI_PATH(0), O_RDWR);

    if (fd_humi < 0) {
        printf("humi sensor open failed !\n");
    }

    fd_baro = aos_open(DEV_BARO_PATH(0), O_RDWR);

    if (fd_baro < 0) {
        printf("baro sensor open failed !\n");
    }
}

static int get_acc_data(float *x, float *y, float *z)
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

static int get_temperature_data(float *dataT)
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

static int get_Humidity_data(float *dataH)
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

static int get_barometer_data(float *dataP)
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

static int get_noise_data(float *data)
{
    static float cnt = 0.0f;

    if (cnt > 10.0f) {
        cnt = 0.0f;
    }

    *data = 25 + 0.1f * cnt;

    cnt += 1;

    return 0;
}

void key_init(void)
{
    int ret = 0;

    ret |= hal_gpio_enable_irq(&brd_gpio_table[GPIO_KEY_1],
                               IRQ_TRIGGER_RISING_EDGE, key1_handle, NULL);
    ret |= hal_gpio_enable_irq(&brd_gpio_table[GPIO_KEY_2],
                               IRQ_TRIGGER_RISING_EDGE, key2_handle, NULL);
    ret |= hal_gpio_enable_irq(&brd_gpio_table[GPIO_KEY_3],
                               IRQ_TRIGGER_RISING_EDGE, key3_handle, NULL);
    if (ret != 0) {
        printf("hal_gpio_enable_irq key return failed.\n");
    }
}

void lvgl_drv_register(void)
{
    lv_disp_drv_init(&dis_drv);

    dis_drv.disp_flush = my_disp_flush;
    dis_drv.disp_fill  = my_disp_fill;
    dis_drv.disp_map   = my_disp_map;
    lv_disp_drv_register(&dis_drv);
}

void my_disp_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                   const lv_color_t *color_p)
{
    int32_t x = 0;
    int32_t y = 0;

    for (y = y1; y <= y2; y++) {
        ST7789H2_WriteLine(x1, y, (uint8_t *)color_p, (x2 - x1 + 1));
        color_p += (x2 - x1 + 1);
    }

    lv_flush_ready();
}

void my_disp_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                  lv_color_t color)
{
    int32_t i = 0;
    int32_t j = 0;

    for (i = x1; i <= x2; i++) {
        for (j = y1; j <= y2; j++) {
            ST7789H2_WritePixel(i, j, color.full);
        }
    }
}

void my_disp_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                 const lv_color_t *color_p)
{
    int32_t x = 0;
    int32_t y = 0;

    for (y = y1; y <= y2; y++) {
        ST7789H2_WriteLine(x1, y, (int16_t *)color_p, (x2 - x1 + 1));
        color_p += (x2 - x1 + 1);
    }
}
