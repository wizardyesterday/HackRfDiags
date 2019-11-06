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

/*
 * wow, this is really gross.
 * i don't know why but if you build crunched_utils for solaris 5.6
 * and then run the binary on 5.5.1 it doesn't work because it claims
 * 'vsnprintf' is undefined. but i can't figure out where vsnprintf
 * is being referenced from. i suspect libcurses because i can't figure
 * out where the linker is getting libcurses from, and 'he' doesn't work
 * without a properly functioning vsnprintf.
 */

#include <stdarg.h>
#include <stdio.h>

#define ARBITRARY_MAX_BUF 16384

int
vsnprintf( char *str, size_t size, const char *format, va_list ap )
{
    char * temp = (char*)malloc( ARBITRARY_MAX_BUF );
    int ret;

    /* don't depend on return value of vsprintf. */
    /* on sunos it is a char* instead of an int. */
    vsprintf( temp, format, ap );
    ret = strlen( temp );

    if ( ret > size )
        ret = size-1;

    memcpy( str, temp, ret );
    free( temp );
    str[ret] = 0;

    return ret;
}

int
snprintf( char *str, size_t size, const char *format, ... )
{
    int ret;
    va_list ap;

    va_start( ap, format );
    ret = vsnprintf( str, size, format, ap );
    va_end( ap );

    return ret;
}
