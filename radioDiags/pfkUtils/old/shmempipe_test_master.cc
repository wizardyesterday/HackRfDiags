#if 0
set -e -x
g++ -Wall -O6 -c shmempipe.cc
g++ -Wall -O6 -c shmempipe_test_master.cc
g++ -Wall -O6 -c shmempipe_test_slave.cc
g++ shmempipe_test_master.o shmempipe.o -o tm -lpthread
g++ shmempipe_test_slave.o shmempipe.o -o ts -lpthread
exit 0
#endif

#include <stdio.h>
#include <unistd.h>

#include "shmempipe.H"
#include "shmempipe_test_msg.H"

class myHandler : public shmempipeHandler {
public:
    bool done;
    time_t last_print;
    time_t start;
#ifdef SHMEMPIPE_STATS
    shmempipeStats stats;
#endif
    myHandler(void) { 
        done = false;
        last_print = start = 0;
    }
    /*virtual*/ void messageHandler(shmempipeMessage * _pMsg) {
        MyTestMsg * pMsg = (MyTestMsg *) _pMsg;
        if (pMsg->messageSize > 0)
        {
#ifdef SHMEMPIPE_STATS
            time_t now = time(0);
            int delta = (int)(now - pInfo->start);
            if (now != pInfo->last_print)
            {
                pInfo->pPipe->getStats(&pInfo->stats, true);
                
                fprintf(stderr,
                        " sb %ld sp %ld rb %ld rp %ld"
                        " sm %ld rm %ld br %ld bf %ld aw %ld           \r",
                        pInfo->stats.sent_bytes,
                        pInfo->stats.sent_packets,
                        pInfo->stats.rcvd_bytes,
                        pInfo->stats.rcvd_packets,
                        pInfo->stats.sent_messages,
                        pInfo->stats.rcvd_messages,
                        pInfo->stats.buffers_read,
                        pInfo->stats.buffers_flushed,
                        pInfo->stats.alloc_waits );
                pInfo->last_print = now;
            }
#endif
            write(1, pMsg->data, pMsg->messageSize);
            pPipe->release(pMsg,false);
        }
        else
        {
            // send it back as an ack.
            pPipe->send(pMsg,true);
        }
    }
    /*virtual*/ void connectHandler(void) {
        printf("connected\n");
    }
    /*virtual*/ void disconnectHandler(void) {
        printf("disconnected\n");
        done = true;
    }
};

int
main()
{
    myHandler handler;
    shmempipeMasterConfig  CONFIG;
    shmempipe * pPipe;
    CONFIG.setPipeName( "shmempipe_test" );
    CONFIG.flushUsecs = 10000;
    CONFIG.pHandler = &handler;
    CONFIG.master2slave.addPool(5000, sizeof(MyTestMsg));
    CONFIG.slave2master.addPool(5000, sizeof(MyTestMsg));
    pPipe = new shmempipe( &CONFIG );
    if (!CONFIG.bInitialized)
    {
        printf("error constructing shmempipe\n");
        return 1;
    }
    while (!handler.done)
        usleep(100000);
    delete pPipe;
    return 0;
}
