#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#include "mqtt.h"
#ifdef __cplusplus
}
#endif // __cplusplus

#include <sys/signal.h>
#include <unistd.h>
#include "Network/TcpClient.h"
#include "Util/logger.h"
#include "Util/CMD.h"

using namespace toolkit;

class Mqtt : public TcpClient{
public:
    typedef std::shared_ptr<Mqtt> Ptr;

    Mqtt(){
        _contex.user_data = this;
        _contex.writev_func = [](void *arg, const struct iovec *iov, int iovcnt){
            Mqtt *thiz = (Mqtt *) arg;
            string str;
            while(iovcnt--){
                auto ptr = iov++;
                str.append((char *)ptr->iov_base,ptr->iov_len);
            }
//            TraceL << "send " << hexdump(str.data(),str.size());
            return thiz->send(std::move(str));
        };
        _contex.handle_ping_resp = [](void *arg){
            Mqtt *thiz = (Mqtt *) arg;
//            DebugL << "handle_ping_resp";
            return 0;
        };

        _contex.handle_conn_ack = [](void *arg, char flags, char ret_code){
            Mqtt *thiz = (Mqtt *) arg;
            DebugL << "handle_conn_ack " << (int)flags << " " << (int)ret_code ;
            thiz->sendPublish("/Service/JIMIMAX/publish","publishPayload",MQTT_QOS_LEVEL1, true);
            thiz->sendSubscribe(MQTT_QOS_LEVEL1,{"/Service/JIMIMAX/publish","/Service/JIMIMAX/will"});
            return 0;
        };

        _contex.handle_publish = [](void *arg, uint16_t pkt_id, const char *topic,
                                    const char *payload, uint32_t payloadsize,
                                    int dup, enum MqttQosLevel qos){
            Mqtt *thiz = (Mqtt *) arg;
            DebugL << "handle_publish " << pkt_id << " " << topic << " " << payload << " " << dup << " " << " " << qos ;
            return 0;
        };

        _contex.handle_pub_ack = [](void *arg, uint16_t pkt_id){
            Mqtt *thiz = (Mqtt *) arg;
            DebugL << "handle_pub_ack";
            return 0;
        };

        _contex.handle_pub_rec = [](void *arg, uint16_t pkt_id){
            Mqtt *thiz = (Mqtt *) arg;
            DebugL << "handle_pub_rec";
            return 0;
        };

        _contex.handle_pub_rel = [](void *arg, uint16_t pkt_id){
            Mqtt *thiz = (Mqtt *) arg;
            DebugL << "handle_pub_rel";
            return 0;
        };

        _contex.handle_pub_comp = [](void *arg, uint16_t pkt_id){
            Mqtt *thiz = (Mqtt *) arg;
            DebugL << "handle_pub_comp";
            return 0;
        };

        _contex.handle_sub_ack = [](void *arg, uint16_t pkt_id,
                                    const char *codes, uint32_t count){
            Mqtt *thiz = (Mqtt *) arg;
            DebugL << "handle_sub_ack " << codes << " " << count;
            return 0;
        };

        _contex.handle_unsub_ack = [](void *arg, uint16_t packet_id){
            Mqtt *thiz = (Mqtt *) arg;
            DebugL << "handle_unsub_ack";
            return 0;
        };
        MqttBuffer_Init(&_buf);
    }
    ~Mqtt(){
        MqttBuffer_Destroy(&_buf);
    };

    //连接服务器结果回调
    virtual void onConnect(const SockException &ex) {
        DebugL << ex.what();
        if(ex){
            return;
        }
        _connectBlock();

    }
    //收到数据回调
    virtual void onRecv(const Buffer::Ptr &pBuf) {
        if(_remain.empty()){
            onRecv(pBuf->data(),pBuf->size());
            return;
        }
        _remain.append(pBuf->data(),pBuf->size());
        onRecv((char *)_remain.data(),_remain.size());
    }

    //被动断开连接回调
    virtual void onErr(const SockException &ex) {
        WarnL << ex.what();
    }
    //tcp连接成功后每2秒触发一次该事件
    virtual void onManager() {
        if(_ticker.elapsedTime() > _keepAlive / 2 * 1000){
            _ticker.resetTime();
            keepAlive();
        }
    }

    void connectMqtt(const string &host,
                     uint16_t port,
                     int keep_alive,
                     const string &id,
                     bool clean_session,
                     const string &will_topic,
                     const string &will_payload,
                     enum MqttQosLevel qos,
                     bool will_retain,
                     const string &user,
                     const string &password){
        _keepAlive = keep_alive;
        startConnect(host,port);
        _connectBlock = [=](){
            Mqtt_PackConnectPkt(&_buf,
                                keep_alive,
                                id.data(),
                                clean_session,
                                !will_topic.empty() ? will_topic.data(): nullptr,
                                !will_payload.empty() ? will_payload.data(): nullptr,
                                will_payload.size(),
                                qos,
                                will_retain,
                                !user.empty() ? user.data(): nullptr,
                                !password.empty() ? password.data(): nullptr,
                                password.size());
            sendPacket();
        };
    }

private:
    void onRecv(char *data,int len){
        int customed = 0;
        auto ret = Mqtt_RecvPkt(&_contex,data,len,&customed);
        if(ret != MQTTERR_NOERROR){
            WarnL << "Mqtt_RecvPkt failed:" << hexdump(data,len);
            shutdown();
            return;
        }

        if(customed == 0){
            //完全为消费数据
            if(_remain.empty()){
                _remain.assign(data,len);
            }
            return;
        }
        if(customed < len){
            //消费部分数据
            string str(data + customed,len - customed);
            _remain = str;
            return;
        }
        //消费全部数据
        _remain.clear();
    }
    void keepAlive(){
        Mqtt_PackPingReqPkt(&_buf);
        sendPacket();
    }

    void sendPacket(){
        Mqtt_SendPkt(&_contex,&_buf,0);
        MqttBuffer_Reset(&_buf);
    }
    void sendPublish(const string &topic,
                     const string &payload,
                     enum MqttQosLevel qos,
                     bool retain){
        Mqtt_PackPublishPkt(&_buf,
                            ++_pkt_id,
                            topic.data(),
                            payload.data(),
                            payload.size(),
                            qos,
                            retain,
                            0);
        sendPacket();
    }

    void sendSubscribe(enum MqttQosLevel qos,const std::initializer_list<const char *> &topics){
        Mqtt_PackSubscribePkt(&_buf,
                              ++_pkt_id,
                              qos,
                              topics.begin(),
                              topics.size());
        sendPacket();
    }
private:
    MqttContext _contex;
    MqttBuffer _buf;
    function<void ()> _connectBlock;
    string _remain;
    int _keepAlive;
    Ticker _ticker;
    uint16_t _pkt_id = 0;
};

int main() {
    Logger::Instance().add(std::make_shared<ConsoleChannel>());

    Mqtt::Ptr mqtt = std::make_shared<Mqtt>();
    mqtt->connectMqtt("10.0.9.56",1883,10,"clientId",true,"/Service/JIMIMAX/will","willPayload",MQTT_QOS_LEVEL1, true,"admin","public");

    //退出程序事件处理
    static semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });// 设置退出信号
    sem.wait();
    return 0;
}