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
#include <unistd.h>

#include "shmempipe.h"
#include "shmempipe_test_msg.h"

bool connected = false;
int count=0;
bool done = false;

void connect(shmempipe *pPipe, void *arg)
{
    printf("got connect\n");
    connected = true;
}

void disconnect(shmempipe *pPipe, void *arg)
{
    printf("got disconnect\n");
    connected = false;
}

void message(shmempipe * pPipe, void *arg, shmempipeMessage * _pMsg)
{
    MyTestMsg * pMsg = (MyTestMsg *) _pMsg;
    pPipe->release(pMsg);
    if (!done)
    {
        pMsg = MyTestMsg::allocSize(pPipe);
        if (pMsg)
        {
            pMsg->seqno = 8;
            pPipe->send(pMsg);
        }
    }
    count++;
    if (count >= 10000000)
        done=true;
}

int
main()
{
    shmempipeSlaveConfig CONFIG;
    shmempipe * pPipe;

    CONFIG.setPipeName( "shmempipe_test" );
    CONFIG.connectCallback = &connect;
    CONFIG.disconnectCallback = &disconnect;
    CONFIG.messageCallback = &message;
    CONFIG.arg = NULL;
    pPipe = new shmempipe( &CONFIG );
    if (!CONFIG.bInitialized)
    {
        printf("error constructing shmempipe\n");
        return 1;
    }

    while (!connected && !done)
    {
        pxfe_select sel;
        sel.rfds.set(0);
        sel.tv.set(0,100000);
        if (sel.select() <= 0)
            continue;
        if (sel.rfds.isset(0))
            done = true;
    }

    if (!done)
        for (int counter=0; counter < 50; counter++)
        {
            MyTestMsg * pMsg = MyTestMsg::allocSize(pPipe);
            pMsg->seqno = 1;
            pPipe->send(pMsg);
        }

    while (connected && !done)
    {
        shmempipeStats stats;
        pPipe->getStats(&stats,true);
        printf("sb %lld sp %lld ss %lld rb %lld rp %lld rs %lld "
               "af %lld fb %lld\n",
               stats.sent_bytes, stats.sent_packets, stats.sent_signals,
               stats.rcvd_bytes, stats.rcvd_packets, stats.rcvd_signals,
               stats.alloc_fails, stats.free_buffers);

        pxfe_select sel;
        sel.rfds.set(0);
        sel.tv.set(0,100000);
        if (sel.select() <= 0)
            continue;
        if (sel.rfds.isset(0))
            done = true;
    }

    delete pPipe;
    return 0;
}
