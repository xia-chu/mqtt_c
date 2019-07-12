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
const char *opt_map_get_value(opt_map map,const char *long_opt);

/**
 * 获取参数个数
 * @param map 参数map
 * @return 参数值
 */
int opt_map_size(opt_map map);

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
 * 添加命令参数
 * 该参数带值，没有默认值，可以不提供该参数
 * @param ctx 命令对象
 * @param cb 解析到该参数的回调函数
 * @param short_opt 参数短名，例如 -h,如果没有短参数名，可以设置为0
 * @param long_opt 参数长名，例如 --help
 * @param description 参数功能描述,例如 "打印此帮助信息"
 * @return 0:成功，-1:失败
 */
int cmd_context_add_option(cmd_context *ctx,
                           on_option_value cb,
                           char short_opt,
                           const char *long_opt,
                           const char *description);
/**
 * 添加命令参数
 * 该参数带值，没有默认值，必须提供该参数，
 * @param ctx 命令对象
 * @param cb 解析到该参数的回调函数
 * @param short_opt 参数短名，例如 -h,如果没有短参数名，可以设置为0
 * @param long_opt 参数长名，例如 --help
 * @param description 参数功能描述,例如 "打印此帮助信息"
 * @return 0:成功，-1:失败
 */
int cmd_context_add_option_must(cmd_context *ctx,
                                on_option_value cb,
                                char short_opt,
                                const char *long_opt,
                                const char *description);

/**
 * 添加命令参数
 * 该参数带值，有默认值，可以不提供该参数
 * @param ctx 命令对象
 * @param cb 解析到该参数的回调函数
 * @param short_opt 参数短名，例如 -h,如果没有短参数名，可以设置为0
 * @param long_opt 参数长名，例如 --help
 * @param description 参数功能描述,例如 "打印此帮助信息"
 * @param default_val 默认值
 * @return 0:成功，-1:失败
 */
int cmd_context_add_option_default(cmd_context *ctx,
                                   on_option_value cb,
                                   char short_opt,
                                   const char *long_opt,
                                   const char *description,
                                   const char *default_val);

/**
 * 添加命令参数
 * 该参数不带值，提供该参数不提供都可以，比如说 ls -l , "-l"参数就是不带值的bool类型
 * @param ctx 命令对象
 * @param cb 解析到该参数的回调函数
 * @param short_opt 参数短名，例如 -h,如果没有短参数名，可以设置为0
 * @param long_opt 参数长名，例如 --help
 * @param description 参数功能描述,例如 "打印此帮助信息"
 * @return 0:成功，-1:失败
 */
int cmd_context_add_option_bool(cmd_context *ctx,
                                on_option_value cb,
                                char short_opt,
                                const char *long_opt,
                                const char *description);
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
/**
 * 输入字符串到shell单例解析器
 * @param user_data 用户数据指针，printf_func回调第一个参数
 * @param func printf打印函数指针
 * @param buf 字符串
 * @param len 字符串长度
 * @return
 */
int shell_input(void *user_data,printf_func func,const char *buf,int len);


/**
 * 输入命令并执行
 * @param user_data 用户数据指针，printf_func回调第一个参数
 * @param func printf打印函数指针
 * @param argc 参数个数
 * @param argv 参数指针
 * @return
 */
int shell_input_args(void *user_data,printf_func func,int argc,char *argv[]);
/**
 * 销毁shell单例解析器
 */
void shell_destory();

/**
 * 注册命令
 * @param cmd 命令
 * @return
 */
int cmd_regist(cmd_context *cmd);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif //MQTT_JIMI_SHELL_H
