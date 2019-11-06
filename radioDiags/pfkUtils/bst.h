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

#ifndef __BYTESTREAM_H__
#define __BYTESTREAM_H__

// ByteStream interface (BST)

#include <sys/types.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "posix_fe.h"

enum BST_OP {
//    BST_OP_NONE,       // stream is uninitialized.
    BST_OP_ENCODE = 1,   // encode data types into a byte stream.
    BST_OP_CALC_SIZE,    // calculate the amount of memory required by
    /**/                 //   an encode sequence but do not actually encode.
    BST_OP_DECODE,       // decode a byte stream and alloc dynamic memory.
    BST_OP_MEM_FREE      // free any dynamically allocated memory.
};

class BST_STREAM {
protected:
    BST_OP  op;
public:
    BST_STREAM(void) { op = (BST_OP)0; /* BST_OP_NONE */ }
    virtual ~BST_STREAM(void) { /*placeholder*/ }
    BST_OP get_op(void) { return op; }
    virtual uint8_t * get_ptr(int step) = 0;
};

class BST_STREAM_BUFFER : public BST_STREAM {
    uint8_t * buffer;
    int     buffer_size;
    uint8_t * ptr;
    int     size;
    int     remaining;
    bool    my_buffer;
public:
    // the user supplies the buffer and also takes care
    // of deleting it.
    BST_STREAM_BUFFER( uint8_t * _buffer, int _size ) {
        buffer = _buffer;
        buffer_size = _size;
        my_buffer = false;
    }
    // this object creates the buffer, which is deleted when
    // this object is deleted.
    BST_STREAM_BUFFER( int _size ) {
        buffer = new uint8_t[_size];
        buffer_size = _size;
        my_buffer = true;
    }
    ~BST_STREAM_BUFFER(void) {
        if (my_buffer)
            delete[] buffer;
    }
    // reset the current buffer position to the beginning, used
    // to reset the stream state for beginning a new operation.
    void start(BST_OP _op, uint8_t * _buffer=NULL, int _size=0) {
        op = _op; 
        if (_buffer && _size)
        {
            if (my_buffer)
                delete[] buffer;
            buffer = _buffer;
            buffer_size = _size;
            my_buffer = false;
        }
        ptr = buffer;
        size = 0;
        remaining = buffer_size;
    }
    // return the start of the buffer; used at the end of an
    // encode to collect the constructed bytestream.
    uint8_t * get_finished_buffer(void) { return buffer; }
    // return the amount of data in the buffer; used at the end
    // of an encode to collect the size of the constructed bytestream.
    int get_finished_size(void) { return size; }
    // return the current working pointer.  used by bst_op in all 
    // data types as the bytestream is processed.  specify the number
    // of bytes needed.  if there are not enough left in the buffer,
    // returns NULL.
    /*virtual*/ uint8_t * get_ptr(int step) {
        if (remaining < step)
            return NULL;
        uint8_t * ret = ptr;
        remaining -= step;
        size += step;
        if (op == BST_OP_CALC_SIZE)
            // return something, anything, that is not NULL,
            // because the user may have specified a NULL buffer ptr
            // for the calc op.
            return (uint8_t*)1;
        ptr += step;
        return ret;
    }
};

// base class for any type which can be represented as a ByteStream
class BST {
    BST * head;
    BST * tail;
    BST * next;
    // consider adding BST * parent here and having
    // destructor de-register self from parent.
    friend class BST_UNION;
public:
    BST(BST *parent) {
        head = tail = next = NULL;
        if (parent) {
            if (parent->tail) {
                parent->tail->next = this;
                parent->tail = this;
            } else {
                parent->head = parent->tail = this;
            }
        }
    }
    virtual ~BST(void) { /*placeholder*/ }
    // return true if operation was successful; 
    // return false if some problem (out of buffer, etc)
    virtual bool bst_op( BST_STREAM *str ) {
        for (BST * b = head; b; b = b->next)
            if (!b->bst_op(str))
                return false;
        return true;
    }
    int bst_calc_size(void) {
        BST_STREAM_BUFFER str(NULL,65536);
        str.start(BST_OP_CALC_SIZE);
        bst_op(&str);
        return str.get_finished_size();
    }
    uint8_t * bst_encode( int * len ) {
        *len = bst_calc_size();
        uint8_t * buffer = new uint8_t[*len];
        if (bst_encode(buffer, len) == false)
        {
            *len = 0;
            delete[] buffer;
            return NULL;
        }
        return buffer;
    }
    bool bst_encode( uint8_t * buffer, int * len ) {
        BST_STREAM_BUFFER str(buffer,*len);
        str.start(BST_OP_ENCODE);
        if (bst_op(&str) == false)
            return false;
        *len = str.get_finished_size();
        return true;
    }
    bool bst_decode( uint8_t * buffer, int len ) {
        BST_STREAM_BUFFER str(buffer,len);
        str.start(BST_OP_DECODE);
        return bst_op(&str);
    }
    void bst_free(void) {
        BST_STREAM_BUFFER str(NULL,0);
        str.start(BST_OP_MEM_FREE);
        bst_op(&str);
    }
};

// BST base types follow.

class BST_UINT64_t : public BST {
    uint64_t v;
public:
    BST_UINT64_t(BST *parent) : BST(parent) { }
    uint64_t &operator()() { return v; }
    const uint64_t &operator()() const { return v; }
    void set(uint64_t _v) { v = _v; }
    const uint64_t get(void) const { return v; }
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        uint8_t * ptr;
        switch (str->get_op())
        {
        case BST_OP_ENCODE:
            ptr = str->get_ptr(8);
            if (!ptr)
                return false;
            ptr[0] = (v >> 56) & 0xFF;
            ptr[1] = (v >> 48) & 0xFF;
            ptr[2] = (v >> 40) & 0xFF;
            ptr[3] = (v >> 32) & 0xFF;
            ptr[4] = (v >> 24) & 0xFF;
            ptr[5] = (v >> 16) & 0xFF;
            ptr[6] = (v >>  8) & 0xFF;
            ptr[7] = (v >>  0) & 0xFF;
            return true;
        case BST_OP_CALC_SIZE:
            if (!str->get_ptr(8))
                return false;
            return true;
        case BST_OP_DECODE:
            ptr = str->get_ptr(8);
            if (!ptr)
                return false;
            v = ((unsigned long long)ptr[0] << 56) +
                ((unsigned long long)ptr[1] << 48) +
                ((unsigned long long)ptr[2] << 40) +
                ((unsigned long long)ptr[3] << 32) +
                ((unsigned long long)ptr[4] << 24) +
                ((unsigned long long)ptr[5] << 16) +
                ((unsigned long long)ptr[6] <<  8) +
                ((unsigned long long)ptr[7] <<  0);
            return true;
        case BST_OP_MEM_FREE:
            return true;
        }
        return false;
    }
};

class BST_UINT32_t : public BST {
    uint32_t v;
public:
    BST_UINT32_t(BST *parent) : BST(parent) { }
    void set(uint32_t _v) { v = _v; }
    uint32_t &operator()() { return v; }
    const uint32_t &operator()() const { return v; }
    const uint32_t get(void) const { return v; }
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        uint8_t * ptr;
        switch (str->get_op())
        {
        case BST_OP_ENCODE:
            ptr = str->get_ptr(4);
            if (!ptr)
                return false;
            ptr[0] = (v >> 24) & 0xFF;
            ptr[1] = (v >> 16) & 0xFF;
            ptr[2] = (v >>  8) & 0xFF;
            ptr[3] = (v >>  0) & 0xFF;
            return true;
        case BST_OP_CALC_SIZE:
            if (!str->get_ptr(4))
                return false;
            return true;
        case BST_OP_DECODE:
            ptr = str->get_ptr(4);
            if (!ptr)
                return false;
            v = (ptr[0] << 24) + (ptr[1] << 16) +
                (ptr[2] <<  8) + (ptr[3] <<  0);
            return true;
        case BST_OP_MEM_FREE:
            return true;
        }
        return false;
    }
};

class BST_UINT16_t : public BST {
    uint16_t v;
public:
    BST_UINT16_t(BST *parent) : BST(parent) { }
    void set(uint16_t _v) { v = _v; }
    uint16_t &operator()() { return v; }
    const uint16_t &operator()() const { return v; }
    const uint16_t get(void) const { return v; }
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        uint8_t * ptr;
        switch (str->get_op())
        {
        case BST_OP_ENCODE:
            ptr = str->get_ptr(2);
            if (!ptr)
                return false;
            ptr[0] = (v >>  8) & 0xFF;
            ptr[1] = (v >>  0) & 0xFF;
            return true;
        case BST_OP_CALC_SIZE:
            if (!str->get_ptr(2))
                return false;
            return true;
        case BST_OP_DECODE:
            ptr = str->get_ptr(2);
            if (!ptr)
                return false;
            v = (ptr[0] <<  8) + (ptr[1] <<  0);
            return true;
        case BST_OP_MEM_FREE:
            return true;
        }
        return false;
    }
};

class BST_UINT8_t : public BST {
    uint8_t v;
public:
    BST_UINT8_t(BST *parent) : BST(parent) { }
    void set(uint8_t _v) { v = _v; }
    uint8_t &operator()() { return v; }
    const uint8_t &operator()() const { return v; }
    const uint8_t get(void) const { return v; }
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        uint8_t * ptr;
        switch (str->get_op())
        {
        case BST_OP_ENCODE:
            ptr = str->get_ptr(1);
            if (!ptr)
                return false;
            ptr[0] = (v >>  0) & 0xFF;
            return true;
        case BST_OP_CALC_SIZE:
            if (!str->get_ptr(1))
                return false;
            return true;
        case BST_OP_DECODE:
            ptr = str->get_ptr(1);
            if (!ptr)
                return false;
            v = (ptr[0] <<  0);
            return true;
        case BST_OP_MEM_FREE:
            return true;
        }
        return false;
    }
};

struct BST_TIMEVAL : public BST {
    BST_TIMEVAL(BST *parent)
        : BST(parent), btv_sec(this), btv_usec(this) { }
    ~BST_TIMEVAL(void) { }
    BST_UINT64_t  btv_sec;
    BST_UINT32_t  btv_usec;
    void set(const struct timeval &tv) {
        btv_sec() = (uint64_t) tv.tv_sec;
        btv_usec() = (uint32_t) tv.tv_usec;
    }
    const void get(struct timeval &tv) const {
        tv.tv_sec = (typeof(tv.tv_sec)) btv_sec();
        tv.tv_usec = (typeof(tv.tv_usec)) btv_usec();
    }
    const std::string Format(void) {
        pxfe_timeval v;
        get(v);
        return v.Format();
    }
};

// BUG: this can't handle strings >= 65536 in length.
// it is legal for string to be a null ptr; however
// it will be represented identically as a zero-length string.
class BST_STRING : public BST {
    std::string string;
public:
    BST_STRING(BST *parent) : BST(parent) {  }
    virtual ~BST_STRING(void) {  }
    std::string &operator()() { return string; }
    const std::string &operator()() const { return string; }
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        uint8_t * ptr;
        BST_UINT16_t  len(NULL);
        switch (str->get_op())
        {
        case BST_OP_ENCODE:
            len() = string.length();
            if (!len.bst_op(str))
                return false;
            if (len() > 0)
            {
                ptr = str->get_ptr(len());
                if (!ptr)
                    return false;
                memcpy(ptr, string.c_str(), len());
            }
            return true;
        case BST_OP_CALC_SIZE:
            len() = string.length();
            if (!len.bst_op(str))
                return false;
            if (len() > 0)
                if (!str->get_ptr(len()))
                    return false;
            return true;
        case BST_OP_DECODE:
            if (!len.bst_op(str))
                return false;
            if (len() > 0)
            {
                ptr = str->get_ptr(len());
                if (!ptr)
                    return false;
                string.resize(len());
                memcpy((void*)string.c_str(), ptr, len());
            }
            return true;
        case BST_OP_MEM_FREE:
            string.clear();
            return true;
        };
        return false;
    }
    void set(const char * str) {
        string = str;
    }
    void set(const std::string &str) {
        string = str;
    }
};

// it is legal for pointer to be NULL pointer.
// it will be encoded as a NULL ptr and the receiver will
// set it as NULL (as expected).
template <class T>
class BST_POINTER : public BST {
public:
    BST_POINTER(BST *parent) : BST(parent) { pointer = NULL; }
    virtual ~BST_POINTER(void) { if (pointer) delete pointer; }
    T * pointer;
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        BST_UINT8_t  flag(NULL);
        switch (str->get_op())
        {
        case BST_OP_ENCODE:
        case BST_OP_CALC_SIZE:
            flag() = (pointer != NULL) ? 1 : 0;
            if (!flag.bst_op(str))
                return false;
            if (pointer)
                if (!pointer->bst_op(str))
                    return false;
            return true;
        case BST_OP_DECODE:
            if (!flag.bst_op(str))
                return false;
            if (flag() == 0)
                pointer = NULL;
            else
            {
                if (!pointer)
                    pointer = new T(NULL);
                if (!pointer->bst_op(str))
                    return false;
            }
            return true;
        case BST_OP_MEM_FREE:
            if (pointer)
            {
                if (!pointer->bst_op(str))
                    return false;
                delete pointer;
            }
            pointer = NULL;
            return true;
        }
        return false;
    }
};

struct BST_ARRAY_ERROR {
};

template <class T>
class BST_ARRAY : public BST {
    int num_items;
    T ** array;
    void bst_var_array_free(void) {
        if (array) {
            for (int i=0; i < num_items; i++)
                delete array[i];
            delete array;
            array = NULL;
            num_items = 0;
        }
    }
public:
    BST_ARRAY(BST *parent) : BST(parent) { array = NULL; num_items = 0; }
    virtual ~BST_ARRAY(void) { bst_var_array_free(); }
    T &operator[](const int index) {
        if (index < 0 || index >= num_items)
            throw BST_ARRAY_ERROR();
        return *array[index];
    }
    const T &operator[](const int index) const {
        if (index < 0 || index >= num_items)
            throw BST_ARRAY_ERROR();
        return *array[index];
    }
    int length(void) const { return num_items; }
    void resize(int c) {
        if (c == num_items)
            return;
        int i;
        T ** narray = NULL;
        if (c > 0)
            narray = new T*[c];
        if (c > num_items) {
            for (i=0; i < num_items; i++)
                narray[i] = array[i];
            for (; i < c; i++)
                narray[i] = new T(NULL);
        } else {// c < num_items
            for (i=0; i < c; i++)
                narray[i] = array[i];
            for (;i < num_items; i++)
                delete array[i];
        }
        if (array)
            delete[] array;
        array = narray;
        num_items = c;
    }
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        int i;
        BST_UINT32_t  count(NULL);
        switch (str->get_op())
        {
        case BST_OP_ENCODE:
        case BST_OP_CALC_SIZE:
            count() = num_items;
            if (!count.bst_op(str))
                return false;
            for (i=0; i < num_items; i++)
                if (!array[i]->bst_op(str))
                    return false;
            return true;
        case BST_OP_DECODE:
            if (!count.bst_op(str))
                return false;
            resize(count());
            for (i=0; i < num_items; i++)
                if (!array[i]->bst_op(str))
                    return false;
            return true;
        case BST_OP_MEM_FREE:
            for (i=0; i < num_items; i++)
                if (!array[i]->bst_op(str))
                    return false;
            bst_var_array_free();
            return true;
        }
        return false;
    }
};

// base class for an object with a union in it.
#include <stdio.h>
#include <stdlib.h>

class BST_UNION : public BST {
    int max;
    BST ** fields;
    void flatten(void) {
        if (fields)
            return;
        fields = new BST*[max];
        BST * b; int i=0;
        for (b = head; b; b = b->next) {
            if (i >= max) {
            enum_mismatch:
                fprintf(stderr, "\nERROR IN UNION: enum mismatch!!\n\n");
                exit(1);
            }
            fields[i++] = b;
        }
        if (i != max) goto enum_mismatch;
    }
public:
    BST_UINT8_t  which;
    BST_UNION( BST *parent, int _max )
        : BST(parent), which(NULL) {
        max = _max; which() = max; fields = NULL;
    }
    virtual ~BST_UNION(void) { delete[] fields; }
    /*virtual*/ bool bst_op( BST_STREAM *str ) {
        flatten();
        if (!which.bst_op(str))
            return false;
        if (which() >= max)
            return false;
        BST_OP op = str->get_op();
        if (op != BST_OP_MEM_FREE)
        {
            if (!fields[which()]->bst_op(str))
                return false;
        }
        else
        {
            // op free should free all fields
            for (BST * b = head; b; b = b->next)
                b->bst_op(str);
            which() = max;
        }
        return true;
    }
};

// these are types for swapbyting in place.

class UINT64_t {
private:
    static const int BYTES = 8;
    typedef uint64_t __T;
    uint8_t p[BYTES];
public:
    UINT64_t( void ) { }
    UINT64_t( __T x ) { set( x ); }
    __T get(void) {
            return
                ((__T)p[0] << 56) + ((__T)p[1] << 48) + 
                ((__T)p[2] << 40) + ((__T)p[3] << 32) + 
                ((__T)p[4] << 24) + ((__T)p[5] << 16) + 
                ((__T)p[6] <<  8) + ((__T)p[7] <<  0);
    }
    void set( __T x ) {
            p[0] = (x >> 56) & 0xff;  p[1] = (x >> 48) & 0xff;
            p[2] = (x >> 40) & 0xff;  p[3] = (x >> 32) & 0xff;
            p[4] = (x >> 24) & 0xff;  p[5] = (x >> 16) & 0xff;
            p[6] = (x >>  8) & 0xff;  p[7] = (x >>  0) & 0xff;
    }
    int size_of(void) { return BYTES; }
    __T incr(__T v=1) { __T n = get() + v; set(n); return n; }
    __T decr(__T v=1) { __T n = get() - v; set(n); return n; }
    // specific to UINT64
    uint32_t get_lo(void) {
        return
            ((uint32_t)p[4] << 24) + ((uint32_t)p[5] << 16) + 
            ((uint32_t)p[6] <<  8) + ((uint32_t)p[7] <<  0);
    }
    uint32_t get_hi(void) {
        return
            ((uint32_t)p[0] << 24) + ((uint32_t)p[1] << 16) + 
            ((uint32_t)p[2] <<  8) + ((uint32_t)p[3] <<  0);
    }
};

class UINT32_t {
private:
    static const int BYTES = 4;
    typedef uint32_t __T;
    uint8_t p[BYTES];
public:
    UINT32_t( void ) { }
    UINT32_t( __T x ) { set( x ); }
    __T get(void) {
            return
                ((__T)p[0] << 24) + ((__T)p[1] << 16) + 
                ((__T)p[2] <<  8) + ((__T)p[3] <<  0);
    }
    void set( __T x ) {
            p[0] = (x >> 24) & 0xff;  p[1] = (x >> 16) & 0xff;
            p[2] = (x >>  8) & 0xff;  p[3] = (x >>  0) & 0xff;
    }
    int size_of(void) { return BYTES; }
    __T incr(__T v=1) { __T n = get() + v; set(n); return n; }
    __T decr(__T v=1) { __T n = get() - v; set(n); return n; }
};

class UINT16_t {
private:
    static const int BYTES = 2;
    typedef uint16_t __T;
    uint8_t p[BYTES];
public:
    UINT16_t( void ) { }
    UINT16_t( __T x ) { set( x ); }
    __T get(void) {
            return
                ((__T)p[0] <<  8) + ((__T)p[1] <<  0);
    }
    void set( __T x ) {
            p[0] = (x >>  8) & 0xff;  p[1] = (x >>  0) & 0xff;
    }
    int size_of(void) { return BYTES; }
    __T incr(__T v=1) { __T n = get() + v; set(n); return n; }
    __T decr(__T v=1) { __T n = get() - v; set(n); return n; }
};

// useless class exists for consistency's sake.
class UINT8_t {
private:
    static const int BYTES = 1;
    typedef uint16_t __T;
    uint8_t p[BYTES];
public:
    UINT8_t( void ) { }
    UINT8_t( __T x ) { set( x ); }
    __T get(void) {
            return ((__T)p[0] <<  0);
    }
    void set( __T x ) {
            p[0] = (x >>  0) & 0xff;
    }
    int size_of(void) { return BYTES; }
    __T incr(__T v=1) { __T n = get() + v; set(n); return n; }
    __T decr(__T v=1) { __T n = get() - v; set(n); return n; }
};





/* examples:


struct myUnion : public BST_UNION {
    enum { AYE, BEE, CEE, DEE, MAX };
    myUnion(BST *parent) :
        BST_UNION(parent,MAX),
        a(this), b(this), c(this), d(this) { }
    BST_UINT64_t  a;
    BST_UINT32_t  b;
    BST_UINT16_t  c;
    BST_UINT8_t   d;
};

struct myStruct1 : public BST {
    myStruct1(BST * parent) :
        BST(parent),
        one(this), two(this) { }
    BST_UINT32_t  one;
    BST_UINT32_t  two;
};

struct myStruct2 : public BST {
    myStruct2(BST *parent)
        : BST(parent),
          thing(this), three(this), un(this) {}
    myStruct1     thing;
    BST_UINT32_t  three;
    myUnion     un;
};

int main() {
    myStruct2  x(NULL);
    myStruct2  y(NULL);
    uint8_t * buffer;
    int len;

    x.thing.one() = 4;
    x.thing.two() = 8;
    x.three() = 12;
    x.un.which() = myUnion::CEE;
    x.un.c() = 127;
    buffer = x.bst_encode(&len);
    if (!buffer) {
        printf("encode error\n");
        return 1;
    }
    for (int i=0; i < len; i++)
        printf("%02x ", buffer[i]);
    printf("\n");
    if (!y.bst_decode(buffer,len)) {
        printf("decode error\n");
        return 1;
    }
    printf("%d %d %d %d ", x.thing.one(), x.thing.two(),
           x.three(), x.un.which());
    switch (x.un.which()) {
    case myUnion::AYE:  printf("a=%lld\n", x.un.a()); break;
    case myUnion::BEE:  printf("b=%d\n"  , x.un.b()); break;
    case myUnion::CEE:  printf("c=%d\n"  , x.un.c()); break;
    case myUnion::DEE:  printf("d=%d\n"  , x.un.d()); break;
    }
    delete[] buffer;
    return 0;
}

 */

#endif /* __BYTESTREAM_H__ */
