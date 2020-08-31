#ifndef BITS_HELPER_H
#define BITS_HELPER_H
#include<cstdint>
#include<ctype.h>
#include<memory>
#include<cstring>
#if defined(WIN32) || defined(_WIN32)
#else
#include<endian.h>
#endif
//---------------------------------------------------------------------
// WORD ORDER
//---------------------------------------------------------------------
#undef IWORDS_BIG_ENDIAN
#ifdef _BIG_ENDIAN_
#if _BIG_ENDIAN_
#define IWORDS_BIG_ENDIAN 1
#endif
#endif
#ifndef __BYTE_ORDER
#if defined(__hppa__) || \
    defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
    (defined(__MIPS__) && defined(__MIPSEB__)) || \
    defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
    defined(__sparc__) || defined(__powerpc__) || \
    defined(__mc68000__) || defined(__s390x__) || defined(__s390__)
#ifndef IWORDS_BIG_ENDIAN
#define IWORDS_BIG_ENDIAN 1
#endif
#else
#ifndef IWORDS_BIG_ENDIAN
#define IWORDS_BIG_ENDIAN 0
#endif
#endif //
#else
#if __BYTE_ORDER ==__LITTLE_ENDIAN
#ifndef IWORDS_BIG_ENDIAN
#define IWORDS_BIG_ENDIAN 0
#endif
#else
#ifndef IWORDS_BIG_ENDIAN
#define IWORDS_BIG_ENDIAN 1
#endif
#endif
#endif

namespace micagent {
using namespace std;
class bits_context{
    static constexpr uint32_t MAX_BUF_LEN=0x1FFFFFFF;
    static constexpr uint32_t INVALID_BITS_OFFSET=0XFFFFFFFF;
public:
    /**
     * @brief bits_context init a bits_context
     * @param bits_buf raw_buf
     * @param bits_len raw_buf's len
     */
    bits_context(void *bits_buf,uint32_t bits_len);
    /**
     * @brief reset reuse the context with another buf
     * @param bits_buf
     * @param bits_len
     */
    void reset(void *bits_buf,uint32_t bits_len);
    /**
     * @brief read_bits   read some bits from the context
     * @param read_len the len you want read <=64
     * @param save where the return value is saved
     * @param offset the offet of read bits
     * @return the next pos will be read ,INVALID_BITS_OFFSET means that an error has occurred
     */
    inline uint32_t read_bits(uint8_t read_len,uint64_t &save,uint32_t offset=INVALID_BITS_OFFSET)
    {
        save=0;
        uint32_t read_begin=offset;
        //从上次读取的pos读取
        if(INVALID_BITS_OFFSET==offset)read_begin=m_last_offet;
        //越界直接返回
        if(m_max_offset-read_len<read_begin)return INVALID_BITS_OFFSET;
        //开始读取的字节编号
        auto begin_byte=read_begin/8;
        //当前字节已用位数
        uint8_t last_offset=read_begin%8;
#if IWORDS_BIG_ENDIAN
        //ABBBBBBC
        //A:当前可读字节剩余部分
        //B：完整的字节部分
        //D：读取完完整字节后仍需读取的长度
        int8_t padding_bit=static_cast<int8_t>(read_len-static_cast<int8_t>(8-last_offset));
        if(padding_bit<=0)
        {//A
            auto mask=get_bit_mask(static_cast<uint8_t>(0-padding_bit),read_len)&0xff;
            save|=(m_src_buf.get()[begin_byte]&mask)>>(0-padding_bit);
        }
        else {
            uint8_t patern_offset=8-last_offset;
            auto mask=get_bit_mask(0,patern_offset)&0xff;
            //A
            save|=(m_src_buf.get()[begin_byte]&mask);
            uint8_t left_bits=read_len-patern_offset;
            uint8_t bits_fix=left_bits%8;
            uint8_t bytes_fix=left_bits/8;
            for(uint8_t i=0;i<bytes_fix;++i)
            {//B
                save<<=8;
                save|=m_src_buf.get()[begin_byte+i];
            }
            save<<=bits_fix;
            //C
            save|=m_src_buf.get()[begin_byte+bytes_fix]>>(8-bits_fix);
        }
#else
        //尾部字节需补齐的位数
        int8_t padding_bit=static_cast<int8_t>(read_len+last_offset-static_cast<int8_t>(64));
        save=*(reinterpret_cast<uint64_t *>(m_src_buf.get()+begin_byte));
        //去掉高位
        if(padding_bit<0){
            save<<=(0-padding_bit);
            save>>=(0-padding_bit);
        }
        //去掉低位
        save>>=last_offset;
        //补齐高位
        if(padding_bit>0){
            uint8_t padding=*(reinterpret_cast<uint8_t *>(m_src_buf.get()+begin_byte+8));
            //将高位置0
            padding<<=(8-padding_bit);
            padding>>=(8-padding_bit);
            //高位填充至指定位置
            save|=(static_cast<uint64_t>(padding)<<(read_len-padding_bit));
        }
#endif
        //修改读取起点
        m_last_offet=read_begin+read_len;
        return m_last_offet;
    }
    /**
     * @brief write_bits write a value to specific bits
     * @param write_len  the write bits len
     * @param input  the value to be written to the context
     * @param offset the offset of the write begin
     * @return the next write begin
     */
    inline uint32_t write_bits(uint8_t write_len,uint64_t input,uint32_t offset=INVALID_BITS_OFFSET )
    {
        uint32_t write_begin=offset;
        if(INVALID_BITS_OFFSET==offset)write_begin=m_last_offet;
        if(m_max_offset-write_len<write_begin)return INVALID_BITS_OFFSET;
        //保留有效位
        input<<=(64-write_len);
        input>>=(64-write_len);

        auto begin_byte=write_begin/8;
        uint8_t last_offset=write_begin%8;
        //padding_bit 为首字节写入后的剩余bit长度
        int8_t padding_bit=static_cast<int8_t>(write_len-static_cast<int8_t>(8-last_offset));
#if IWORDS_BIG_ENDIAN
                //ABBBBBBC
        //A:当前首字节需要填充的部分
        //B：完整的字节填充部分
        //C：完整字节填充完之后的剩余字节数
        if(padding_bit<0)
        {//先将写入位清0
            auto mask=get_bit_mask(static_cast<uint8_t>(0-padding_bit),write_len)&0xff;
            m_src_buf.get()[begin_byte]&=~mask;
            m_src_buf.get()[begin_byte]|=(input<<(0-padding_bit))&0xff;
        }
        else {
            uint8_t patern_offset=8-last_offset;
            uint8_t left_bits=write_len-patern_offset;
            uint8_t bits_fix=left_bits%8;
            uint8_t bytes_fix=left_bits/8;
            //C 写入高地址
            auto mask=get_bit_mask(0,bits_fix);
            m_src_buf.get()[begin_byte+bytes_fix]&=(~(mask<<(8-bits_fix)));
            m_src_buf.get()[begin_byte+bytes_fix]|=(input&mask)<<(8-bits_fix);
            input>>=bits_fix;
            //B 字节填充
            for(int i=bytes_fix-1;i>=0;i--)
            {
                m_src_buf.get()[begin_byte+i]=0;
                m_src_buf.get()[begin_byte+i]=input&0xff;
                input>>=8;
            }
            //A 写入低位
            m_src_buf.get()[begin_byte]|=input;
        }
#else
        //写入首字节,先清空再写入
        auto mask=get_bit_mask(last_offset,8-last_offset);
        if(padding_bit<0){
            mask=get_bit_mask(last_offset,write_len);
        }
        m_src_buf.get()[begin_byte]&=~mask;

        m_src_buf.get()[begin_byte]|=((input&0xff)<<last_offset);
        if(padding_bit>0) {
            //多字节写入,将低位置0
            input>>=(8-last_offset);
            memcpy(m_src_buf.get()+begin_byte+1,&input,8);
        }
#endif
        //修改写入起点
        m_last_offet=write_begin+write_len;
        return m_last_offet;
    }
    /**
     * @brief get_buf_size get the raw_buf'len
     * @return
     */
    uint32_t get_buf_size()const{return  m_buf_size;}
    /**
     * @brief get_src_buf if you have changed the buf with write_bits(),you can update you raw_buf with the content that this return buf is filled with
     * @return the context's copy buf
     */
    shared_ptr<uint8_t>get_src_buf(){return m_src_buf;}
    ~bits_context();
    /**
     * @brief get_bit_mask generate a 64-bits mask with a specific range
     * @param begin the range begin
     * @param len the range length
     * @return
     * @details if the len>64,it will cut out the value to make it below to or equal to 64
     */
    static uint64_t get_bit_mask(uint8_t begin,uint8_t len){
        begin&=0x3f;
        len&=0x7f;
        if(len!=0x40)len&=0x3f;
        uint64_t bit_mask=0;
        uint64_t mask_begin=static_cast<uint64_t>(static_cast<uint64_t>(1)<<(begin));
        len=len+begin>64?64-begin:len;
        while(len>0)
        {
            bit_mask|=mask_begin;
            mask_begin<<=1;
            --len;
        }
        return  bit_mask;
    }
private:
    shared_ptr<uint8_t>m_src_buf;
    uint32_t m_buf_size;
    uint32_t m_last_offet;
    uint32_t m_max_offset;
};
}
#endif // BITS_HELPER_H
