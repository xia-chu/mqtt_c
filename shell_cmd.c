#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include "jimi_shell.h"
#include "jimi_log.h"

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

void regist_cmd(){
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
