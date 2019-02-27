//
// Created by xzl on 2019/2/27.
//

#ifndef MQTT_MD5_H
#define MQTT_MD5_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define MD5_HEX_LEN 16
void md5(const uint8_t *initial_msg, size_t initial_len, uint8_t *digest);
char *hexdump(uint8_t *digest,size_t digest_len, char *str_buf, size_t str_buf_len,int upCase );


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus


#endif //MQTT_MD5_H
