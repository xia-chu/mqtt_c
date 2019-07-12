#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include "jimi_shell.h"
#include "jimi_log.h"

//////////////////////////////////////////////////////////自定义输出，可以替换成串口或网络输出///////////////////////////////

static void s_printf(void *user_data,const char *fmt,...){
    va_list ap;
    va_start(ap,fmt);
    vprintf(fmt,ap);
    va_end(ap);
    if(fmt[0] == '$'){
        rewind(stdout);
    }
}

extern void regist_cmd();
/////////////////////////////////////////////////////////main///////////////////////////////////////////////////////////

int main(int argc,char *argv[]){
    //注册各种命令
    regist_cmd();
    //开始shell交互
    s_printf(NULL,"$ 欢迎进入命令模式，你可以输入\"help\"命令获取帮助\r\n$ ");
    char buf[256];
    while(1){
        int i = read(STDIN_FILENO,buf, sizeof(buf));
        if(i > 0){
            if(strncmp(buf,"exit",4) == 0){
                break;
            }
            shell_input(NULL,s_printf,buf,i);
        } else {
            break;
        }
    }
    shell_destory();
    return 0;
}