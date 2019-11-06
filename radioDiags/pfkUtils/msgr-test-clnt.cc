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

#include <iostream>
#include <iomanip>
#include <unistd.h>

using namespace std;

class MyClient : public PfkMsgr {
    /*virtual*/ void handle_connected(void)
    {
        cout << "connected\n";
    }
    /*virtual*/ void handle_disconnected(void)
    {
        cout << "disconnected\n";
    }
    /*virtual*/ void handle_msg(const PfkMsg &msg)
    {
        if (0)
        {
            cout << "got msg:\n";
            for (uint32_t fld = 0; fld < msg.get_num_fields(); fld++)
            {
                uint16_t fldlen;
                uint8_t * f = (uint8_t *) msg.get_field(&fldlen, fld);
                if (f == NULL)
                    cout << "can't fetch field " << fld << endl;
                else {
                    cout << "field " << fld << ": ";
                    for (uint32_t fldpos = 0; fldpos < fldlen; fldpos++)
                        cout << hex << setw(2) << setfill('0')
                             << (int) f[fldpos] << " ";
                    cout << endl;
                }
            }
        }
        else
            cerr << "r ";
    }
public:
    MyClient(void) : PfkMsgr("127.0.0.1", 4000)
    {
    }
    ~MyClient(void)
    {
    }
};

int
main()
{
    MyClient  client;

    while (1)
    {
        uint8_t  buf[0x100];
        PfkMsg m;
        m.init(0x0104, buf, sizeof(buf));
        m.add_val((uint16_t) 1234);
        m.add_field(16, (void*)"this is a test");
        m.finish();
        if (client.send_msg(m))
            cerr << "s ";
        usleep(10000);
    }

    return 0;
}
