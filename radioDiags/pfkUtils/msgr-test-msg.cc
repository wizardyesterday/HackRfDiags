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

#include "msgr.h"
#include <stdio.h>

int
main()
{
    uint8_t  bigbuf[0x100];
    uint32_t bigbuflen = 0;
    uint32_t len;
    uint32_t consumed;
    uint8_t * ptr;
    PfkMsg  m;

//msg one
    m.init(0x0103, bigbuf + bigbuflen, sizeof(bigbuf) - bigbuflen);
    m.add_val((uint8_t)  1);
    m.add_val((uint16_t) 2);
    m.add_val((uint32_t) 3);
    m.add_val((uint64_t) 4);
    bigbuflen += m.finish();

//msg two
    m.init(0x0107, bigbuf + bigbuflen, sizeof(bigbuf) - bigbuflen);
    m.add_val((uint16_t) 0xfedc);
    m.add_val((uint16_t) 0x1234);
    m.add_val((uint32_t) 0x56788765);
    bigbuflen += m.finish();

//msg three
    m.init(0x010f, bigbuf + bigbuflen, sizeof(bigbuf) - bigbuflen);
    m.add_val((uint8_t) 0xdc);
    m.add_field(16, (void*)"this is a test");
    bigbuflen += m.finish();

//debug dump
    printf("encoded: ");
    for (uint32_t i = 0; i < bigbuflen; i++)
        printf("%02x ", bigbuf[i]);
    printf("\n");

//init for decode
    ptr = bigbuf;
    len = bigbuflen;

    while (len > 0)
    {
        if (m.parse(ptr, len, &consumed) == true)
        {
            printf("\ngot msg with %u fields\n", m.get_num_fields());
            for (uint32_t fld = 0; fld < m.get_num_fields(); fld++)
            {
                uint16_t fldlen;
                uint8_t * f = (uint8_t *) m.get_field(&fldlen, fld);
                if (f == NULL)
                    printf("can't fetch field %u\n", fld);
                else {
                    printf("field %u: ", fld);
                    for (uint32_t fldpos = 0; fldpos < fldlen; fldpos++)
                        printf("%02x ", f[fldpos]);
                    printf("\n");
                }
            }
        }
        else
            printf("parse returned false\n");
        len -= consumed;
        ptr += consumed;
        printf("consumed = %d remaining len = %d\n", consumed, len);
    }

    printf("\n");

    return 0;
}
