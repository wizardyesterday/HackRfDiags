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

#include <stdio.h>
#include <stdlib.h>

#include "MemoryManager.H"
#include "dll2.H"

struct MemoryBlock {
    LListLinks <MemoryBlock>  links[1];
    char *file;
    int line;
    char *function;
    size_t size;
    char buf[0];
    MemoryBlock(size_t _size, char *_file, int _line, char *_function) {
        size = _size;
        file = _file;
        line = _line;
        function = _function;
    }
    void * buffer(void) { return (void*) buf; }
    static MemoryBlock * ptrToBlock(void * _ptr) {
        MemoryBlock * b = (MemoryBlock *) _ptr;
        char * tmp = (char*) _ptr;
        tmp -=
            sizeof(b->links) + sizeof(b->size) + sizeof(b->file) +
            sizeof(b->line) + sizeof(b->function);
        b = (MemoryBlock *) tmp;
        return b;
    }
    void * operator new(size_t sz, size_t realsz) {
        return (void*) ::malloc(sz + realsz);
    }
    void operator delete(void * ptr) {
        ::free(ptr);
    }
};

class MemoryManager {
    LList <MemoryBlock, 0> list;
    int alloc_count;
    int free_count;
    int used_bytes;
    int used_peak;
public:
    MemoryManager(void) {
        alloc_count = free_count = used_bytes = used_peak = 0;
    }
    ~MemoryManager(void) {
        printf("MemoryManager results: \n"
               "alloc %d, free %d, used_p %d, used_b %d, used_peak %d\n",
               alloc_count, free_count, 
               list.get_cnt(),
               used_bytes, used_peak);
        MemoryBlock * b;
        for (b=list.get_head(); b; b=list.get_next(b))
            printf("%d bytes at %#x: %s:%d:%s\n",
                   b->size, (int)b->buf, 
                   b->file, b->line, b->function);
    }
    void * alloc(size_t sz, char *file, int line, char *function) {
        MemoryBlock * b = new(sz) MemoryBlock(sz,file,line,function);
        list.add(b);
        alloc_count++;
        used_bytes += sz;
        if (used_bytes > used_peak)
            used_peak = used_bytes;
        return b->buffer();
    }
    void free(void * ptr) {
        MemoryBlock * b = MemoryBlock::ptrToBlock(ptr);
        list.remove(b);
        free_count++;
        used_bytes -= b->size;
        delete b;
    }
};

static  MemoryManager   mgr;

void *
operator new( size_t sz )
{
    return mgr.alloc(sz,(char*)__FILE__,__LINE__,(char*)__FUNCTION__);
}

void *
operator new[]( size_t sz )
{
    return mgr.alloc(sz,(char*)__FILE__,__LINE__,(char*)__FUNCTION__);
}

void *
operator new(size_t sz, const char * file, int line, const char * function)
{
    return mgr.alloc(sz,(char*)file,line,(char*)function);
}

void *
operator new[](size_t sz, const char * file, int line, const char * function)
{
    return mgr.alloc(sz,(char*)file,line,(char*)function);
}

void
operator delete( void * ptr )
{
    mgr.free(ptr);
}

void
operator delete[]( void * ptr )
{
    mgr.free(ptr);
}
