//
// Created by xzl on 2019-06-04.
//

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <getopt.h>
#include "jimi_shell.h"
#include "jimi_buffer.h"
#include "mqtt_wrapper.h"
#include "avl-tree.h"

//最大支持32个参数
#define MAX_ARGV 32

typedef struct cmd_splitter{
    buffer *_buf;
    on_shell_argv _callback;
    void *_user_data;
} cmd_splitter;


cmd_splitter* cmd_splitter_alloc(on_shell_argv callback,void *user_data){
    cmd_splitter *ret = (cmd_splitter *)malloc(sizeof(cmd_splitter));
    CHECK_PTR(ret,NULL);
    ret->_buf = buffer_alloc();
    if(!ret->_buf){
        free(ret);
        LOGE("buffer_alloc failed!");
        return NULL;
    }
    ret->_callback = callback;
    ret->_user_data = user_data;
    return ret;
}

int cmd_splitter_free(cmd_splitter *ctx){
    CHECK_PTR(ctx,-1);
    buffer_free(ctx->_buf);
    free(ctx);
    return 0;
}

static inline int is_blank(char ch){
    switch (ch){
        case '\r':
        case '\t':
        case ' ':
            return 1;
        default:
            return 0;
    }
}
static inline void cmd_splitter_cmd_line(cmd_splitter *ctx,char *start,int len){
    start[len] = '\0';
    char *argv[MAX_ARGV];
    memset(argv,0,sizeof(argv));
    int argc = 0;
    char last = ' ';
    char last_last = ' ';

    int i = 0;
    for(i = 0 ; i < len && argc < MAX_ARGV; ++i){
        if(is_blank(start[i]) && last != '\\'){
            //上个字节不是"\"且本字节是空格或制表符，那么认定该字节是分隔符号
            last_last = last;
            last = start[i];
            start[i] = '\0';
        } else{
            //不是空格
            if(is_blank(last) && last_last != '\\'){
                //上个字节是空格并且上上个字节不是"\"，那么说明是新起参数
                argv[argc++] = start + i;
            }
            last_last = last;
            last = start[i];
        }
    }

    //替换"\ "为空格
    for(i = 0 ; i < argc ; ++i ){
        while(1){
            char *pos = strstr(argv[i],"\\");
            if(!pos){
                break;
            }
            if(pos[1] == ' ' || pos[1] == '\t'){
                memmove(pos , pos + 1,strlen(pos + 1));
            }else{
                break;
            }
        }
    }
    if(ctx->_callback){
        ctx->_callback(ctx->_user_data,argc,argv);
    }
}

int cmd_splitter_input(cmd_splitter *ctx,const char *data,int len){
    CHECK_PTR(ctx,-1);
    CHECK_PTR(data,-1);
    if(len <= 0){
        len = strlen(data);
    }
    CHECK_RET(-1,buffer_append(ctx->_buf,data,len));
    char *start = ctx->_buf->_data;
    char *pos = NULL;
    while (1){
        pos = strstr(start,"\n");
        if(!pos){
            //等待更多数据
            break;
        }
        cmd_splitter_cmd_line(ctx,start,pos - start);
        start = pos + 1;
    }

    if(start != ctx->_buf->_data){
        ctx->_buf->_len -=  (start - ctx->_buf->_data);
        memmove(ctx->_buf->_data,start,ctx->_buf->_len + 1);
    }
    return 0;
}

static void argv_test(void *user_data,int argc,char *argv[]){
    LOGI("cmd:");
    int i;
    for(i = 0 ; i < argc ; ++i ){
        LOGD("%s",argv[i]);
    }
}

void test_cmd_splitter(){
    const char http_str[] = "setmqtt --id  \t iemi1234567890 \t\t  --secret abcdefghijk  \t \t --server iot.\\ \\ jimax.com:1900\r\n"
                            "setmqtt -u \tiemi1234567890 -p   \tabcdefghijk -s \t\t  iot.\\ jimax.com:1900\r\n"
                            "getmqtt\r\n"
                            "setio --piont 1 --flag  0\r\n"
                            "setio -p 1 -f  0\r\n"
                            "getio --piont 1\r\n"
                            "getio -p 1\r\n"
                            "getadc\r\n"
                            "setpwm --hz 3000 --ratio 0.40\r\n"
                            "setpwm -z 3000 -r 0.40\r\n"
                            "getpwm\r\n";

    cmd_splitter *ctx = cmd_splitter_alloc(argv_test,NULL);
    cmd_splitter_input(ctx, "\r\n",2);
    cmd_splitter_input(ctx, "\r\n\r\n",4);
    cmd_splitter_input(ctx, "\r",1);
    cmd_splitter_input(ctx, "\n\r\n",3);


    int totalTestCount = 10;
    while (--totalTestCount) {
        int totalSize = sizeof(http_str) - 1;
        int slice = totalSize / totalTestCount;
        const char *ptr = http_str;
        while (ptr + slice < http_str + totalSize) {
            buffer slice_buf;
            buffer_init(&slice_buf);
            buffer_assign(&slice_buf, ptr, slice);
            cmd_splitter_input(ctx, slice_buf._data, slice_buf._len);
            buffer_release(&slice_buf);
            ptr += slice;
        }
        cmd_splitter_input(ctx, ptr, http_str + totalSize - ptr + 1);
    }

    cmd_splitter_free(ctx);
}
////////////////////////////////////////////////////////////////////////
//参数值对象
typedef struct opt_value{
    value_type _type;
    union {
        int _int;
        long _long;
        double _double;
        char *_string;
    } _val;
    char *_val_str;
} opt_value;

opt_value *opt_value_alloc(value_type type,va_list ap){
    opt_value *ret = (opt_value *)malloc(sizeof(opt_value));
    CHECK_PTR(ret,NULL);
    memset(ret,0, sizeof(opt_value));
    ret->_type = type;
    switch (type){
        case val_int: ret->_val._int = va_arg(ap,int); break;
        case val_long: ret->_val._long = va_arg(ap,long); break;
        case val_double: ret->_val._double = va_arg(ap, double); break;
        case val_string: ret->_val._string = strdup(va_arg(ap,char *)); break;
        default: break;
    }
    return ret;
}

opt_value *opt_value_alloc_args(value_type type,...){
    va_list ap;
    va_start(ap,type);
    opt_value *ctx = opt_value_alloc(type,ap);
    va_end(ap);
    return ctx;
}
 int opt_value_free(opt_value *ctx){
    CHECK_PTR(ctx,-1);
    if(ctx->_type == val_string){
        free(ctx->_val._string);
    }
    if(ctx->_val_str){
        free(ctx->_val_str);
    }
    free(ctx);
    return 0;
}

value_type opt_value_type(opt_value *ctx){
    CHECK_PTR(ctx,val_null);
    return ctx->_type;
}

const char *opt_value_to_string(opt_value *ctx){
    if(!ctx){
        return "null";
    }

    if(ctx->_type == val_string){
        return ctx->_val._string;
    }
    if(ctx->_type == val_null){
        return "null";
    }

    if(ctx->_val_str){
        return ctx->_val_str;
    }

    char *str = malloc(32);
    switch (ctx->_type){
        case val_int:
            sprintf(str,"%d",ctx->_val._int);
            break;
        case val_long:
            sprintf(str,"%lu",ctx->_val._long);
            break;
        case val_double:
            sprintf(str,"%f",ctx->_val._double);
            break;
        default:
            strcpy(str,"null");
            break;
    }
    ctx->_val_str = str;
    return ctx->_val_str;
}


//参数对象，加入是 --help参数
typedef struct option_context{
    on_option_value _cb;
    char _short_opt; //参数短描述，比如 -h
    char *_long_opt; //参数长描述，比如 --help
    char *_description; //参数说明，比如 "打印此帮助信息"
    int _opt_must;  //该参数是否必选在命令中存在，0:非必须，1:必须
    arg_type _arg_type; //参数后面是否跟值，比如说help参数后面就不跟值
    value_type _val_type;//参数值类型
    opt_value *_default_val;//默认参数
}option_context;

typedef struct cmd_context{
    char *_name;
    char *_description;//命令描述
    AVLTree *_options;//参数列表
    on_cmd_complete _cb;//参数解析完毕的回调
}cmd_context;


option_context *option_context_alloc(on_option_value cb,
                                     char short_opt,
                                     const char *long_opt,
                                     const char *description,
                                     int opt_must,
                                     arg_type arg_type,
                                     value_type val_type,
                                     int have_default_val,
                                     va_list ap){
    option_context *ret = (option_context *)malloc(sizeof(option_context));
    CHECK_PTR(ret,NULL);
    ret->_cb = cb;
    ret->_short_opt = short_opt;
    ret->_long_opt = strdup(long_opt);
    ret->_description = strdup(description);
    ret->_opt_must = opt_must;
    ret->_arg_type = arg_type;
    ret->_val_type = val_type;
    if(have_default_val && val_type != val_null && arg_type != arg_none){
        ret->_default_val = opt_value_alloc(val_type,ap);
    }else{
        ret->_default_val = NULL;
    }
    return ret;
}


static option_context s_help_option;
static int s_help_option_inited = 0;
static const char *argsType[] = {"无参","有参","选参"};
static const char *mustExist[] = {"选填","必填"};
static const char defaultPrefix[] = "默认:";

static option_value_ret s_on_help(void *user_data,printf_func func,cmd_context *cmd,const char *opt_long_name,const char *opt_val){
    int i ;
    int maxLen_longOpt = 0;
    int maxLen_default = 0;

    for (i = 0 ; i < avl_tree_num_entries(cmd->_options) ; ++i){
        AVLTreeNode *node = avl_tree_get_node_by_index(cmd->_options,i);
        option_context *opt = avl_tree_node_value(node);

        int long_opt_name_len = strlen(opt->_long_opt);
        if(long_opt_name_len > maxLen_longOpt){
            maxLen_longOpt = long_opt_name_len;
        }

        int default_value_len = strlen(opt_value_to_string(opt->_default_val));
        if(default_value_len > maxLen_default){
            maxLen_default = default_value_len;
        }
    }

    for (i = 0 ; i < avl_tree_num_entries(cmd->_options) ; ++i){
        AVLTreeNode *node = avl_tree_get_node_by_index(cmd->_options,i);
        option_context *opt = avl_tree_node_value(node);

        //打印短参和长参名
        if(opt->_short_opt){
            func(user_data,"  -%c  --%s",opt->_short_opt,opt->_long_opt);
        }else{
            func(user_data,"      --%s",opt->_long_opt);
        }

        int j ;
        for(j = 0 ; j < maxLen_longOpt - strlen(opt->_long_opt) ; ++j){
            func(user_data," ");
        }

        //打印是否有参
        func(user_data,"  %s",argsType[opt->_arg_type]);
        //打印默认参数
        const char *defaultValue = opt_value_to_string(opt->_default_val);
        func(user_data,"  %s%s",defaultPrefix,defaultValue);

        for(j = 0 ;j < maxLen_default - strlen(defaultValue) ; ++j){
            func(user_data," ");
        }

        //打印是否必填参数
        if(opt->_default_val){
            func(user_data,"  %s",mustExist[0]);
        }else{
            func(user_data,"  %s",mustExist[opt->_opt_must ? 1 : 0]);
        }

        //打印描述
        func(user_data,"  %s\r\n",opt->_description);
    }
    return ret_interrupt;
}


static option_context *option_context_help(){
    if(!s_help_option_inited){
        s_help_option._cb = s_on_help;
        s_help_option._short_opt = 'h';
        s_help_option._long_opt = "help";
        s_help_option._description = "打印此帮助信息";
        s_help_option._opt_must = 0;
        s_help_option._arg_type = arg_none;
        s_help_option._val_type = val_null;
        s_help_option._default_val = NULL;
        s_help_option_inited = 1;
    }
    return &s_help_option;
}

int cmd_context_add_option_help(cmd_context *ctx){
    CHECK_PTR(ctx,-1);
    avl_tree_insert(ctx->_options,option_context_help()->_long_opt,option_context_help(),NULL,NULL);
    return 0;
}


int option_context_free(option_context *ctx){
    CHECK_PTR(ctx,-1);
    free(ctx->_long_opt);
    free(ctx->_description);
    if(ctx->_default_val){
        opt_value_free(ctx->_default_val);
    }
    free(ctx);
    return 0;
}

static int avl_tree_option_comp(AVLTreeKey key1, AVLTreeKey key2){
    return strcmp((char *)key1,(char *)key2);
}
static void avl_tree_free_value(AVLTreeValue val){
    option_context_free((option_context *)val);
}

cmd_context *cmd_context_alloc(const char *cmd_name,const char *description,on_cmd_complete cb){
    cmd_context *ret = (cmd_context *) malloc(sizeof(cmd_context));
    CHECK_PTR(ret,NULL);
    ret->_name = strdup(cmd_name);
    ret->_description = strdup(description);
    ret->_options = avl_tree_new(avl_tree_option_comp);
    ret->_cb = cb;
    cmd_context_add_option_help(ret);
    return ret;
}


int cmd_context_free(cmd_context *ctx){
    CHECK_PTR(ctx,-1);
    avl_tree_free(ctx->_options);
    free(ctx->_description);
    free(ctx->_name);
    free(ctx);
    return 0;
}

const char *cmd_context_get_name(cmd_context *ctx) {
    CHECK_PTR(ctx, NULL);
    return ctx->_name;
}


int cmd_context_add_option(cmd_context *ctx,
                           on_option_value cb,
                           char short_opt,
                           const char *long_opt,
                           const char *description,
                           int opt_must,
                           arg_type arg_type,
                           value_type val_type,
                           int have_default_val,
                           ...){
    CHECK_PTR(ctx,-1);
    va_list ap;
    va_start(ap,have_default_val);
    option_context *opt = option_context_alloc(cb,short_opt,long_opt,description,opt_must,arg_type,val_type,have_default_val,ap);
    avl_tree_insert(ctx->_options,opt->_long_opt,opt,NULL,avl_tree_free_value);
    va_end(ap);
    return 0;
}

int cmd_context_add_option_must(cmd_context *ctx,
                                on_option_value cb,
                                char short_opt,
                                const char *long_opt,
                                const char *description,
                                value_type val_type){
    return cmd_context_add_option(ctx,cb,short_opt,long_opt,description,1,arg_required,val_type,0);
}

#define LONG_OPT_OFFSET 0xFF
#define OPT_INDEX_NOT_FOUND -1

static int short_opt_comp(AVLTreeKey key1, AVLTreeKey key2){
    return key1 - key2;
}

static void avl_tree_free_opt_value(AVLTreeValue value){
    if(value){
        opt_value_free((opt_value *)value);
    }
}
int cmd_context_execute(cmd_context *ctx,void *user_data,printf_func func,int argc,char *argv[]){
    CHECK_PTR(ctx,-1);
    if(argc < 1){
        func(user_data,"参数不够\r\n");
        return -1;
    }
    if(strcmp(ctx->_name,argv[0]) != 0){
        func(user_data,"命令名不匹配:%s\r\n", argv[0]);
        return -1;
    }
    AVLTree  *opt_val_map = avl_tree_new(avl_tree_option_comp);
    AVLTree  *opt_short_to_long = avl_tree_new(short_opt_comp);

    int option_size = avl_tree_num_entries(ctx->_options);
    buffer *short_opt_str = buffer_alloc();
    struct option *long_opt_array = (struct option *) malloc(sizeof(struct option) * (option_size + 1));
    int i;

    for(i = 0 ; i < option_size ; ++i ){
        AVLTreeNode *opt_node = avl_tree_get_node_by_index(ctx->_options,i);
        option_context *opt = avl_tree_node_value(opt_node);

        if(opt->_default_val){
            //默认参数
            avl_tree_insert(opt_val_map,opt->_long_opt,opt->_default_val,NULL,NULL);
        }
        //添加长参数
        struct option *long_opt = long_opt_array + i;
        long_opt->name = opt->_long_opt;
        long_opt->has_arg = opt->_arg_type;
        long_opt->flag = NULL;
        long_opt->val = i + LONG_OPT_OFFSET;

        if(!opt->_short_opt){
            //没有短参数
            continue;
        }
        avl_tree_insert(opt_short_to_long,(AVLTreeKey)opt->_short_opt,(AVLTreeValue)long_opt->val,NULL,NULL);
        //添加短参数
        buffer_append(short_opt_str,&opt->_short_opt,1);
        switch (opt->_arg_type){
            case arg_required:
                buffer_append(short_opt_str,":",1);
                break;
//            case arg_optional:
//                buffer_append(short_opt_str,"::",2);
                break;
            default:
                break;
        }
    }

    //长参数结尾符
    long_opt_array[i].flag = 0;
    long_opt_array[i].name = 0;
    long_opt_array[i].has_arg = 0;
    long_opt_array[i].val = 0;

    int index;
    optind = 0;
    opterr = 0;

    while ((index = getopt_long(argc, argv, short_opt_str->_data,long_opt_array ,NULL)) != -1) {
        if(index < LONG_OPT_OFFSET){
            //短参数,我们转换成长参数
           AVLTreeNode *node = avl_tree_lookup_node(opt_short_to_long,(AVLTreeKey)index);
           if(node == NULL){
               func(user_data,"\t未识别的选项\"%c\",输入\"-h\"获取帮助.\r\n",(char)index);
               goto completed;
           }
            //转换成长参数
           index = (int)avl_tree_node_value(node);
        }

        AVLTreeNode *opt_node = avl_tree_get_node_by_index(ctx->_options,index - LONG_OPT_OFFSET);
        if(!opt_node){
            func(user_data,"\t未识别的选项\"%d\",输入\"-h\"获取帮助.\r\n",index);
            goto completed;
        }

        option_context *opt = (option_context*) avl_tree_node_value(opt_node);
        if(opt->_cb && ret_interrupt == opt->_cb(user_data,func,ctx,opt->_long_opt,optarg)){
            goto completed;
        }
        opt_value *value = NULL;
        switch (opt->_val_type){
            case val_int:
                value = opt_value_alloc_args(opt->_val_type,atoi(optarg));
                break;
            case val_long:
                value = opt_value_alloc_args(opt->_val_type,atol(optarg));
                break;
            case val_double:
                value = opt_value_alloc_args(opt->_val_type,atof(optarg));
                break;
            case val_string:
                value = opt_value_alloc_args(opt->_val_type,optarg ? optarg : "");
                break;
            default:
                break;
        }

        //先移除默认参数
        avl_tree_remove(opt_val_map,opt->_long_opt);
        avl_tree_insert(opt_val_map,
                        opt->_long_opt,
                        value ? value :  opt_value_alloc_args(val_null,NULL),
                        NULL,
                        avl_tree_free_opt_value);
        optarg = NULL;
    }


    for(i = 0 ; i < option_size ; ++i ){
        AVLTreeNode *opt_node = avl_tree_get_node_by_index(ctx->_options,i);
        option_context *opt = avl_tree_node_value(opt_node);
        if(!opt->_opt_must){
            //非必选参数
            continue ;
        }
        //必选参数查看有未提供
        AVLTreeNode *node = avl_tree_lookup_node(opt_val_map,opt->_long_opt);
        if(node == NULL){
            func(user_data,"\t参数\"%s\"必选提供!\r\n",opt->_long_opt);
            goto completed;
        }
    }

    if(ctx->_cb){
        ctx->_cb(user_data,func,ctx,opt_val_map);
    }

completed:

    free(short_opt_str);
    free(long_opt_array);
    avl_tree_free(opt_short_to_long);
    avl_tree_free(opt_val_map);
    return 0;
}


int opt_value_map_get_size(opt_value_map map){
    return avl_tree_num_entries(map);
}

opt_value *opt_value_map_get(opt_value_map map,const char *key){
    return avl_tree_lookup(map,(AVLTreeKey)key);
}

opt_value *opt_value_map_value_of_index(opt_value_map map,int index){
    AVLTreeNode *node = avl_tree_get_node_by_index(map,index);
    if(!node){
        return NULL;
    }
    return (opt_value*)avl_tree_node_value(node);
}


const char *opt_value_map_name_of_index(opt_value_map map,int index){
    AVLTreeNode *node = avl_tree_get_node_by_index(map,index);
    if(!node){
        return NULL;
    }
    return (const char*)avl_tree_node_key(node);
}




















