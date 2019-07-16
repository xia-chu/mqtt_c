package com.jimi.mqtt;

public class MQTT {
    static public final int iot_bool = 0x01;//布尔型，占用一个字节
    static public final int iot_enum = 0x02;//本质同于string
    static public final int iot_string = 0x03;//字符串类型，可变长度
    static public final int iot_double = 0x05;//双精度浮点型，8个字节
    public interface IotCallBack{
        /**
         * 对象输出协议数据，请通过网络发送给服务器
         * @param data 数据块
         */
        void iot_on_output(byte[] data);

        /**
         * 连接iot服务器成功后回调
         * @param ret_code 失败时为错误代码，0代表成功
         */
        void iot_on_connect(int ret_code);

        /**
         * 收到服务器下发的端点数据回调
         * @param req_flag 数据类型，最后一位为0则代表回复，为1代表请求
         * @param req_id 本次请求id
         * @param data_type 数据类型，可能为iot_bool、iot_enum、iot_string、iot_double
         * @param data 端点数据，可能为Boolean、String,String,Double类型，根据data_type来区分
         */
        void iot_on_message(int req_flag, int req_id, int tag_id , int data_type ,Object data);
    }

    static public class IotContext extends Object{
        /**
         * 构建mqtt对象
         * @param cb 事件处理对象
         */
        public IotContext(IotCallBack cb){
            _ctx = iot_context_alloc(cb);
        }

        @Override
        protected void finalize() throws Throwable {
            super.finalize();
            release();
        }

        /**
         * 立即释放对象
         */
        public void release(){
            if(_ctx != 0){
                iot_context_free(_ctx);
                _ctx = 0;
            }
        }

        /**
         * 发送连接、登录包
         * @param client_id 客户端id
         * @param secret 授权码
         * @param user_name 用户名，一般为JIMIMAX
         * @return 0为成功，其他为错误代码
         */
        public int send_connect(String client_id,String secret,String user_name){
            return iot_send_connect_pkt(_ctx,client_id,secret,user_name);
        }

        /**
         * 发送布尔型的端点数据
         * @param tag_id 端点id
         * @param flag 端点值，0或1
         * @return 0为成功，其他为错误代码
         */
        public int send_bool(int tag_id,boolean flag){
            return iot_send_bool_pkt(_ctx,tag_id,flag);
        }

        /**
         * 发送双精度浮点型的端点数据
         * @param tag_id 端点id
         * @param db 端点值
         * @return 0为成功，其他为错误代码
         */
        public int send_double(int tag_id,double db){
            return iot_send_double_pkt(_ctx,tag_id,db);
        }

        /**
         * 发送枚举型的端点数据
         * @param tag_id 端点id
         * @param enum_str 枚举字符串
         * @return 0为成功，其他为错误代码
         */
        public int send_enum(int tag_id,String enum_str){
            return iot_send_enum_pkt(_ctx,tag_id,enum_str);
        }

        /**
         * 发送字符串型的端点数据
         * @param tag_id 端点id
         * @param str 字符串
         * @return 0为成功，其他为错误代码
         */
        public int send_string(int tag_id,String str){
            return iot_send_string_pkt(_ctx,tag_id,str);
        }


        /**
         * 网络层收到数据后请调用此函数输入给本对象处理
         * @param data 数据指针
         * @param offset 数据偏移量
         * @param len 数据长度
         * @return 0代表成功，其他为错误代码
         */
        public int input_data(byte[] data,int offset, int len){
            return iot_input_data(_ctx,data,offset,len);
        }

        /**
         * 网络层收到数据后请调用此函数输入给本对象处理
         * @param data 数据指针
         * @return 0代表成功，其他为错误代码
         */
        public int input_data(byte[] data){
            return iot_input_data(_ctx,data,0,data.length);
        }

        /**
         * 请每隔一段时间触发此函数，建议1~3秒
         * 本对象内部需要定时器管理状态
         * @return 0为成功，-1为失败
         */
        public int timer_schedule(){
            return iot_timer_schedule(_ctx);
        }
        private long _ctx = 0;
    }
    /**
     * 创建iot客户端对象
     * @param cb 回调结构体参数
     * @return  对象指针
     */
    static private native long iot_context_alloc(IotCallBack cb);


    /**
     * 释放iot客户端对象
     * @param arg 对象指针,iot_context_alloc创建的返回值
     * @return 0代表成功，-1为失败
     */
    static private native int iot_context_free(long arg);


    /**
     * 发送连接、登录包
     * @see iot_context_alloc
     * @param iot_ctx 对象指针
     * @param client_id 客户端id
     * @param secret 授权码
     * @param user_name 用户名，一般为JIMIMAX
     * @return 0为成功，其他为错误代码
     */
    static private native int iot_send_connect_pkt(long iot_ctx,String client_id,String secret,String user_name);

    /**
     * 发送布尔型的端点数据
     * @param iot_ctx 对象指针
     * @param tag_id 端点id
     * @param flag 端点值，0或1
     * @return 0为成功，其他为错误代码
     */
    static private native int iot_send_bool_pkt(long iot_ctx,int tag_id,boolean flag);

    /**
     * 发送双精度浮点型的端点数据
     * @param iot_ctx 对象指针
     * @param tag_id 端点id
     * @param db 端点值
     * @return 0为成功，其他为错误代码
     */
    static private native int iot_send_double_pkt(long iot_ctx,int tag_id,double db);

    /**
     * 发送枚举型的端点数据
     * @param iot_ctx 对象指针
     * @param tag_id 端点id
     * @param enum_str 枚举字符串，以'\0'结尾
     * @return 0为成功，其他为错误代码
     */
    static private native int iot_send_enum_pkt(long iot_ctx,int tag_id,String enum_str);

    /**
     * 发送字符串型的端点数据
     * @param iot_ctx 对象指针
     * @param tag_id 端点id
     * @param str 字符串，以'\0'结尾
     * @return 0为成功，其他为错误代码
     */
    static private native int iot_send_string_pkt(long iot_ctx,int tag_id,String str);

    /**
     * 网络层收到数据后请调用此函数输入给本对象处理
     * @param iot_ctx 对象指针
     * @param data 数据指针
     * @param offset 数据偏移量
     * @param len 数据长度
     * @return 0代表成功，其他为错误代码
     */
    static private native int iot_input_data(long iot_ctx,byte[] data,int offset, int len);

    /**
     * 请每隔一段时间触发此函数，建议1~3秒
     * 本对象内部需要定时器管理状态
     * @param iot_ctx 对象指针
     * @return 0为成功，-1为失败
     */
    static private native int iot_timer_schedule(long iot_ctx);
}
