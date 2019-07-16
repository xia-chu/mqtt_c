#include <jni.h>
#include <string>
#include <jimi_memory.h>
#include <jimi_iot.h>
#include "jimi_iot.h"
#include "jimi_log.h"
using namespace std;

#define JNI_API(retType,funName,...) extern "C"  JNIEXPORT retType Java_com_jimi_mqtt_MQTT_##funName(JNIEnv* env, jclass cls,##__VA_ARGS__)

string stringFromJstring(JNIEnv *env,jstring jstr){
    if(!env || !jstr){
        LOGW("invalid args");
        return "";
    }
    const char *field_char = env->GetStringUTFChars(jstr, 0);
    string ret(field_char,env->GetStringUTFLength(jstr));
    env->ReleaseStringUTFChars(jstr, field_char);
    return ret;
}
string stringFromJbytes(JNIEnv *env,jbyteArray jbytes){
    if(!env || !jbytes){
        LOGW("invalid args");
        return "";
    }
    jbyte *bytes = env->GetByteArrayElements(jbytes, 0);
    string ret((char *)bytes,env->GetArrayLength(jbytes));
    env->ReleaseByteArrayElements(jbytes,bytes,0);
    return ret;
}
string stringFieldFromJava(JNIEnv *env,  jobject jdata,jfieldID jid){
    if(!env || !jdata || !jid){
        LOGW("invalid args");
        return "";
    }
    jstring field_str = (jstring)env->GetObjectField(jdata,jid);
    auto ret = stringFromJstring(env,field_str);
    env->DeleteLocalRef(field_str);
    return ret;
}

string bytesFieldFromJava(JNIEnv *env,  jobject jdata,jfieldID jid){
    if(!env || !jdata || !jid){
        LOGW("invalid args");
        return "";
    }
    jbyteArray jbufArray = (jbyteArray)env->GetObjectField(jdata, jid);
    string ret = stringFromJbytes(env,jbufArray);
    env->DeleteLocalRef(jbufArray);
    return ret;
}

jstring jstringFromString(JNIEnv* env, const char* pat) {
    return (jstring)env->NewStringUTF(pat);
}

jbyteArray jbyteArrayFromString(JNIEnv* env, const char* pat,int len = 0){
    if(len <= 0){
        len = strlen(pat);
    }
    jbyteArray jarray = env->NewByteArray(len);
    env->SetByteArrayRegion(jarray, 0, len, (jbyte *)(pat));
    return jarray;
}

jobject to_bool(JNIEnv* env,bool flag){
    static jclass jclass_obj = (jclass)env->NewGlobalRef(env->FindClass("java/lang/Boolean"));
    static jmethodID jmethodID_init = env->GetMethodID(jclass_obj, "<init>", "(Z)V");
    jobject ret = env->NewObject(jclass_obj, jmethodID_init,(jboolean)flag);
    return ret;
}

jobject to_double(JNIEnv* env, double db){
    static jclass jclass_obj = (jclass)env->NewGlobalRef(env->FindClass("java/lang/Double"));
    static jmethodID jmethodID_init = env->GetMethodID(jclass_obj, "<init>", "()D");
    jobject ret = env->NewObject(jclass_obj, jmethodID_init,(jdouble)db);
    return ret;
}

static bool loadAllClass(JNIEnv* env){
    env->DeleteLocalRef(to_bool(env,0));
    env->DeleteLocalRef(to_double(env,0));
    return true;
}
static JavaVM *s_jvm = nullptr;

template <typename FUN>
void doInJavaThread(FUN &&fun){
    JNIEnv *env;
    int status = s_jvm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (status != JNI_OK) {
        if (s_jvm->AttachCurrentThread(&env, NULL) != JNI_OK) {
            return;
        }
    }
    fun(env);
    if (status != JNI_OK) {
        //Detach线程
        s_jvm->DetachCurrentThread();
    }
}

#define emitEvent(delegate,method,argFmt,...) \
{ \
    doInJavaThread([&](JNIEnv* env) { \
        static jclass cls = env->GetObjectClass(delegate); \
        static jmethodID jmid = env->GetMethodID(cls, method, argFmt); \
        jobject localRef = env->NewLocalRef(delegate); \
        if(localRef){ \
            env->CallVoidMethod(localRef, jmid, ##__VA_ARGS__); \
        }else{ \
            LOGW("弱引用已经释放: %s %s",method,argFmt); \
        }\
    }); \
}

/*
 * 加载动态库
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    //设置日志
    s_jvm = vm;
    return JNI_VERSION_1_6;
}
/*
 * 卸载动态库
 */
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved){
}


/**
 * 对象输出协议数据，请调用writev发送给服务器
 * @param arg 用户数据指针,即本结构体的_user_data参数
 * @param iov 数据块
 * @param iovcnt 数据块个数
 * @param 返回-1代表失败，大于0则为成功
 */
static int iot_on_output_s(void *arg, const struct iovec *iov, int iovcnt){
    int size = 0;
    for (int i = 0; i < iovcnt; ++i) {
        size += (iov + i)->iov_len;
    }
    char *buf = (char *)jimi_malloc(size);
    int pos = 0;
    for (int i = 0; i < iovcnt; ++i) {
        memcpy(buf + pos , (iov + i)->iov_base , (iov + i)->iov_len);
        pos += (iov + i)->iov_len;
    }
    emitEvent((jobject)arg,"iot_on_output","([B)V",jbyteArrayFromString(env,buf,size));
    jimi_free(buf);

}

/**
 * 连接iot服务器成功后回调
 * @param arg 用户数据指针,即本结构体的_user_data参数
 * @param ret_code 失败时为错误代码，0代表成功
 */
static void iot_on_connect_s(void *arg, char ret_code){
    emitEvent((jobject)arg,"iot_on_connect","(I)V",(jint)ret_code);
}

/**
 * 收到服务器下发的端点数据回调
 * @param arg 用户数据指针,即本结构体的_user_data参数
 * @param req_flag 数据类型，最后一位为0则代表回复，为1代表请求
 * @param req_id 本次请求id
 * @param data 端点数据，只读
 */

#define emitEvent_Mqtt(...)  \
        emitEvent( \
                (jobject) arg,\
                "iot_on_message",\
                "(IIIILjava/lang/Object;)V", \
                (jint) req_flag, \
                (jint) req_id, \
                (jint) data->_tag_id, \
                (jint) data->_type, \
                ##__VA_ARGS__);

static void iot_on_message_s(void *arg,int req_flag, uint32_t req_id, iot_data *data){
    switch (data->_type) {
        case iot_bool:   emitEvent_Mqtt(to_bool(env, data->_data._bool));
            break;
        case iot_enum:   emitEvent_Mqtt(jstringFromString(env, data->_data._enum._data));
            break;
        case iot_string: emitEvent_Mqtt(jstringFromString(env, data->_data._string._data));
            break;
        case iot_double: emitEvent_Mqtt(to_double(env, data->_data._double));
            break;
    }

}

/**
 * 创建iot客户端对象
 * @param cb 回调结构体参数
 * @return  对象指针
 */
JNI_API(jlong, iot_context_alloc,jobject cb){
    static bool s_load_flag = loadAllClass(env);
    iot_callback cb_c = {iot_on_output_s,iot_on_connect_s,iot_on_message_s,env->NewWeakGlobalRef(cb)};
    return (jlong)iot_context_alloc(&cb_c);
}


/**
 * 释放iot客户端对象
 * @param arg 对象指针,iot_context_alloc创建的返回值
 * @return 0代表成功，-1为失败
 */
JNI_API(jint, iot_context_free,jlong arg){
    return iot_context_free((void *)arg);
}


/**
 * 发送连接、登录包
 * @see iot_context_alloc
 * @param iot_ctx 对象指针
 * @param client_id 客户端id
 * @param secret 授权码
 * @param user_name 用户名，一般为JIMIMAX
 * @return 0为成功，其他为错误代码
 */
JNI_API(jint, iot_send_connect_pkt,jlong iot_ctx,jstring client_id,jstring secret,jstring user_name){
    return iot_send_connect_pkt(
            (void *) iot_ctx,
            stringFromJstring(env, client_id).data(),
            stringFromJstring(env, secret).data(),
            stringFromJstring(env, user_name).data());
}

/**
 * 发送布尔型的端点数据
 * @param iot_ctx 对象指针
 * @param tag_id 端点id
 * @param flag 端点值，0或1
 * @return 0为成功，其他为错误代码
 */
JNI_API(jint, iot_send_bool_pkt,jlong iot_ctx,jint tag_id,jboolean flag){
    return iot_send_bool_pkt((void *) iot_ctx, tag_id, flag);
}

/**
 * 发送双精度浮点型的端点数据
 * @param iot_ctx 对象指针
 * @param tag_id 端点id
 * @param double_num 端点值
 * @return 0为成功，其他为错误代码
 */
JNI_API(jint, iot_send_double_pkt,jlong iot_ctx,jint tag_id,jdouble double_num){
    return iot_send_double_pkt((void *) iot_ctx, tag_id, double_num);
}

/**
 * 发送枚举型的端点数据
 * @param iot_ctx 对象指针
 * @param tag_id 端点id
 * @param enum_str 枚举字符串，以'\0'结尾
 * @return 0为成功，其他为错误代码
 */
JNI_API(jint, iot_send_enum_pkt,jlong iot_ctx,jint tag_id,jstring enum_str){
    return iot_send_enum_pkt((void *) iot_ctx, tag_id, stringFromJstring(env,enum_str).data());
}

/**
 * 发送字符串型的端点数据
 * @param iot_ctx 对象指针
 * @param tag_id 端点id
 * @param str 字符串，以'\0'结尾
 * @return 0为成功，其他为错误代码
 */
JNI_API(jint, iot_send_string_pkt,jlong iot_ctx,jint tag_id,jstring str){
    return iot_send_string_pkt((void *) iot_ctx, tag_id, stringFromJstring(env,str).data());
}

/**
 * 网络层收到数据后请调用此函数输入给本对象处理
 * @param iot_ctx 对象指针
 * @param data 数据指针
 * @param offset 数据偏移
 * @param len 数据长度
 * @return 0代表成功，其他为错误代码
 */
JNI_API(jint, iot_input_data,jlong iot_ctx,jbyteArray data,int offset, int len){
    jbyte *bytes = env->GetByteArrayElements(data, 0);
    int ret = iot_input_data((void *)iot_ctx,(char *)bytes + offset,len);
    env->ReleaseByteArrayElements(data,bytes,0);
    return ret;
}

/**
 * 请每隔一段时间触发此函数，建议1~3秒
 * 本对象内部需要定时器管理状态
 * @param iot_ctx 对象指针
 * @return 0为成功，-1为失败
 */
JNI_API(jint, iot_timer_schedule,jlong iot_ctx){
    return iot_timer_schedule((void *)iot_ctx);
}



