#include <stddef.h>
#include <unistd.h>
#include "jimi_shell.h"
#include "jimi_log.h"

static void s_printf(void *user_data,const char *fmt,...){
    va_list ap;
    va_start(ap,fmt);
    vprintf(fmt,ap);
    va_end(ap);
}

static void s_on_argv(void *user_data,int argc,char *argv[]){
    cmd_manager *manager = (cmd_manager *)user_data;
    cmd_manager_execute(manager,NULL,s_printf,argc,argv);
}

static void s_on_complete(void *user_data, printf_func func,cmd_context *cmd,opt_map all_opt){

    func(user_data,"%s on_complete\r\n",cmd_context_get_name(cmd));
    int i = opt_map_size(all_opt);
    for(; i != 0 ; --i){
        const char *value = opt_map_value_of_index(all_opt,i - 1);
        const char *key = opt_map_key_of_index(all_opt,i - 1);
        func(user_data,"%s = %s\r\n",key ,value);
    }

    func(user_data,"user = %s\r\n",opt_map_get_value(all_opt,"user"));
}

static option_value_ret on_get_option(void *user_data,
                                      printf_func func,
                                      cmd_context *cmd,
                                      const char *opt_long_name,
                                      const char *opt_val){
    func(user_data,"on_get_option: %s %s\r\n",opt_long_name,opt_val);
    if(strcmp(opt_long_name,"abort") == 0){
        return ret_interrupt;
    }

    return ret_continue;
}

int main(int argc,char *argv[]){
    cmd_context *cmd = cmd_context_alloc("test","测试命令",s_on_complete);
    cmd_context_add_option(cmd,on_get_option,'b',"bbbb","测试bbb");
    cmd_context_add_option(cmd,on_get_option,'c',"cccc","测试bbb");
    cmd_context_add_option(cmd,on_get_option,0,"no_short","测试无短参数名");
    cmd_context_add_option_default(cmd,on_get_option,'s',"server","设置服务器地址","127.0.0.1:80");
    cmd_context_add_option_bool(cmd,on_get_option,'a',"abort","测试中断参数");
    cmd_context_add_option_bool(cmd,on_get_option,'d',"ddddd","测试ddddd");
    cmd_context_add_option_must(cmd,on_get_option,'u',"user","测试必须提供用户名");
    cmd_context_add_option_must(cmd,on_get_option,'p',"pwd","测试必须提供密码");

    cmd_manager *manager = cmd_manager_alloc();
    cmd_manager_add_cmd(manager,cmd);

    cmd_splitter *ctx = cmd_splitter_alloc(s_on_argv,manager);
    printf("$ 欢迎进入命令模式，你可以输入\"help\"命令获取帮助\r\n");
    char buf[256];
    while(1){
        printf("$ ");
        rewind(stdout);
        int i = read(STDIN_FILENO,buf, sizeof(buf));
        if(i > 0){
            cmd_splitter_input(ctx,buf,i);
        } else {
            break;
        }
    }
    cmd_splitter_free(ctx);
    cmd_manager_free(manager);
    return 0;
}