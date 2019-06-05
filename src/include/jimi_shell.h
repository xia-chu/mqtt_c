//
// Created by xzl on 2019-06-04.
//

#ifndef MQTT_JIMI_SHELL_H
#define MQTT_JIMI_SHELL_H


#include <stdarg.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef void(* on_argv)(void *user_data,int argc,char *argv[]);

/**
 * 命令行处理对象
 */
typedef struct cmd_splitter cmd_splitter;

/**
 * 创建shell命令行处理对象
 * @return 对象指针
 */
cmd_splitter* cmd_splitter_alloc(on_argv callback,void *user_data);


/**
 * 释放命令行处理对象
 * @param ctx 对象指针
 * @return 0：成功
 */
int cmd_splitter_free(cmd_splitter *ctx);

/**
 * 输入字符串到对象里面来split命令行，支持输入"\ "代表空格
 * @param ctx shell字符串处理对象
 * @param data 零散的分断的字符串，以回车符号作为行分隔符
 * @param len 字符串长度
 * @return 0：成功
 */
int cmd_splitter_input(cmd_splitter *ctx,const char *data,int len);

/**
 * 测试命令行工具是否正常
 */
void test_cmd_splitter();

////////////////////////////////////////////////////////////////////
//参数后面是否跟值，比如说help参数后面就不跟值
typedef enum arg_type {
    arg_none = 0,    //no_argument,
    arg_required = 1,//required_argument,
    arg_optional = 2,//required_optional,
} arg_type;


////////////////////////////////////////////////////////////////////
/**
 * 参数值map列表
 */
typedef void *opt_map ;
/**
 * 通过长参数名获取值
 * @param map 参数map
 * @param long_opt 长参数名
 * @return 参数值
 */
const char *opt_map_get(opt_map map,const char *long_opt);

/**
 * 获取参数个数
 * @param map 参数map
 * @return 参数值
 */
int opt_map_get_size(opt_map map);

/**
 * 获取索引获取值
 * @param map 参数map
 * @param index 索引
 * @return 参数值
 */
const char *opt_map_value_of_index(opt_map map,int index);

/**
 * 获取索引获取参数长名
 * @param map 参数map
 * @param index 索引
 * @return 参数长名
 */
const char *opt_map_key_of_index(opt_map map,int index);

////////////////////////////////////////////////////////////////////

/**
 * printf函数类型声明
 * @param user_data 回调用户指针
 * @param fmt 参数类型列表例如 "%d %s"
 * @param ... 参数列表
 */
typedef void(*printf_func)(void *user_data,const char *fmt,...);


/**
 * 命令对象
 */
typedef struct cmd_context cmd_context;

/**
 * 解析命令结束
 * @param cmd
 * @param all_opt
 * @return
 */
typedef void(*on_cmd_complete)(void *user_data, printf_func func, cmd_context *cmd,opt_map all_opt);


/**
 * 创建命令
 * @param cmd_name 命令名，例如 netstate
 * @param description 命令功能描述
 * @return 命令对象
 */
cmd_context *cmd_context_alloc(const char *cmd_name,const char *description,on_cmd_complete cb);

/**
 * 释放命令
 * @param ctx 命令对象
 * @return 0:成功，-1:失败
 */
int cmd_context_free(cmd_context *ctx);

/**
 * 获取命令的名称，比如说netstate
 * @param ctx 命令对象
 * @return 命令的名称，请勿free
 */
const char *cmd_context_get_name(cmd_context *ctx);
////////////////////////////////////////////////////////////////////


typedef enum option_value_ret{
    ret_continue = 0,//继续解析下一个参数
    ret_interrupt,//中断解析下一个参数
}option_value_ret;

/**
 * 解析命令参数值时，可以触发这个回调，可以做一些参数合法性检查，比如说输入ip地址加端口号时，检查有没有冒号分隔
 * @param user_data printf_func第一个参数，用户指针
 * @param func 打印函数
 * @param cmd 命令对象
 * @param opt_long_name 参数长名
 * @param opt_val 参数值字符串，可能为NULL
 */
typedef option_value_ret (*on_option_value)(void *user_data,printf_func func,cmd_context *cmd,const char *opt_long_name,const char *opt_val);


/**
 * 命令添加参数选项
 * 范例：
 *      cmd_context_add_option(ctx,'h','help',"打印此帮助信息",0,arg_none，NULL)
 *      cmd_context_add_option(ctx,'p','port',"设置端口",1,arg_required,"80")
 * @param ctx 命令对象
 * @param cb 解析到该参数的回调函数
 * @param short_opt 参数短名，例如 -h,如果没有短参数名，可以设置为0
 * @param long_opt 参数长名，例如 --help
 * @param description 参数功能描述,例如 "打印此帮助信息"
 * @param opt_must 该参数是否必选在命令中存在，0:非必须，1:必须
 * @param arg_type 参数后面是否跟值，比如说help参数后面就不跟值
 * @param default_val 默认参数
 * @return 0:成功，-1:失败
 */
int cmd_context_add_option(cmd_context *ctx,
                           on_option_value cb,
                           char short_opt,
                           const char *long_opt,
                           const char *description,
                           int opt_must,
                           arg_type arg_type,
                           const char *default_val);

/**
 * 解析命令行
 * @param ctx 命令行对象
 * @param user_data printf_func的第一个参数
 * @param func printf打印函数
 * @param argc 参数个数
 * @param argv 参数列表
 * @return 0:成功，-1:失败
 */
int cmd_context_execute(cmd_context *ctx,void *user_data,printf_func func,int argc,char *argv[]);

////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif //MQTT_JIMI_SHELL_H
