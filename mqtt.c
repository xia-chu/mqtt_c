#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "net.h"
#include "jimi_iot.h"
#include "jimi_log.h"
#include "jimi_memory.h"
#include "jimi_shell.h"

#ifdef __alios__
#include <netmgr.h>
#include <network/network.h>
#include "app_entry.h"
#ifdef AOS_ATCMD
#include <atparser.h>
#endif
#else
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#endif

#define FAILED "Failed: "
#define SUCCESS "OK: "

//////////////////////////////////////////////////////////version命令////////////////////////////////////////////////////

#define VERSION "1.0.0"
static void on_complete_version(void *user_data, printf_func func,cmd_context *cmd,opt_map all_opt) {
    func(user_data,SUCCESS"%s\r\n",VERSION);
}

//////////////////////////////////////////////////////////setmqtt命令////////////////////////////////////////////////////
static char mqtt_client_id[32] = {0};
static char mqtt_secret[32] = {0};
static char mqtt_server[32] = {0};

static option_value_ret on_option_setmqtt_server(void *user_data,printf_func func,cmd_context *cmd,const char *opt_long_name,const char *opt_val){
    const char *pos = strstr(opt_val,":");
    if(!pos){
        func(user_data,FAILED"%d %s\r\n",-1, "mqtt服务器地址无效，必须包含冒号分隔地址与端口号");
        return ret_interrupt;
    }
    return ret_continue;
}

static void on_complete_setmqtt(void *user_data, printf_func func,cmd_context *cmd,opt_map all_opt){
    strcpy(mqtt_client_id,opt_map_get_value(all_opt,"client_id"));
    strcpy(mqtt_secret,opt_map_get_value(all_opt,"secret"));
    strcpy(mqtt_server,opt_map_get_value(all_opt,"server"));
    func(user_data,SUCCESS"%s %s %s\r\n",mqtt_client_id,mqtt_secret,mqtt_server);
}

//////////////////////////////////////////////////////////getmqtt命令////////////////////////////////////////////////////

static void on_complete_getmqtt(void *user_data, printf_func func,cmd_context *cmd,opt_map all_opt){
    func(user_data,SUCCESS"%s %s %s\r\n",mqtt_client_id,mqtt_secret,mqtt_server);
}

//////////////////////////////////////////////////////////setio命令//////////////////////////////////////////////////////

static void on_complete_setio(void *user_data, printf_func func,cmd_context *cmd,opt_map all_opt){
    int flag = atoi(opt_map_get_value(all_opt,"flag"));
    int point = atoi(opt_map_get_value(all_opt,"point"));
    func(user_data,SUCCESS"已经设置管脚:%d电平为:%d\r\n",point,flag ? 1 : 0 );
}

//////////////////////////////////////////////////////////getio命令//////////////////////////////////////////////////////

static void on_complete_getio(void *user_data, printf_func func,cmd_context *cmd,opt_map all_opt){
    int point = atoi(opt_map_get_value(all_opt,"point"));
    func(user_data,SUCCESS"%d %d\r\n",point,1);
}

//////////////////////////////////////////////////////////getadc命令//////////////////////////////////////////////////////

static void on_complete_getadc(void *user_data, printf_func func,cmd_context *cmd,opt_map all_opt){
    func(user_data,SUCCESS"%0.2f\r\n",1.28);
}

//////////////////////////////////////////////////////////setpwm命令//////////////////////////////////////////////////////
static int hz = 1000;
static float ratio = 0.5f;

static void on_complete_setpwm(void *user_data, printf_func func,cmd_context *cmd,opt_map all_opt){
    hz = atoi(opt_map_get_value(all_opt,"hz"));
    ratio = atof(opt_map_get_value(all_opt,"ratio"));
    func(user_data,SUCCESS"%d %0.2f\r\n",hz,ratio);
}

//////////////////////////////////////////////////////////getpwm命令//////////////////////////////////////////////////////

static void on_complete_getpwm(void *user_data, printf_func func,cmd_context *cmd,opt_map all_opt){
    func(user_data,SUCCESS"%d %0.2f\r\n",hz,ratio);
}

//////////////////////////////////////////////////////////reboot命令//////////////////////////////////////////////////////

static void on_complete_reboot(void *user_data, printf_func func,cmd_context *cmd,opt_map all_opt){
    func(user_data,SUCCESS"系统将重启!\r\n");
}

//////////////////////////////////////////////////////////regist_cmd/////////////////////////////////////////////////////

static void regist_cmd(){
    {
        //获取版本号信息
        cmd_context *cmd = cmd_context_alloc("version","获取版本号",on_complete_version);
        cmd_regist(cmd);
    }

    {
        //设置mqtt登录信息
        cmd_context *cmd = cmd_context_alloc("setmqtt", "设置mqtt登录信息", on_complete_setmqtt);
        cmd_context_add_option_must(cmd, NULL, 'u', "client_id", "mqtt客户端id，一般是硬件的iemi号");
        cmd_context_add_option_must(cmd, NULL, 'p', "secret", "mqtt登录密钥");
        cmd_context_add_option_default(cmd, on_option_setmqtt_server, 's', "server", "mqtt服务器地址","39.108.84.233:1883");
        cmd_regist(cmd);
    }

    {
        //获取mqtt登录信息
        cmd_context *cmd = cmd_context_alloc("getmqtt", "设置mqtt登录信息", on_complete_getmqtt);
        cmd_regist(cmd);
    }

    {
        //设置gpio
        cmd_context *cmd = cmd_context_alloc("setio", "设置gpio管脚电平", on_complete_setio);
        cmd_context_add_option_must(cmd, NULL, 'p', "point", "gpio管脚号，范围 0 ~ 16(假设有16脚)");
        cmd_context_add_option_must(cmd, NULL, 'f', "flag", "gpio口电压高低,0或1");
        cmd_regist(cmd);
    }

    {
        //获取gpio
        cmd_context *cmd = cmd_context_alloc("getio", "获取gpio管脚电平", on_complete_getio);
        cmd_context_add_option_must(cmd, NULL, 'p', "point", "gpio管脚号，范围 0 ~ 16(假设有16脚)");
        cmd_regist(cmd);
    }

    {
        //获取adc采样信息
        cmd_context *cmd = cmd_context_alloc("getadc", "获取adc采样值", on_complete_getadc);
        cmd_regist(cmd);
    }


    {
        //获取adc采样信息
        cmd_context *cmd = cmd_context_alloc("setpwm", "设置PWM信息", on_complete_setpwm);
        cmd_context_add_option_must(cmd, NULL, 'z', "hz", "pwd频率,单位赫兹");
        cmd_context_add_option_must(cmd, NULL, 'r', "ratio", "pwd占空比，取值范围0.01 ~ 1.00");
        cmd_regist(cmd);
    }


    {
        //获取adc采样信息
        cmd_context *cmd = cmd_context_alloc("getpwm", "获取PWM信息", on_complete_getpwm);
        cmd_regist(cmd);
    }

    {
        //reboot
        cmd_context *cmd = cmd_context_alloc("reboot", "重启系统", on_complete_reboot);
        cmd_regist(cmd);
    }

}

/////////////////////////////////////////////////////////main///////////////////////////////////////////////////////////


//#define CLIENT_ID "IMEI17328379636"
//#define USER_NAME "JIMIMAX"
//#define SECRET "efbf5c0a07cc413bce2013d60e2e3435"
//#define SERVER_IP "39.108.84.233"
//#define SERVER_PORT 1883

#define CLIENT_ID "IMEI866855039412986"
#define USER_NAME "JIMIMAX"
#define SECRET "2d8c4c567a5df8de559d025bc338d05c"
#define SERVER_IP "39.108.84.233"
#define SERVER_PORT 1883

typedef struct {
    //iot_cxt对象
    void *_ctx;
    //套接字描述符
    int _fd;
} iot_user_data;

/**
 * 发送数据至网络
 * @param arg 用户数据指针，为iot_user_data指针
 * @param iov 数据块数组指针
 * @param iovcnt 数据块个数
 * @return -1为失败，>=0 为成功
 */
static int send_data_to_sock(void *arg, const struct iovec *iov, int iovcnt){
    iot_user_data *user_data = (iot_user_data *)arg;
#ifdef __alios__
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
#else
    int ret = writev(user_data->_fd,iov,iovcnt);
#endif
    if(ret == 0){
        LOGW("send failed:%d %s",errno,strerror(errno));
    }
    return ret;
}


//////////////////////////////////////////////////////////////////////
static int flag = 1;
static double db = 0;
/**
 * 定时器触发回调
 * @param user_data 用户数据指针
 */
void on_timer_tick(iot_user_data *user_data){
    iot_timer_schedule(user_data->_ctx);

    flag = !flag;
    db += 0.01 ;
    if(db == 100){
        db = 0;
    }
    char time_str[26];
    get_now_time_str(time_str,sizeof(time_str));

    if(flag){
         iot_send_bool_pkt(user_data->_ctx,210038,1);
//         iot_send_enum_pkt(user_data->_ctx,410498,"1");
//         iot_send_string_pkt(user_data->_ctx,410499,time_str);
//         iot_send_double_pkt(user_data->_ctx,410500,db);

//        buffer buffer;
//        buffer_init(&buffer);
//        iot_buffer_start(&buffer,1,iot_get_request_id(user_data->_ctx));
//         iot_buffer_append_bool(&buffer,410497,1);
//        iot_buffer_append_enum(&buffer,410498,"e1");
//        iot_buffer_append_string(&buffer,410499,time_str);
//        iot_buffer_append_double(&buffer,410500,db);
//        iot_send_buffer(user_data->_ctx,&buffer);
//        buffer_release(&buffer);
    }else{
        buffer buffer;
        buffer_init(&buffer);
        iot_buffer_start(&buffer,1,iot_get_request_id(user_data->_ctx));
        iot_buffer_append_bool(&buffer,210038,0);
//        iot_buffer_append_enum(&buffer,410498,"2");
//        iot_buffer_append_string(&buffer,410499,time_str);
//        iot_buffer_append_double(&buffer,410500,db);
        iot_send_buffer(user_data->_ctx,&buffer);
        buffer_release(&buffer);
    }
}

/**
 * 登录iot服务器成功后回调
 * @param arg 用户数据指针
 * @param ret_code 错误代码，0为成功
 */
void iot_on_connect(void *arg, char ret_code){
    LOGD("ret_code :%d",ret_code);
}

/**
 * 收到服务器下发的端点数据回调
 * @param arg 用户数据指针,即本结构体的_user_data参数
 * @param req_flag 数据类型，最后一位为0则代表回复，为1代表请求
 * @param req_id 本次请求id
 * @param data 端点数据，只读
 */
void iot_on_message(void *arg,int req_flag, uint32_t req_id, iot_data *data){
    switch (data->_type){
        case iot_bool:
            LOGD("req_flag:%d , req_id:%d , tag_id:%d , type:%d , bool:%d",req_flag,req_id,data->_tag_id,data->_type,data->_data._bool);
            break;
        case iot_string:
            LOGD("req_flag:%d , req_id:%d , tag_id:%d , type:%d , string:%s",req_flag,req_id,data->_tag_id,data->_type,data->_data._string._data);
            break;
        case iot_enum:
            LOGD("req_flag:%d , req_id:%d , tag_id:%d , type:%d , enum:%s",req_flag,req_id,data->_tag_id,data->_type,data->_data._enum._data);
            break;
        case iot_double:
            LOGD("req_flag:%d , req_id:%d , tag_id:%d , type:%d , double:%f",req_flag,req_id,data->_tag_id,data->_type,data->_data._double);
            break;
    }
}

int s_exit_flag  = 0;
void on_stop(int sig){
    s_exit_flag = 1;
}

/**
 * 运行主函数
 */
void run_main(){
    //设置日志等级
    set_log_level(log_trace);

    //数据结构体
    iot_user_data user_data;

    //网络层连接服务器
    user_data._fd = net_connet_server(SERVER_IP,SERVER_PORT,3);
    if(user_data._fd  == -1){
        return ;
    }
    //回调函数列表
    iot_callback callback = {send_data_to_sock,iot_on_connect,iot_on_message,&user_data};

    //创建iot对象
    user_data._ctx = iot_context_alloc(&callback);
    //开始登陆iot服务器
    iot_send_connect_pkt(user_data._ctx,CLIENT_ID,SECRET,USER_NAME);

    //设置socket接收超时时间，两秒超时一次，目的是产生一个定时器
    net_set_sock_timeout(user_data._fd ,1,2);

    //socket接收buffer
    char buffer[1024];
    int timeout = 0x7FFFFFFF;
    signal(SIGINT,on_stop);
    while (!s_exit_flag){
        //接收数据
        int recv = read(user_data._fd,buffer, sizeof(buffer));
        if(recv == 0){
            //服务器断开连接
            LOGE("read eof\r\n");
            break;
        }
        if(recv == -1 ){
            //接收超时，触发定时器
            on_timer_tick(&user_data);
            if(--timeout == 0){
                //程序到了需要退出的时刻
                break;
            }
            continue;
        }
        //收到数据，输入到iot对象
        iot_input_data(user_data._ctx,buffer,recv);
    }
    //是否iot对象
    iot_context_free(user_data._ctx);
}

#ifdef __alios__
int linkkit_main(void *paras){
    int ret;
    int argc = 0;
    char **argv = NULL;

    if (paras != NULL) {
        app_main_paras_t *p = (app_main_paras_t *)paras;
        argc = p->argc;
        argv = p->argv;
    }
    run_main();
}
#else
int main(int argc,char *argv[]){
    run_main();
    return 0;
}

#endif



