#pragma once
#ifndef RC4_H
#define RC4_H
#include "MD5.h"
#include <stdint.h>
#include<string>
#include <cmath>
#include<string.h>
#if defined(__linux)
#include <arpa/inet.h>
#else
#endif
namespace micagent {
typedef int64_t (* get_timestamp_func)();
using namespace std;
class rc4_interface
{
    struct rc4_sbox {
        int i;
        int j;
        uint8_t sbox[256];
        rc4_sbox():i(0),j(0){
            memset(sbox,0,256);
        }
    };
public:
    rc4_interface(get_timestamp_func time_func=nullptr,string dev_ip="0.0.0.0",string mac="ff:ff:ff:ff:ff:ff"):m_time_func(time_func) {
        m_base_key=generate_key(dev_ip,mac);
    }
    inline int encrypt(const char *src, char *des,uint64_t src_size,uint64_t des_size){
        /*safety check*/
        if(des_size-8<src_size)return -1;
        struct rc4_sbox rs;
        uint64_t h=hash_key(src,src_size);
        uint64_t key=m_base_key;
        int64_t time=0;
        if(m_time_func)time=(uint64_t)m_time_func();
        key=hmac(key,(uint64_t)time);
        rc4_init(&rs, key);
        key ^= h;
        uint32_t tmp;
        tmp = htonl(time);
        memcpy(des, &tmp, 4);
        tmp = htonl((uint32_t)key ^ (uint32_t)(key >> 32));
        memcpy(des+4, &tmp, 4);
        rc4_encode(&rs, (const uint8_t *)src, (uint8_t *)des+8, src_size);

        return src_size + 8;
    }
    inline int decrypt(const char *src, char *des,uint64_t src_size,uint64_t des_size,uint64_t src_key){
        /*safety check*/
        if(src_size<8||src_size-8>des_size)return -1;
        uint32_t pt=0, check=0;
        uint64_t h;
        struct rc4_sbox rs;
        memcpy(&pt, src, 4);
        memcpy(&check, src+4, 4);
        pt = ntohl(pt);
        check = ntohl(check);
         src_key = hmac(src_key, pt);
        rc4_init(&rs, src_key);

        rc4_encode(&rs, (const uint8_t *)src+8, (uint8_t *)des, des_size);
        h = hash_key(des, des_size);
        src_key ^= h;

        if (check != ((uint32_t)src_key ^ (uint32_t)(src_key >> 32))) {
            return -1;
        }
        return des_size;
    }
    static uint64_t generate_key(const string &ip="0.0.0.0",const string &mac="ff:ff:ff:ff:ff:ff"){
        MD5_CTX md5_ctx;
        unsigned char aHash[16]={0};
        MD5Init(&md5_ctx);
        MD5Update(&md5_ctx,(unsigned char *)ip.c_str(),ip.length());
        MD5Update(&md5_ctx,(unsigned char *)mac.c_str(),mac.length());
        MD5Final(aHash,&md5_ctx);
        return hash_key(( char *)aHash,16);
    }
private:
    static void rc4_init(struct rc4_sbox *rs, uint64_t seed);
    static void rc4_encode(struct rc4_sbox *rs, const uint8_t *src, uint8_t *des, uint64_t sz);
    static uint64_t hmac(uint64_t x, uint64_t y);
    static  uint64_t hash_key(const char * str, uint64_t sz) {
        uint32_t djb_hash = 5381L;
        uint32_t js_hash = 1315423911L;

        uint64_t i;
        for (i=0;i<sz;i++) {
            uint8_t c = (uint8_t)str[i];
            djb_hash += (djb_hash << 5) + c;
            js_hash ^= ((js_hash << 5) + c + (js_hash >> 2));
        }

        return (uint64_t) djb_hash << 32 | js_hash;
    }
    get_timestamp_func m_time_func;
    uint64_t m_base_key;
};
}
#endif // RC4_H
