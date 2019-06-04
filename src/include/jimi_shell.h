//
// Created by xzl on 2019-06-04.
//

#ifndef MQTT_JIMI_SHELL_H
#define MQTT_JIMI_SHELL_H


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef void(* on_shell_argv)(void *user_data,int argc,char *argv[]);

/**
 * 命令行处理对象
 */
typedef struct shell_context shell_context;

/**
 * 创建shell命令行处理对象
 * @return 对象指针
 */
shell_context* shell_context_alloc(on_shell_argv callback,void *user_data);


/**
 * 释放命令行处理对象
 * @param ctx 对象指针
 * @return 0：成功
 */
int shell_context_free(shell_context *ctx);

/**
 * 输入字符串到对象里面来split命令行，支持输入"\ "代表空格
 * @param ctx shell字符串处理对象
 * @param data 零散的分断的字符串，以回车符号作为行分隔符
 * @param len 字符串长度
 * @return 0：成功
 */
int shell_context_input(shell_context *ctx,const char *data,int len);

/**
 * 测试命令行工具是否正常
 */
void test_shell_context();
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif //MQTT_JIMI_SHELL_H
