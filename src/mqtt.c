/*
base on
v2.0 2016/4/19
 */
#include "mqtt.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>

static const char Mqtt_TrailingBytesForUTF8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

/**
 * 封装发布确认数据包
 * @param buf 存储数据包的缓冲区对象
 * @param pkt_id 被确认的发布数据数据包的ID
 * @return 成功返回MQTTERR_NOERROR
 */
static int Mqtt_PackPubAckPkt(struct MqttBuffer *buf, uint16_t pkt_id);
/**
 * 封装已接收数据包
 * @param buf 存储数据包的缓冲区对象
 * @param pkt_id 被确认的发布数据(Publish)数据包的ID
 * @return 成功返回MQTTERR_NOERROR
 */
static int Mqtt_PackPubRecPkt(struct MqttBuffer *buf, uint16_t pkt_id);
/**
 * 封装发布数据释放数据包
 * @param buf 存储数据包的缓冲区对象
 * @param pkt_id 被确认的已接收数据包(PubRec)的ID
 * @return 成功返回MQTTERR_NOERROR
 */
static int Mqtt_PackPubRelPkt(struct MqttBuffer *buf, uint16_t pkt_id);
/**
 * 封装发布数据完成数据包
 * @param buf 存储数据包的缓冲区对象
 * @param pkt_id 被确认的发布数据释放数据包(PubRel)的ID
 * @return 成功返回MQTTERR_NOERROR
 */
static int Mqtt_PackPubCompPkt(struct MqttBuffer *buf, uint16_t pkt_id);

uint16_t Mqtt_RB16(const char *v)
{
    const uint8_t *uv = (const uint8_t*)v;
    return (((uint16_t)(uv[0])) << 8) | uv[1];
}

void Mqtt_WB16(uint16_t v, char *out)
{
    uint8_t *uo = (uint8_t*)out;
    uo[0] = (uint8_t)(v >> 8);
    uo[1] = (uint8_t)(v);
}


/**
 * 获取数据包大小
 * @param stream 数据流
 * @param size 数据流大小
 * @param len 返回数据包大小
 * @return 返回长度字段占用字节数
 */
 int Mqtt_ReadLength(const char *stream, int size, uint32_t *len)
{
    int i;
    const uint8_t *in = (const uint8_t*)stream;
    uint32_t multiplier = 1;

    *len = 0;
    for(i = 0; i < size; ++i) {
        *len += (in[i] & 0x7f) * multiplier;

        if(!(in[i] & 0x80)) {
            return i + 1;
        }

        multiplier *= 128;
        if(multiplier >= 128 * 128 * 128) {
            return kLengthOutOfRange; // error, out of range
        }
    }

    return kLengthNotEnough; // not complete
}

int Mqtt_DumpLength(size_t len, char *buf)
{
    int i;
    for(i = 1; i <= 4; ++i) {
        *((uint8_t*)buf) = len % 128;
        len /= 128;
        if(len > 0) {
            *buf |= 128;
            ++buf;
        }
        else {
            return i;
        }
    }

    return -1;
}


void Mqtt_PktWriteString(char **buf, const char *str, uint16_t len)
{
    Mqtt_WB16(len, *buf);
    memcpy(*buf + 2, str, len);
    *buf += 2 + len;
}


int Mqtt_CheckClentIdentifier(const char *id)
{
    int len;
    for(len = 0; '\0' != id[len]; ++len) {
        if(!isalnum(id[len])) {
            return -1;
        }
    }

    return len;
}

static int Mqtt_IsLegalUtf8(const char *first, int len)
{
    unsigned char bv;
    const unsigned char *tail = (const unsigned char *)(first + len);

    switch(len) {
    default:
        return MQTTERR_NOT_UTF8;

    case 4:
        bv = *(--tail);
        if((bv < 0x80) || (bv > 0xBF)) {
            return MQTTERR_NOT_UTF8;
        }
    case 3:
        bv = *(--tail);
        if((bv < 0x80) || (bv > 0xBF)) {
            return MQTTERR_NOT_UTF8;
        }
    case 2:
        bv = *(--tail);
        if((bv < 0x80) || (bv > 0xBF)) {
            return MQTTERR_NOT_UTF8;
        }
        switch(*(const unsigned char *)first) {
        case 0xE0:
            if(bv < 0xA0) {
                return MQTTERR_NOT_UTF8;
            }
            break;

        case 0xED:
            if(bv > 0x9F) {
                return MQTTERR_NOT_UTF8;
            }
            break;

        case 0xF0:
            if(bv < 0x90) {
                return MQTTERR_NOT_UTF8;
            }
            break;

        case 0xF4:
            if(bv > 0x8F) {
                return MQTTERR_NOT_UTF8;
            }
            break;

        default:
            break;
        }
    case 1:
        if(((*(unsigned char *)first >= 0x80) && (*(unsigned char *)first < 0xC2)) || (*(unsigned char *)first > 0xF4)) {
            return MQTTERR_NOT_UTF8;
        }
    }

    return MQTTERR_NOERROR;
}

static int Mqtt_CheckUtf8(const char *str, size_t len)
{
    size_t i;

    for(i = 0; i < len;) {
        int ret;
        char utf8_char_len = Mqtt_TrailingBytesForUTF8[(uint8_t)str[i]] + 1;

        if(i + utf8_char_len > len) {
            return MQTTERR_NOT_UTF8;
        }

        ret = Mqtt_IsLegalUtf8(str, utf8_char_len);
        if(ret != MQTTERR_NOERROR) {
            return ret;
        }

        i += utf8_char_len;
        if('\0'== str[i]) {
            break;
        }
    }

    return (int)i;
}



int Mqtt_HandlePingResp(struct MqttContext *ctx, char flags,
                               char *pkt, size_t size)
{
    if((0 != flags) || (0 != size)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    return ctx->handle_ping_resp(ctx->user_data);
}

int Mqtt_HandleConnAck(struct MqttContext *ctx, char flags,
                              char *pkt, size_t size)
{
    char ack_flags, ret_code;

    if((0 != flags) || (2 != size)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    ack_flags = pkt[0];
    ret_code = pkt[1];

    if(((ack_flags & 0x01) && (0 != ret_code)) || (ret_code > 5)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    return ctx->handle_conn_ack(ctx->user_data, ack_flags, ret_code);
}

static int Mqtt_HandlePublish(struct MqttContext *ctx, char flags,
                              char *pkt, size_t size)
{
    const char dup = flags & 0x08;
    const char qos = ((uint8_t)flags & 0x06) >> 1;
    const char retain = flags & 0x01;
    uint16_t topic_len, pkt_id = 0;
    size_t payload_len;
    char *payload;
    char *topic, *cursor;
    int err;

    if(size < 2) {
        return MQTTERR_ILLEGAL_PKT;
    }

    topic_len = Mqtt_RB16(pkt);
    if(size < (size_t)(2 + topic_len)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    switch(qos) {
    case MQTT_QOS_LEVEL0: // qos0 have no packet identifier
        if(0 != dup) {
            return MQTTERR_ILLEGAL_PKT;
        }

        memmove(pkt, pkt + 2, topic_len); // reuse the space to store null terminate
        topic = pkt;

        payload_len = size - 2 - topic_len;
        payload = pkt + 2 + topic_len;
        break;

    case MQTT_QOS_LEVEL1:
    case MQTT_QOS_LEVEL2:
        topic = pkt + 2;
        if(topic_len + 4 > size) {
            return MQTTERR_ILLEGAL_PKT;
        }

        pkt_id = Mqtt_RB16(pkt + topic_len + 2);
        if(0 == pkt_id) {
            return MQTTERR_ILLEGAL_PKT;
        }
        payload_len = size - 4 - topic_len;
        payload = pkt + 4 + topic_len;
        break;

    default:
        return MQTTERR_ILLEGAL_PKT;
    }

    assert(NULL != topic);
    topic[topic_len] = '\0';

    if(Mqtt_CheckUtf8(topic, topic_len) != topic_len) {
        return MQTTERR_ILLEGAL_PKT;
    }

    cursor = topic;
    while('\0' != *cursor) {
        if(('+' == *cursor) || ('#' == *cursor)) {
            return MQTTERR_ILLEGAL_PKT;
        }
        ++cursor;
    }

    err = ctx->handle_publish(ctx->user_data, pkt_id, topic,
                              payload, payload_len, dup,
                              (enum MqttQosLevel)qos);

    // send the publish response.
    if(err >= 0) {
        struct MqttBuffer response[1];
        MqttBuffer_Init(response);

        switch(qos) {
        case MQTT_QOS_LEVEL2:
            assert(0 != pkt_id);
            err = Mqtt_PackPubRecPkt(response, pkt_id);
            break;

        case MQTT_QOS_LEVEL1:
            assert(0 != pkt_id);
            err = Mqtt_PackPubAckPkt(response, pkt_id);
            break;

        default:
            break;
        }

        if((MQTTERR_NOERROR == err) && (MQTT_QOS_LEVEL0 != qos)) {
            if(Mqtt_SendPkt(ctx, response, 0) != response->buffered_bytes) {
                err = MQTTERR_FAILED_SEND_RESPONSE;
            }
        }
        else if(MQTT_QOS_LEVEL0 != qos){
            err = MQTTERR_FAILED_SEND_RESPONSE;
        }

        MqttBuffer_Destroy(response);
    }

    return err;
}

 int Mqtt_HandlePubAck(struct MqttContext *ctx, char flags,
                             char *pkt, size_t size)
{
    uint16_t pkt_id;

    if((0 != flags) || (2 != size)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    pkt_id = Mqtt_RB16(pkt);
    if(0 == pkt_id) {
        return MQTTERR_ILLEGAL_PKT;
    }

    return ctx->handle_pub_ack(ctx->user_data, pkt_id);
}

 int Mqtt_HandlePubRec(struct MqttContext *ctx, char flags,
                             char *pkt, size_t size)
{
    uint16_t pkt_id;
    int err;

    if((0 != flags) || (2 != size)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    pkt_id = Mqtt_RB16(pkt);
    if(0 == pkt_id) {
        return MQTTERR_ILLEGAL_PKT;
    }

    err = ctx->handle_pub_rec(ctx->user_data, pkt_id);
    if(err >= 0) {
        struct MqttBuffer response[1];
        MqttBuffer_Init(response);

        err = Mqtt_PackPubRelPkt(response, pkt_id);
        if(MQTTERR_NOERROR == err) {
            if(Mqtt_SendPkt(ctx, response, 0) != response->buffered_bytes) {
                err = MQTTERR_FAILED_SEND_RESPONSE;
            }
        }

        MqttBuffer_Destroy(response);
    }

    return err;
}

 int Mqtt_HandlePubRel(struct MqttContext *ctx, char flags,
                             char *pkt, size_t size)
{
    uint16_t pkt_id;
    int err;

    if((2 != flags) || (2 != size)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    pkt_id = Mqtt_RB16(pkt);
    if(0 == pkt_id) {
        return MQTTERR_ILLEGAL_PKT;
    }

    err = ctx->handle_pub_rel(ctx->user_data, pkt_id);
    if(err >= 0) {
        struct MqttBuffer response[1];
        MqttBuffer_Init(response);
        err = Mqtt_PackPubCompPkt(response, pkt_id);
        if(MQTTERR_NOERROR == err) {
            if(Mqtt_SendPkt(ctx, response, 0) != response->buffered_bytes) {
                err = MQTTERR_FAILED_SEND_RESPONSE;
            }
        }
        MqttBuffer_Destroy(response);
    }

    return err;
}

 int Mqtt_HandlePubComp(struct MqttContext *ctx, char flags,
                              char *pkt, size_t size)
{
    uint16_t pkt_id;

    if((0 != flags) || (2 != size)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    pkt_id = Mqtt_RB16(pkt);
    if(0 == pkt_id) {
        return MQTTERR_ILLEGAL_PKT;
    }

    return ctx->handle_pub_comp(ctx->user_data, pkt_id);
}

 int Mqtt_HandleSubAck(struct MqttContext *ctx, char flags,
                             char *pkt, size_t size)
{
    uint16_t pkt_id;
    char *code;

    if((0 != flags) || (size < 2)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    pkt_id = Mqtt_RB16(pkt);
    if(0 == pkt_id) {
        return MQTTERR_ILLEGAL_PKT;
    }

    for(code = pkt + 2; code < pkt + size; ++code ) {
        if(*code & 0x7C) {
            return MQTTERR_ILLEGAL_PKT;
        }
    }

    return ctx->handle_sub_ack(ctx->user_data, pkt_id, pkt + 2, size - 2);
}

 int Mqtt_HandleUnsubAck(struct MqttContext *ctx, char flags,
                               char *pkt, size_t size)
{
    uint16_t pkt_id;

    if((0 != flags) || (2 != size)) {
        return MQTTERR_ILLEGAL_PKT;
    }

    pkt_id = Mqtt_RB16(pkt);
    if(0 == pkt_id) {
        return MQTTERR_ILLEGAL_PKT;
    }

    return ctx->handle_unsub_ack(ctx->user_data, pkt_id);
}

static int Mqtt_Dispatch(struct MqttContext *ctx, char fh,  char *pkt, size_t size)
{
    const char flags = fh & 0x0F;

    switch(((uint8_t)fh) >> 4) {
    case MQTT_PKT_PINGRESP:
        return Mqtt_HandlePingResp(ctx, flags, pkt, size);

    case MQTT_PKT_CONNACK:
        return Mqtt_HandleConnAck(ctx, flags, pkt, size);

    case MQTT_PKT_PUBLISH:
        return Mqtt_HandlePublish(ctx, flags, pkt, size);

    case MQTT_PKT_PUBACK:
        return Mqtt_HandlePubAck(ctx, flags, pkt, size);

    case MQTT_PKT_PUBREC:
        return Mqtt_HandlePubRec(ctx, flags, pkt, size);

    case MQTT_PKT_PUBREL:
        return Mqtt_HandlePubRel(ctx, flags, pkt, size);

    case MQTT_PKT_PUBCOMP:
        return Mqtt_HandlePubComp(ctx, flags, pkt, size);

    case MQTT_PKT_SUBACK:
        return Mqtt_HandleSubAck(ctx, flags, pkt, size);

    case MQTT_PKT_UNSUBACK:
        return Mqtt_HandleUnsubAck(ctx, flags, pkt, size);

    default:
        break;
    }

    return MQTTERR_ILLEGAL_PKT;
}


int Mqtt_RecvPkt(struct MqttContext *ctx,char *data,int bytes,int *customed) {
    uint32_t remaining_len = 0;
    char *cursor = data,*end = data + bytes;
    int ret = MQTTERR_NOERROR;

    *customed = 0;
    while(end - cursor >= 2) {
        //第一个字节是固定字节(fixed header)，bytes等于长度字段所占字节数
        bytes = Mqtt_ReadLength(cursor + 1, end - cursor - 1, &remaining_len);

        if(kLengthNotEnough == bytes) {
            //数据不够
            break;
        }

        if(kLengthOutOfRange == bytes) {
            ret = MQTTERR_ILLEGAL_PKT;
            break;
        }

        // one byte for the fixed header
        if(cursor + remaining_len + bytes + 1 > end) {
            //数据不够
            break;
        }

        ret = Mqtt_Dispatch(ctx, cursor[0], cursor + bytes + 1, remaining_len);

        if(ret != MQTTERR_NOERROR) {
            return ret;
        }
		cursor += bytes + 1 + remaining_len;
    }

    *customed = cursor - data;

    return ret;
}

int Mqtt_SendPkt(struct MqttContext *ctx, const struct MqttBuffer *buf, uint32_t offset)
{
    const struct MqttExtent *cursor;
    const struct MqttExtent *first_ext;
    uint32_t bytes;
    int ext_count;
    int i;
    struct iovec *iov;

    if(offset >= buf->buffered_bytes) {
        return 0;
    }

    cursor = buf->first_ext;
    bytes = 0;
    while(cursor && bytes < offset) {
        bytes += cursor->len;
        cursor = cursor->next;
    }

    first_ext = cursor;
    ext_count = 0;
    for(; cursor; cursor = cursor->next) {
        ++ext_count;
    }

    if(0 == ext_count) {
        return 0;
    }

    assert(first_ext);

    iov = (struct iovec*)malloc(sizeof(struct iovec) * ext_count);
    if(!iov) {
        return MQTTERR_OUTOFMEMORY;
    }

    iov[0].iov_base = first_ext->payload + (offset - bytes);
    iov[0].iov_len = first_ext->len - (offset - bytes);

    i = 1;
    for(cursor = first_ext->next; cursor; cursor = cursor->next) {
        iov[i].iov_base = cursor->payload;
        iov[i].iov_len = cursor->len;
        ++i;
    }

    i = ctx->writev_func(ctx->user_data, iov, ext_count);
    free(iov);

    return i;
}



int Mqtt_PackConnectPkt(struct MqttBuffer *buf, uint16_t keep_alive, const char *id,
                        int clean_session, const char *will_topic,
                        const char *will_msg, uint16_t msg_len,
                        enum MqttQosLevel qos, int will_retain, const char *user,
                        const char *password, uint16_t pswd_len)
{
    int ret;
    uint16_t id_len, wt_len, user_len;
    size_t total_len;
    char flags = 0;
    struct MqttExtent *fix_head, *variable_head, *payload;
    char *cursor;


    fix_head = MqttBuffer_AllocExtent(buf, 5);
    if(NULL == fix_head) {
        return MQTTERR_OUTOFMEMORY;
    }

    variable_head = MqttBuffer_AllocExtent(buf, 10);
    if(NULL == variable_head) {
        return MQTTERR_OUTOFMEMORY;
    }

    total_len = 10; // length of the variable header
    id_len = Mqtt_CheckClentIdentifier(id);
    if(id_len < 0) {
        return MQTTERR_ILLEGAL_CHARACTER;
    }
    total_len += id_len + 2;

    if(clean_session) {
        flags |= MQTT_CONNECT_CLEAN_SESSION;
    }

    if(will_msg && !will_topic) {
        return MQTTERR_INVALID_PARAMETER;
        }

    wt_len = 0;
    if(will_topic) {
        flags |= MQTT_CONNECT_WILL_FLAG;
        wt_len = strlen(will_topic);
        if(Mqtt_CheckUtf8(will_topic, wt_len) != wt_len) {
            return MQTTERR_NOT_UTF8;
        }
    }

    switch(qos) {
    case MQTT_QOS_LEVEL0:
        flags |= MQTT_CONNECT_WILL_QOS0;
        break;
    case MQTT_QOS_LEVEL1:
        flags |= (MQTT_CONNECT_WILL_FLAG | MQTT_CONNECT_WILL_QOS1);
        break;
    case MQTT_QOS_LEVEL2:
        flags |= (MQTT_CONNECT_WILL_FLAG | MQTT_CONNECT_WILL_QOS2);
        break;
    default:
        return MQTTERR_INVALID_PARAMETER;
    }

    if(will_retain) {
        flags |= (MQTT_CONNECT_WILL_FLAG | MQTT_CONNECT_WILL_RETAIN);
    }

    if(flags & MQTT_CONNECT_WILL_FLAG) {
        total_len += 4 + wt_len + msg_len;
    }

    if(!user && password) {
        return MQTTERR_INVALID_PARAMETER;
    }

    /*must have user + password
     in v2.0
    */
    if(NULL == user ||
        NULL == password){
        return MQTTERR_INVALID_PARAMETER;
    }


    user_len = 0;
    if(user) {
        flags |= MQTT_CONNECT_USER_NAME;
        user_len = strlen(user);
        ret = Mqtt_CheckUtf8(user, user_len);
        if(user_len != ret) {
            return MQTTERR_NOT_UTF8;
        }

        total_len += user_len + 2;
    }

    if(password) {
        flags |= MQTT_CONNECT_PASSORD;
        total_len += pswd_len + 2;
    }



    payload = MqttBuffer_AllocExtent(buf, total_len - 10);
    fix_head->payload[0] = MQTT_PKT_CONNECT << 4;

    ret = Mqtt_DumpLength(total_len, fix_head->payload + 1);
    if(ret < 0) {
        return MQTTERR_PKT_TOO_LARGE;
    }
    fix_head->len = ret + 1; // ajust the length of the extent

    variable_head->payload[0] = 0;
    variable_head->payload[1] = 4;
    variable_head->payload[2] = 'M';
    variable_head->payload[3] = 'Q';
    variable_head->payload[4] = 'T';
    variable_head->payload[5] = 'T';
    variable_head->payload[6] = 4; // protocol level 4
    variable_head->payload[7] = flags;
    Mqtt_WB16(keep_alive, variable_head->payload + 8);

    //write payload client_id
    cursor = payload->payload;
    Mqtt_PktWriteString(&cursor, id, id_len);

    if(flags & MQTT_CONNECT_WILL_FLAG) {
        if(!will_msg) {
            will_msg = "";
            msg_len = 0;
        }

        Mqtt_PktWriteString(&cursor, will_topic, wt_len);
        Mqtt_PktWriteString(&cursor, will_msg, msg_len);
    }

    if(flags & MQTT_CONNECT_USER_NAME) {
        Mqtt_PktWriteString(&cursor, user, user_len);
    }

    if(flags & MQTT_CONNECT_PASSORD) {
        Mqtt_PktWriteString(&cursor, password, pswd_len);
    }

    MqttBuffer_AppendExtent(buf, fix_head);
    MqttBuffer_AppendExtent(buf, variable_head);
    MqttBuffer_AppendExtent(buf, payload);

    return MQTTERR_NOERROR;
}


int Mqtt_PackPublishPkt(struct MqttBuffer *buf, uint16_t pkt_id, const char *topic,
                        const char *payload, uint32_t size,
                        enum MqttQosLevel qos, int retain, int own)
{
    int ret;
    size_t topic_len, total_len;
    struct MqttExtent *fix_head, *variable_head;
    char *cursor;

    if(0 == pkt_id) {
        return MQTTERR_INVALID_PARAMETER;
    }

    for(topic_len = 0; '\0' != topic[topic_len]; ++topic_len) {
        if(('#' == topic[topic_len]) || ('+' == topic[topic_len])) {
            return MQTTERR_INVALID_PARAMETER;
        }
    }

    if(Mqtt_CheckUtf8(topic, topic_len) != topic_len) {
        return MQTTERR_NOT_UTF8;
    }

    fix_head = MqttBuffer_AllocExtent(buf, 5);
    if(NULL == fix_head) {
        return MQTTERR_OUTOFMEMORY;
    }

    fix_head->payload[0] = MQTT_PKT_PUBLISH << 4;

    if(retain) {
        fix_head->payload[0] |= 0x01;
    }

    total_len = topic_len + size + 2;
    switch(qos) {
    case MQTT_QOS_LEVEL0:
        break;
    case MQTT_QOS_LEVEL1:
        fix_head->payload[0] |= 0x02;
        total_len += 2;
        break;
    case MQTT_QOS_LEVEL2:
        fix_head->payload[0] |= 0x04;
        total_len += 2;
        break;
    default:
        return MQTTERR_INVALID_PARAMETER;
    }

    ret = Mqtt_DumpLength(total_len, fix_head->payload + 1);
    if(ret < 0) {
        return MQTTERR_PKT_TOO_LARGE;
    }
    fix_head->len = ret + 1;

    variable_head = MqttBuffer_AllocExtent(buf, total_len - size);
    if(NULL == variable_head) {
        return MQTTERR_OUTOFMEMORY;
    }
    cursor = variable_head->payload;

    Mqtt_PktWriteString(&cursor, topic, topic_len);
    if(MQTT_QOS_LEVEL0 != qos) {
        Mqtt_WB16(pkt_id, cursor);
    }

    MqttBuffer_AppendExtent(buf, fix_head);
    MqttBuffer_AppendExtent(buf, variable_head);
    if(0 != size) {
        MqttBuffer_Append(buf, (char*)payload, size, own);
    }


    return MQTTERR_NOERROR;
}

int Mqtt_SetPktDup(struct MqttBuffer *buf)
{
    struct MqttExtent *fix_head = buf->first_ext;
    uint8_t pkt_type = ((uint8_t)buf->first_ext->payload[0]) >> 4;
    if(!fix_head || (MQTT_PKT_PUBLISH != pkt_type)) {
        return MQTTERR_INVALID_PARAMETER;
    }

    buf->first_ext->payload[0] |= 0x08;
    return MQTTERR_NOERROR;
}

static int Mqtt_PackPubAckPkt(struct MqttBuffer *buf, uint16_t pkt_id)
{
    struct MqttExtent *ext;

    if(0 == pkt_id)  {
        return MQTTERR_INVALID_PARAMETER;
    }

    ext = MqttBuffer_AllocExtent(buf, 4);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    ext->payload[0]= MQTT_PKT_PUBACK << 4;
    ext->payload[1] = 2;
    Mqtt_WB16(pkt_id, ext->payload + 2);
    MqttBuffer_AppendExtent(buf, ext);

    return MQTTERR_NOERROR;
}

static int Mqtt_PackPubRecPkt(struct MqttBuffer *buf, uint16_t pkt_id)
{
    struct MqttExtent *ext;

    if(0 == pkt_id) {
        return MQTTERR_INVALID_PARAMETER;
    }

    ext = MqttBuffer_AllocExtent(buf, 4);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    ext->payload[0]= MQTT_PKT_PUBREC << 4;
    ext->payload[1] = 2;
    Mqtt_WB16(pkt_id, ext->payload + 2);
    MqttBuffer_AppendExtent(buf, ext);

    return MQTTERR_NOERROR;
}

static int Mqtt_PackPubRelPkt(struct MqttBuffer *buf, uint16_t pkt_id)
{
    struct MqttExtent *ext;

    if(0 == pkt_id) {
        return MQTTERR_INVALID_PARAMETER;
    }

    ext = MqttBuffer_AllocExtent(buf, 4);
    ext->payload[0]= MQTT_PKT_PUBREL << 4 | 0x02;
    ext->payload[1] = 2;
    Mqtt_WB16(pkt_id, ext->payload + 2);
    MqttBuffer_AppendExtent(buf, ext);

    return MQTTERR_NOERROR;
}

static int Mqtt_PackPubCompPkt(struct MqttBuffer *buf, uint16_t pkt_id)
{
    struct MqttExtent *ext;

    if(0 == pkt_id) {
        return MQTTERR_INVALID_PARAMETER;
    }

    ext = MqttBuffer_AllocExtent(buf, 4);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    ext->payload[0]= MQTT_PKT_PUBCOMP << 4;
    ext->payload[1] = 2;
    Mqtt_WB16(pkt_id, ext->payload + 2);
    MqttBuffer_AppendExtent(buf, ext);

    return MQTTERR_NOERROR;
}

int Mqtt_PackSubscribePkt(struct MqttBuffer *buf, uint16_t pkt_id,
                          enum MqttQosLevel qos, const char *const *topics, int topics_len)
{

    int ret;
    size_t topic_len, remaining_len;
    struct MqttExtent *fixed_head, *ext;
    char *cursor;
    size_t topic_total_len = 0;
    const char *topic;

    if(0 == pkt_id) {
        return MQTTERR_INVALID_PARAMETER;
    }

    int i=0;
    for(i=0; i<topics_len; ++i){
        topic = topics[i];
        if(!topic)
            return MQTTERR_INVALID_PARAMETER;
        topic_len = strlen(topic);
        topic_total_len += topic_len;
        if(Mqtt_CheckUtf8(topic, topic_len) != topic_len) {
            return MQTTERR_NOT_UTF8;
        }
    }

    fixed_head = MqttBuffer_AllocExtent(buf, 5);
    if(NULL == fixed_head) {
        return MQTTERR_OUTOFMEMORY;
    }
    fixed_head->payload[0] = (char)((MQTT_PKT_SUBSCRIBE << 4) | 0x00);

    remaining_len = 2 + 2*topics_len + topic_total_len + topics_len*1;  // 2 bytes packet id, 2 bytes topic length + topic + 1 byte reserve
    ext = MqttBuffer_AllocExtent(buf, remaining_len);
    if(NULL == ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    ret = Mqtt_DumpLength(remaining_len, fixed_head->payload + 1);
    if(ret < 0) {
        return MQTTERR_PKT_TOO_LARGE;
    }
    fixed_head->len = ret + 1;

    cursor = ext->payload;
    Mqtt_WB16(pkt_id, cursor);
    cursor += 2;

    //write payload
    for(i=0; i<topics_len; ++i){
        topic = topics[i];
        topic_len = strlen(topic);
        Mqtt_PktWriteString(&cursor, topic, topic_len);
        cursor[0] = qos & 0xFF;
        cursor += 1;
    }

    
    MqttBuffer_AppendExtent(buf, fixed_head);
    MqttBuffer_AppendExtent(buf, ext);


    return MQTTERR_NOERROR;
}

int Mqtt_AppendSubscribeTopic(struct MqttBuffer *buf, const char *topic, enum MqttQosLevel qos)
{
    struct MqttExtent *fixed_head = buf->first_ext;
    struct MqttExtent *ext;
    size_t topic_len;
    uint32_t remaining_len;
    char *cursor;
    int ret;
    const char sub_type = (char)(MQTT_PKT_SUBSCRIBE << 4 | 0x02);
    if(!fixed_head || (sub_type != fixed_head->payload[0]) || !topic) {
        return MQTTERR_INVALID_PARAMETER;
    }

    topic_len = strlen(topic);
    ext = MqttBuffer_AllocExtent(buf, topic_len + 3);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    cursor = ext->payload;
    Mqtt_PktWriteString(&cursor, topic, topic_len);
    cursor[0] = qos;

    if(Mqtt_ReadLength(fixed_head->payload + 1, 4, &remaining_len) < 0) {
        return MQTTERR_INVALID_PARAMETER;
    }

    remaining_len += topic_len + 3;
    ret = Mqtt_DumpLength(remaining_len, fixed_head->payload + 1);
    if(ret < 0) {
        return MQTTERR_PKT_TOO_LARGE;
    }

    fixed_head->len = ret + 1;
    MqttBuffer_AppendExtent(buf, ext);
    return MQTTERR_NOERROR;
}

int Mqtt_PackUnsubscribePkt(struct MqttBuffer *buf, uint16_t pkt_id, const char *const *topics, int topics_len)
{
    struct MqttExtent *fixed_head, *ext;
    size_t topic_len;
    uint32_t remaining_len;
    char *cursor;
    int ret;
    int topic_total_len = 0;
    int i;
    const char* topic;

    if(0 == pkt_id) {
        return MQTTERR_INVALID_PARAMETER;
    }

    for(i=0; i<topics_len; ++i){
        topic = topics[i];
        if(!topic)
            return MQTTERR_INVALID_PARAMETER;
        topic_len = strlen(topic);
        topic_total_len += topic_len;
        if(Mqtt_CheckUtf8(topic, topic_len) != topic_len) {
            return MQTTERR_NOT_UTF8;
        }
    }

    remaining_len = 2 + 2*topics_len + topic_total_len; // 2 bytes for packet id + 2 bytest topic_len + topic

    fixed_head = MqttBuffer_AllocExtent(buf, 5);
    if(!fixed_head) {
        return MQTTERR_OUTOFMEMORY;
    }

    fixed_head->payload[0] = (char)(MQTT_PKT_UNSUBSCRIBE << 4 | 0x00);
    ret = Mqtt_DumpLength(remaining_len, fixed_head->payload + 1);
    if(ret < 0) {
        return MQTTERR_PKT_TOO_LARGE;
    }
    fixed_head->len = ret + 1;

    ext = MqttBuffer_AllocExtent(buf, remaining_len);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    cursor = ext->payload;
    Mqtt_WB16(pkt_id, cursor);
    cursor += 2;

    //write paylod
    for(i=0; i<topics_len; ++i){
        topic = topics[i];
        topic_len = strlen(topic);
        Mqtt_PktWriteString(&cursor, topic, topic_len);
    }

    MqttBuffer_AppendExtent(buf, fixed_head);
    MqttBuffer_AppendExtent(buf, ext);

    return MQTTERR_NOERROR;
}

int Mqtt_AppendUnsubscribeTopic(struct MqttBuffer *buf, const char *topic)
{
    struct MqttExtent *fixed_head = buf->first_ext;
    struct MqttExtent *ext;
    size_t topic_len;
    uint32_t remaining_len;
    char *cursor;
    int ret;
    const char unsub_type =(char)(MQTT_PKT_UNSUBSCRIBE << 4 | 0x02);
    if(!fixed_head || (unsub_type != fixed_head->payload[0]) || !topic) {
        return MQTTERR_INVALID_PARAMETER;
    }

    topic_len = strlen(topic);
    ext = MqttBuffer_AllocExtent(buf, topic_len + 2);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    cursor = ext->payload;
    Mqtt_PktWriteString(&cursor, topic, topic_len);

    if(Mqtt_ReadLength(fixed_head->payload + 1, 4, &remaining_len) < 0) {
        return MQTTERR_INVALID_PARAMETER;
    }

    remaining_len += topic_len + 2;
    ret = Mqtt_DumpLength(remaining_len, fixed_head->payload + 1);
    if(ret < 0) {
        return MQTTERR_PKT_TOO_LARGE;
    }
    fixed_head->len = ret + 1;

    MqttBuffer_AppendExtent(buf, ext);
    return MQTTERR_NOERROR;
}

int Mqtt_PackPingReqPkt(struct MqttBuffer *buf)
{
    struct MqttExtent *ext = MqttBuffer_AllocExtent(buf, 2);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    ext->payload[0] = (char)(MQTT_PKT_PINGREQ << 4);
    ext->payload[1] = 0;
    MqttBuffer_AppendExtent(buf, ext);

    return MQTTERR_NOERROR;
}

int Mqtt_PackDisconnectPkt(struct MqttBuffer *buf)
{
    struct MqttExtent *ext = MqttBuffer_AllocExtent(buf, 2);
    if(!ext) {
        return MQTTERR_OUTOFMEMORY;
    }

    ext->payload[0] = (char)(MQTT_PKT_DISCONNECT << 4);
    ext->payload[1] = 0;
    MqttBuffer_AppendExtent(buf, ext);

    return MQTTERR_NOERROR;
}

