#include <stddef.h>
#include <unistd.h>
#include "jimi_shell.h"
#include "jimi_log.h"

static void user_printf(void *user_data,const char *fmt,...){
    va_list ap;
    va_start(ap,fmt);
    vprintf(fmt,ap);
    va_end(ap);
}

static void on_argv(void *user_data,int argc,char *argv[]){
    cmd_context *cmd = (cmd_context *)user_data;
    cmd_context_execute(cmd,NULL,user_printf,argc,argv);
}

static void on_cmd_complete(void *user_data, printf_func func,cmd_context *cmd,opt_value_map all_value){

    func(user_data,"on_cmd_complete\r\n");
    int i = opt_value_map_get_size(all_value);
    for(; i != 0 ; --i){
        opt_value *value = opt_value_map_value_of_index(all_value,i - 1);
        const char *key = opt_value_map_name_of_index(all_value,i - 1);
        func(user_data,"%s = %s\r\n",key , opt_value_to_string(value));
    }

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
    cmd_context *cmd = cmd_context_alloc("test","测试命令",on_cmd_complete);
    cmd_context_add_option(cmd,on_get_option,'p',"port","设置端口号",1,opt_required,val_int,1,80);
    cmd_context_add_option(cmd,on_get_option,'s',"server","设置服务器地址",1,opt_required,val_string,1,"127.0.0.1:80");
    cmd_context_add_option(cmd,on_get_option,'a',"abort","测试中断参数",0,opt_none,val_null,0);
    cmd_context_add_option(cmd,on_get_option,'m',"must","测试必须提供参数",1,opt_required,val_int,0);
    cmd_context_add_option(cmd,on_get_option,0,"no_short","测试无短参数名",0,opt_required,val_int,0);


    cmd_splitter *ctx = cmd_splitter_alloc(on_argv,cmd);
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
    cmd_context_free(cmd);
    return 0;
}