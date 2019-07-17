#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "net.h"
#include "jimi_iot.h"
#include "jimi_log.h"
#include "senson_event.h"
#include "app_entry.h"

#include <netmgr.h>
#include <network/network.h>
#include <aos/yloop.h>
#include <aos/kernel.h>
#include <aos/hal/gpio.h>
#include <soc_init.h>
#ifdef AOS_ATCMD
#include <atparser.h>
#endif

typedef struct {
    //iot_cxt对象
    void *_ctx;
    //套接字描述符
    int _fd;
} iot_user_data;

static char s_user_name[] = "JIMIMAX";
static char s_client_id[32] = "IMEI17328379634";
static char s_secret[32] = "3ec79d7a4da932faf834c15b687d8caf";
static char s_server_ip[16] = "39.108.84.233";
static int  s_server_port = 1883;
static int  s_timer_ms = 3000;

static void on_timer(iot_user_data *user_data);
extern void set_gpio(int port, int config, int type);
extern void init_sensor();

/**
 * 发送数据至网络
 * @param arg 用户数据指针，为iot_user_data指针
 * @param iov 数据块数组指针
 * @param iovcnt 数据块个数
 * @return -1为失败，>=0 为成功
 */
static int send_data_to_sock(void *arg, const struct iovec *iov, int iovcnt){
    iot_user_data *user_data = (iot_user_data *)arg;
    int size = 0;
    for (int i = 0; i < iovcnt; ++i) {
        size += (iov + i)->iov_len;
    }
    char *buf = jimi_malloc(size);
    int pos = 0;
    for (int i = 0; i < iovcnt; ++i) {
        memcpy(buf + pos , (iov + i)->iov_base , (iov + i)->iov_len);
        pos += (iov + i)->iov_len;
    }
    int ret = write(user_data->_fd, buf, size);
    jimi_free(buf);
    if(ret == 0){
        LOGW("send failed:%d %s",errno,strerror(errno));
    }
    return ret;
}


/**
 * 收到服务器下发的端点数据回调
 * @param arg 用户数据指针,即本结构体的_user_data参数
 * @param req_flag 数据类型，最后一位为0则代表回复，为1代表请求
 * @param req_id 本次请求id
 * @param data 端点数据，只读
 */
static void on_iot_message(void *arg,int req_flag, uint32_t req_id, iot_data *data){
    switch (data->_tag_id){
        case 210112:
            //led0
            set_gpio(GPIO_LED_1,OUTPUT_PUSH_PULL,!data->_data._bool);
            break;
        case 210115:
            //led1
            set_gpio(GPIO_LED_2,OUTPUT_PUSH_PULL,!data->_data._bool);
            break;
        case 210114:
            //led2
            set_gpio(GPIO_LED_3,OUTPUT_PUSH_PULL,!data->_data._bool);
            break;
        default:
            break;
    }
}

static void on_event(input_event_t *event, iot_user_data *user_data) {
    switch (event->type){
        case EV_BUTTON:
            switch (event->code){
                case CODE_BUTTON_0:{
                    iot_send_bool_pkt(user_data->_ctx,210120,event->value);
                }
                    break;
                case CODE_BUTTON_1:{
                    iot_send_bool_pkt(user_data->_ctx,210121,event->value);
                }
                    break;
                case CODE_BUTTON_2:{
                    iot_send_bool_pkt(user_data->_ctx,210122,event->value);
                }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

static void on_timer(iot_user_data *user_data){
    //定时器
    iot_timer_schedule(user_data->_ctx);
    buffer buffer;
    buffer_init(&buffer);
    iot_buffer_start(&buffer,1,iot_get_request_id(user_data->_ctx));

    {
        float x,y,z;
        if(!get_acc_data(&x,&y,&z)){
            char buf[128] = {0};
            sprintf(buf,"%0.2f %0.2f %0.2f",x,y,z);
            iot_buffer_append_string(&buffer,210116,buf);

        }
    }
    {
        float temp;
        if(!get_temperature_data(&temp)){
            iot_buffer_append_double(&buffer,210117,temp);
        }
    }

    {
        float humi;
        if(!get_humidity_data(&humi)){
            iot_buffer_append_double(&buffer,210118,humi);
        }
    }


    {
        float barometer;
        if(!get_barometer_data(&barometer)){
            iot_buffer_append_double(&buffer,210119,barometer);
        }
    }

    iot_send_buffer(user_data->_ctx,&buffer);
    buffer_release(&buffer);
    //下次执行
    aos_post_delayed_action(s_timer_ms,on_timer,user_data);
}

/**
 * 登录iot服务器成功后回调
 * @param arg 用户数据指针
 * @param ret_code 错误代码，0为成功
 */
void on_iot_connect(iot_user_data *arg, char ret_code){
    if(ret_code == 0){
        LOGI("登录mqtt服务器成功");
        static int s_flag = 0;
        if(!s_flag){
            s_flag = true;
            init_sensor();
            aos_register_event_filter(EV_BUTTON, on_event, arg);
            on_timer(arg);
        }
    }else{
        LOGW("登录mqtt服务器失败:%d",ret_code);
    }
}


/**
 * 运行主函数
 */
void run_main(){
    //设置日志等级
    set_log_level(log_trace);

    //网络层连接服务器
    static iot_user_data user_data;
    user_data._fd = net_connet_server(s_server_ip,s_server_port,3);
    if(user_data._fd  == -1){
        return ;
    }
    //回调函数列表
    iot_callback callback = {send_data_to_sock,on_iot_connect,on_iot_message,&user_data};

    //创建iot对象
    user_data._ctx = iot_context_alloc(&callback);
    //开始登陆iot服务器
    iot_send_connect_pkt(user_data._ctx,s_client_id,s_secret,s_user_name);

    //socket接收buffer
    char buffer[1024];
    while (1){
        //接收数据
        int recv = read(user_data._fd,buffer, sizeof(buffer));
        if(recv == 0){
            //服务器断开连接
            LOGE("read eof\r\n");
            break;
        }
        //收到数据，输入到iot对象
        iot_input_data(user_data._ctx,buffer,recv);
    }
    //是否iot对象
    iot_context_free(user_data._ctx);
}


static char *aos_strdup(const char *str){
    char *ret = jimi_malloc(strlen(str));
    strcpy(ret,str);
    return ret;
}

static void setup_memory(){
    set_malloc_ptr(aos_malloc);
    set_free_ptr(aos_free);
    set_realloc_ptr(aos_realloc);
    set_strdup_ptr(aos_strdup);
}

int linkkit_main(void *paras){
    int ret;
    int argc = 0;
    char **argv = NULL;
    if (paras != NULL) {
        app_main_paras_t *p = (app_main_paras_t *)paras;
        argc = p->argc;
        argv = p->argv;
    }
    setup_memory();
    run_main();
}



