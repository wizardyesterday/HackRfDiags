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

#if 1
#define LOGNEW new
#define MALLOC(size) malloc( size )
#define STRDUP(str) strdup( str )
#else

#ifdef __cplusplus

void * operator new     ( size_t s, char * file, int line );
void * operator new[]   ( size_t s, char * file, int line );

#define LOGNEW new(__FILE__,__LINE__)

extern "C" {

#endif

void * malloc_record( char * file, int line, int size );
char * strdup_record( char * file, int line, char *str );
#define MALLOC(size) malloc_record( __FILE__, __LINE__, size )
#define STRDUP(str) strdup_record( __FILE__, __LINE__, str )

#ifdef __cplusplus
};
#endif
#endif /* if 0 */
