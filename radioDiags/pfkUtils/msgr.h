/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */
/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#ifndef __MSGR_H__
#define __MSGR_H__

#include <inttypes.h>
#include <string.h>
#include <string>
#include <netinet/in.h>

#include "LockWait.h"

class PfkMsg {
public:
    static const uint32_t MAX_FIELDS = 1000;
private:
    struct field_t {
        uint16_t len;
        void * data;
    };
    field_t fields[MAX_FIELDS];
    uint8_t * buffer;
    uint32_t max_msg_len;
    uint32_t num_fields;
    uint32_t msg_len;
    static const uint8_t MAGIC1 = 0xA5;
    static const uint8_t MAGIC2 = 0x5A;
    friend class PfkMsgr;
    const void * getBuffer(uint32_t *plen) const;
    uint8_t _buffer[16384];
public:
    PfkMsg(void);
    ~PfkMsg(void);

// methods for sending
    void init(uint16_t msg_type, void * buf = NULL, uint32_t _buflen = 0);
    bool add_field(uint16_t len, void * data);
    bool add_val(uint8_t val);
    bool add_val(uint16_t val);
    bool add_val(uint32_t val);
    bool add_val(uint64_t val);
    uint32_t finish(void);

// methods for receiving
    // return true if a message is ready to consume
    // *consumed is filled out with how many bytes were
    // consumed from the buf.
    bool parse(void * buf, uint32_t len, uint32_t *consumed);
    uint32_t get_num_fields(void) const;
    const void * get_field(uint16_t *len, uint32_t fieldnum) const;
    bool get_val(uint8_t *val, uint32_t fieldnum) const;
    bool get_val(uint16_t *val, uint32_t fieldnum) const;
    bool get_val(uint32_t *val, uint32_t fieldnum) const;
    bool get_val(uint64_t *val, uint32_t fieldnum) const;
};

class PfkMsgr {
    PfkMsg rcvd_msg;
    int closerPipe[2];
    int fd; // only if server
    int dataFd;
    void common_init(const std::string &host, int port);
    bool threadRunning;
    bool isServer;
    struct sockaddr_in sa;
    static void * _thread_entry(void * arg);
    void thread_main(void);
    static const uint32_t RCV_BUFFER_SIZE = 32768;
    uint8_t rcvBuffer[RCV_BUFFER_SIZE];
    uint32_t rcvBufferSize;
    std::string unsent_portion;
    WaitUtil::Lockable lock;
protected:
    virtual void handle_connected(void) = 0;
    virtual void handle_disconnected(void) = 0;
    virtual void handle_msg(const PfkMsg &msg) = 0;
public:
    PfkMsgr(int port); // server
    PfkMsgr(const std::string &host, int port); // client
    virtual ~PfkMsgr(void);
    void stop(void);
    // return false if msg wasn't sent
    bool send_msg(const PfkMsg &msg);
    bool send_buf(const void *buf, int len);
};

// inline impls below

inline
PfkMsg :: PfkMsg(void)
{
    msg_len = 0;
    num_fields = 0;
    buffer = NULL;
    max_msg_len = 0;
}

inline
PfkMsg :: ~PfkMsg(void)
{
}

inline void
PfkMsg :: init(uint16_t msg_type, void * buf, uint32_t _buflen)
{
    if (buf == NULL)
    {
        buf = _buffer;
        _buflen = sizeof(_buffer);
    }
    buffer = (uint8_t *) buf;
    max_msg_len = _buflen;
    buffer[0] = MAGIC1;
    buffer[1] = MAGIC2;
    msg_len = 6; // reserve for length and numfields
    num_fields = 0;
    add_val(msg_type);
}

inline uint32_t
PfkMsg :: finish(void)
{
    buffer[2] = (msg_len    >> 8) & 0xFF;
    buffer[3] = (msg_len    >> 0) & 0xFF;
    buffer[4] = (num_fields >> 8) & 0xFF;
    buffer[5] = (num_fields >> 0) & 0xFF;
    return msg_len;
}

inline const void *
PfkMsg :: getBuffer(uint32_t *plen) const
{
    *plen = msg_len;
    return buffer;
}

inline uint32_t
PfkMsg :: get_num_fields(void) const
{
    return num_fields;
}

inline bool
PfkMsg :: add_field(uint16_t len, void * data)
{
    if (((uint32_t)len + msg_len) > max_msg_len)
        return false;
    if (num_fields >= MAX_FIELDS)
        return false;
    buffer[msg_len++] = (len >> 8) & 0xFF;
    buffer[msg_len++] = (len >> 0) & 0xFF;
    fields[num_fields].len = len;
    fields[num_fields].data = buffer + msg_len;
    memcpy(buffer + msg_len, data, len);
    msg_len += len;
    num_fields ++;
    return true;
}

inline bool
PfkMsg :: add_val(uint8_t val)
{
    return add_field(1, &val);
}

inline bool
PfkMsg :: add_val(uint16_t val)
{
    uint8_t tmp[2];
    tmp[0] = (val >> 8) & 0xFF;
    tmp[1] = (val >> 0) & 0xFF;
    return add_field(2,tmp);
}

inline bool
PfkMsg :: add_val(uint32_t val)
{
    uint8_t tmp[4];
    tmp[0] = (val >> 24) & 0xFF;
    tmp[1] = (val >> 16) & 0xFF;
    tmp[2] = (val >>  8) & 0xFF;
    tmp[3] = (val >>  0) & 0xFF;
    return add_field(4,tmp);
}

inline bool
PfkMsg :: add_val(uint64_t val)
{
    uint8_t tmp[8];
    tmp[0] = (val >> 56) & 0xFF;
    tmp[1] = (val >> 48) & 0xFF;
    tmp[2] = (val >> 40) & 0xFF;
    tmp[3] = (val >> 32) & 0xFF;
    tmp[4] = (val >> 24) & 0xFF;
    tmp[5] = (val >> 16) & 0xFF;
    tmp[6] = (val >>  8) & 0xFF;
    tmp[7] = (val >>  0) & 0xFF;
    return add_field(8,tmp);
}

inline const void *
PfkMsg :: get_field(uint16_t *len, uint32_t fieldnum) const
{
    if (fieldnum >= num_fields)
        return NULL;
    *len = fields[fieldnum].len;
    return fields[fieldnum].data;
}

inline bool
PfkMsg :: get_val(uint8_t *val, uint32_t fieldnum) const
{
    uint16_t len;
    uint8_t * tmp = (uint8_t *) get_field(&len, fieldnum);
    if (tmp == NULL)
        return false;
    *val = *tmp;
    return true;
}

inline bool
PfkMsg :: get_val(uint16_t *val, uint32_t fieldnum) const
{
    uint16_t len;
    uint8_t * tmp = (uint8_t *) get_field(&len, fieldnum);
    if (tmp == NULL)
        return false;
    *val = ((uint16_t)(tmp[0]) << 8) + (uint16_t)(tmp[1]);
    return true;
}

inline bool
PfkMsg :: get_val(uint32_t *val, uint32_t fieldnum) const
{
    uint16_t len;
    uint8_t * tmp = (uint8_t *) get_field(&len, fieldnum);
    if (tmp == NULL)
        return false;
    *val =
        ((uint32_t)(tmp[0]) << 24) + ((uint32_t)(tmp[1]) << 16) +
        ((uint32_t)(tmp[2]) <<  8) +  (uint32_t)(tmp[3]);
    return true;
}

inline bool
PfkMsg :: get_val(uint64_t *val, uint32_t fieldnum) const
{
    uint16_t len;
    uint8_t * tmp = (uint8_t *) get_field(&len, fieldnum);
    if (tmp == NULL)
        return false;
    *val =
        ((uint64_t)(tmp[0]) << 56) + ((uint64_t)(tmp[1]) << 48) +
        ((uint64_t)(tmp[2]) << 40) + ((uint64_t)(tmp[3]) << 32) +
        ((uint64_t)(tmp[4]) << 24) + ((uint64_t)(tmp[5]) << 16) +
        ((uint64_t)(tmp[6]) <<  8) +  (uint64_t)(tmp[7]);
    return true;
}

#endif /* __MSGR_H__ */
