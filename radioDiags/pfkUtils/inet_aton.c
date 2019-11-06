
#include "pfkutils_config.h"

#ifndef HAVE_INET_ATON
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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <ctype.h>

int
inet_aton( register const char *cp, struct in_addr *addr )
{
    register unsigned int val;
    register int base, n;
    register char c;
    u_int parts[4];
    register u_int *pp = parts;

    c = *cp;
    for (;;)
    {
        /*
         * Collect number up to ``.''.
         * Values are specified as for C:
         * 0x=hex, 0=octal, isdigit=decimal.
         */
        if (!isdigit((int)c))
            return (0);
        val = 0; base = 10;
        if (c == '0')
        {
            c = *++cp;
            if (c == 'x' || c == 'X')
                base = 16, c = *++cp;
            else
                base = 8;
        }
        for (;;)
        {
            if (isascii((int)c) && isdigit((int)c))
            {
                val = (val * base) + (c - '0');
                c = *++cp;
            }
            else if (base == 16 && isascii((int)c) && isxdigit((int)c))
            {
                val = (val << 4) |
                    (c + 10 - (islower((int)c) ? 'a' : 'A'));
                c = *++cp;
            }
            else
                break;
        }
        if (c == '.')
        {
            /*
             * Internet format:
             *      a.b.c.d
             *      a.b.c   (with c treated as 16 bits)
             *      a.b     (with b treated as 24 bits)
             */
            if (pp >= parts + 3)
                return (0);
            *pp++ = val;
            c = *++cp;
        }
        else
            break;
    }
    /*
     * Check for trailing characters.
     */
    if (c != '\0' && (!isascii((int)c) || !isspace((int)c)))
        return (0);
    /*
     * Concoct the address according to
     * the number of parts specified.
     */
    n = pp - parts + 1;
    switch (n)
    {
    case 0:
        return (0);     /* initial nondigit */

    case 1:         /* a -- 32 bits */
        break;

    case 2:         /* a.b -- 8.24 bits */
        if (val > 0xffffff)
            return (0);
        val |= parts[0] << 24;
        break;

    case 3:         /* a.b.c -- 8.8.16 bits */
        if (val > 0xffff)
            return (0);
        val |= (parts[0] << 24) | (parts[1] << 16);
        break;

    case 4:         /* a.b.c.d -- 8.8.8.8 bits */
        if (val > 0xff)
            return (0);
        val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
        break;
    }

    if (addr)
        addr->s_addr = htonl(val);

    return (1);
}
#endif
