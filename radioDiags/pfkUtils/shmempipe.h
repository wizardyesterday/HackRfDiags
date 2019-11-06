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

#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include "pfkutils_config.h"
#include "posix_fe.h"

/** information about what pools should be included in the shared mem */
struct shmempipePoolInfo {
    /** limit to how many unique pools can be created */
    static const int maxPools = 64;
    uint8_t numPools;
    uint32_t bufSizes[maxPools]; // specify in increasing order.
    uint32_t numBufs[maxPools];
    inline shmempipePoolInfo(void);
    /** helper function to add a pool.
     * \param numBuf  the number of buffers to add
     * \param bufSize  the size of each buffer to add
     * \return true of okay, or false if failure (i.e. reached max) */
    inline bool addPool(uint32_t numBuf, uint32_t bufSize);
};

#define SHMEMPIPE_FILEPATH_FMT "/tmp/p2p.%s.shmem"
#define SHMEMPIPE_M2SPATH_FMT "/tmp/p2p.%s.m2s"
#define SHMEMPIPE_S2MPATH_FMT "/tmp/p2p.%s.s2m"

/** information about where the memory and pipe files are put */
struct shmempipeFilename {
    /** max length of a full path */
    static const int FILENAMEMAX = 512;
    char filename[FILENAMEMAX];
    char m2sname[FILENAMEMAX];
    char s2mname[FILENAMEMAX];
    inline shmempipeFilename(void);
    /** helper function to set path, i.e. "MYPIPENAME" */
    inline void setPipeName(const char * _pipename);
};

class shmempipe;
struct shmempipeMessage;

/** a user's function for handling connection and disconnection
 * should follow this signature. */
typedef void (*shmempipeConnectionHandlerFunc)(shmempipe *, void *arg);
/** a user's function for handling a message should follow this
 * signature. */
typedef void (*shmempipeMessageHandlerFunc)(shmempipe *, void *arg,
                                         shmempipeMessage * pMsg);

/** information about all registered callbacks: connection,
 * disconnection, and message arrival, and extra data to pass along */
struct shmempipeCallbacks {
    /** pointer to user's connection callback goes here */
    shmempipeConnectionHandlerFunc connectCallback;
    /** pointer to user's disconnection callback goes here */
    shmempipeConnectionHandlerFunc disconnectCallback;
    /** pointer to user's message handler function goes here */
    shmempipeMessageHandlerFunc messageCallback;
    /** any data the user wants passed to the callback goes here */
    void * arg;
};

/** master configuration object, create one of these and construct
 * a shmempipe to create a pipe. */
struct shmempipeMasterConfig :
    public shmempipeFilename, shmempipeCallbacks, shmempipePoolInfo
{
    /** check this value after shmempipe constructor has returned to
     * see if the pipe construction was successful. */
    bool bInitialized;
};

/** slave configuration object, create one of these and construct
 * a shmempipe to attach to an existing pipe. */
struct shmempipeSlaveConfig :
    public shmempipeFilename, shmempipeCallbacks
{
    /** check this value to see if constructor succeeded. */
    bool bInitialized;
};

/** base class for all user messages, no user-servicable parts inside. */
struct shmempipeMessage {
    friend class shmempipeBufferList;
private:
    uint32_t next; // offset
    uint8_t bMaster;
    uint8_t poolInd;
    uint32_t bufInd;
    uint32_t bufferSize;
public:
    uint32_t messageSize;
private:
    enum { FREE=1, APP, QUEUE } owner;
    friend class shmempipe;
};

/** statistics that you can retrieve. */
struct shmempipeStats {
    uint64_t sent_bytes;       //!< how many bytes sent
    uint64_t sent_packets;     //!< how many packet sent
    uint64_t sent_signals;     //!< how many wakeup signals sent
    uint64_t rcvd_bytes;       //!< how many bytes of data received
    uint64_t rcvd_packets;     //!< how many packets received
    uint64_t rcvd_signals;     //!< how many wakeup signals received
    uint64_t alloc_fails;      //!< how many times has alloc failed
    uint64_t free_buffers;     //!< number of free buffers in pools
    inline shmempipeStats(void);
    inline void init(void);
};

class shmempipeBufferList {
private:
    volatile uint32_t head; // dequeue from head
    volatile uint32_t tail; // enqueue to tail
    volatile uint32_t count;
    // shmempipeMessage::next points towards tail
    pthread_mutex_t mutex;
    pthread_cond_t  empty_cond;
    volatile bool needspoke;
    volatile bool bIsWaiting;
    inline bool   lock(void);
    inline void unlock(void);
public:
    void init(void);
    void cleanup(void);
    inline bool empty(void);
    inline void poke(void);
    inline uint32_t get_count(void);
    // the only time needlock=false is at init time when
    // building the freelists initially.
    inline bool enqueue( uintptr_t base, shmempipeMessage * msg,
                         bool *pSignalSent, bool debug,
                         bool needlock=true );
    inline shmempipeMessage * dequeue(uintptr_t base,
                                      bool *pSignalled,
                                      bool bWait, bool debug);
};

struct shmempipeHeader {
    shmempipePoolInfo    poolInfo;
    shmempipeBufferList  pools[shmempipePoolInfo::maxPools];
    shmempipeBufferList  master2slave;
    shmempipeBufferList  slave2master;
    bool attachedFlag;
    // pool buffers start here
};

/** the actual pipe itself */
class shmempipe {
    bool m_bMaster;
    bool m_bConnected;
    bool m_bufferListsInitialized;
    shmempipeFilename m_filename;
    shmempipeCallbacks m_callbacks;
    shmempipeStats m_stats;
    pthread_mutex_t m_statsMutex;
    inline void   lockStats(void);
    inline void unlockStats(void);
    int m_shmemFd;
    int m_myPipeFd;
    int m_otherPipeFd;
    pxfe_pipe m_closerPipe;
    uint32_t m_fileSize;
    uintptr_t m_shmemPtr;
    uintptr_t m_shmemLimit;
    shmempipeHeader * m_pHeader;
    shmempipeBufferList * m_myBufferList;
    shmempipeBufferList * m_otherBufferList;

    enum {
        CLOSER_NOT_EXIST,
        CLOSER_RUNNING,
        CLOSER_EXITING,
        CLOSER_DEAD
    } m_closerState;
    pthread_t m_closerId;
    void startCloserThread(void);
    void stopCloserThread(void);
    static void * _closerThreadEntry(void *arg);
    void closerThread(void);

    bool m_bReaderRunning;
    bool m_bReaderStop;
    void startReaderThread(void);
    void stopReaderThread(void);
    static void * _readerThreadEntry(void *arg);
    void readerThread(void);

public:
    /** constructor that creates a pipe */
    shmempipe( shmempipeMasterConfig * pConfig );
    /** constructor that attaches to an existing pipe */
    shmempipe( shmempipeSlaveConfig * pConfig );
    ~shmempipe( void );
    /** allocate a message buffer from pools by specifying size you want.
     * \param size   the size of buffer you want
     * \param bWait  if no buffers available, should i wait or return NULL?
     * \return a message pointer or NULL if no buffers available. */
    inline shmempipeMessage * allocSize(uint32_t size, bool bWait=false);
    /** allocate a message buffer from pools by specifying which pool.
     * \param poolInd   the id of the pool you want.
     * \param bWait  if no buffers available, should i wait or return NULL?
     * \return a message pointer or NULL if no buffers available. */
    inline shmempipeMessage * allocPool(uint32_t poolInd, bool bWait=false);
    /** release a buffer back to the pipe's pools.
     * \param pBuf pointer to the buffer
     * \return false if something screwed up */
    inline bool release(shmempipeMessage * pBuf);
    /** send a buffer through to the peer.
     * \param pBuf pointer to the buffer
     * \return false if something screwed up */
    inline bool send(shmempipeMessage * pBuf);
    /** fetch stats about performance of this buffer.
     * \param pStats pointer to user's object to fill out
     * \param zero should i reset stats after collecting? */
    void getStats(shmempipeStats * pStats, bool zero=false);
};

// inline implementations below

inline
shmempipePoolInfo :: shmempipePoolInfo(void)
{
    numPools = 0;
}

inline
bool shmempipePoolInfo :: addPool(uint32_t numBuf, uint32_t bufSize)
{
    if (numPools == maxPools)
        return false;
    bufSizes[numPools] = bufSize;
    numBufs[numPools] = numBuf;
    numPools++;
    return true;
}

inline
shmempipeFilename :: shmempipeFilename(void)
{
    filename[0] = 0;
    m2sname[0] = 0;
    s2mname[0] = 0;
}

inline void
shmempipeFilename :: setPipeName(const char * _pipename)
{
    snprintf(filename,sizeof(filename),
             SHMEMPIPE_FILEPATH_FMT, _pipename);
    filename[FILENAMEMAX-1] = 0;
    snprintf(m2sname,sizeof(m2sname),
             SHMEMPIPE_M2SPATH_FMT, _pipename);
    m2sname[FILENAMEMAX-1] = 0;
    snprintf(s2mname,sizeof(s2mname),
             SHMEMPIPE_S2MPATH_FMT, _pipename);
    s2mname[FILENAMEMAX-1] = 0;
}

inline void
shmempipeStats :: init(void)
{
    memset(this, 0, sizeof(*this));
}

inline
shmempipeStats :: shmempipeStats(void)
{
    init();
}

inline bool
shmempipeBufferList :: lock(void)
{
    int ret;
    ret = pthread_mutex_lock( &mutex );
    if (ret < 0)
    {
#if HAVE_PTHREAD_MUTEX_CONSISTENT_NP
        if (ret == EOWNERDEAD)
        {
            pthread_mutex_consistent_np(&mutex);
            pthread_mutex_unlock( &mutex );
        }
#endif
        return false;
    }
    return true;
}

inline void
shmempipeBufferList :: unlock(void)
{
    pthread_mutex_unlock( &mutex );
}

inline bool
shmempipeBufferList :: empty(void)
{
    return head == 0;
}

inline void
shmempipeBufferList :: poke(void)
{
    lock();
    needspoke = true;
    unlock();
    pthread_cond_signal( &empty_cond );
}

inline uint32_t
shmempipeBufferList :: get_count(void)
{
    return count;
}

inline bool
shmempipeBufferList :: enqueue( uintptr_t base,
                               shmempipeMessage * msg,
                               bool *pSignalSent,
                               bool debug,
                               bool needlock /*=true*/ )
{
    uint32_t msgoff = ((uintptr_t)msg) - base;
    bool signal = false;
    msg->next = 0;
    if (needlock)
        if (lock() == false)
            return false;
    if (tail != 0)
    {
        shmempipeMessage * tailmsg = (shmempipeMessage *)(tail + base);
        tailmsg->next = msgoff;
        tail = msgoff;
    }
    else
    {
        head = tail = msgoff;
    }
    signal = bIsWaiting;
    bIsWaiting = false;
    count++;
    if (needlock)
        unlock();
    if (signal)
    {
        pthread_cond_broadcast(&empty_cond);
        if (pSignalSent)
            *pSignalSent = true;
    }
    return true;
}

inline shmempipeMessage *
shmempipeBufferList :: dequeue(uintptr_t base, bool *pSignalled,
                              bool bWait, bool debug)
{
    uint32_t msgoff;
    shmempipeMessage * pMsg = NULL;
    if (lock() == false)
        return NULL;
    if (head == 0 && !needspoke)
    {
        if (bWait)
        {
            bIsWaiting = true;
            pthread_cond_wait(&empty_cond, &mutex);
            bIsWaiting = false;
            if (pSignalled)
                *pSignalled = true;
        }
    }
    if (head != 0 && !needspoke)
    {
        msgoff = head;
        pMsg = (shmempipeMessage *)(msgoff + base);
        head = pMsg->next;
        if (head == 0)
            tail = 0;
        count--;
    }
    if (pMsg == NULL)
        needspoke = false;
    unlock();
    return pMsg;
}

inline void
shmempipe :: lockStats(void)
{
    pthread_mutex_lock  ( &m_statsMutex );
}

inline void
shmempipe :: unlockStats(void)
{
    pthread_mutex_unlock( &m_statsMutex );
}

inline shmempipeMessage *
shmempipe :: allocSize(uint32_t size, bool bWait /*=false */)
{
    int poolInd;
    for (poolInd = 0; poolInd < m_pHeader->poolInfo.numPools; poolInd++)
    {
        if (m_pHeader->poolInfo.bufSizes[poolInd] >= size)
            break;
    }
    if (poolInd == m_pHeader->poolInfo.numPools)
    {
        lockStats();
        m_stats.alloc_fails++;
        unlockStats();
        return NULL;
    }
    shmempipeMessage * ret = 
        m_pHeader->pools[poolInd].dequeue(m_shmemPtr, NULL, bWait, false);
    if (ret == NULL)
    {
        lockStats();
        m_stats.alloc_fails++;
        unlockStats();
    }
    else
        ret->poolInd = poolInd;
    return ret;
}

inline shmempipeMessage *
shmempipe :: allocPool(uint32_t poolInd, bool bWait /*=false*/ )
{
    if (poolInd < 0 || poolInd >= m_pHeader->poolInfo.numPools)
    {
        lockStats();
        m_stats.alloc_fails++;
        unlockStats();
        return NULL;
    }
    shmempipeMessage * ret = 
        m_pHeader->pools[poolInd].dequeue(m_shmemPtr, NULL, bWait, false);
    if (ret == NULL)
    {
        lockStats();
        m_stats.alloc_fails++;
        unlockStats();
    }
    else
        ret->poolInd = poolInd;
    return ret;
}

inline bool
shmempipe :: release(shmempipeMessage * pBuf)
{
    return 
        m_pHeader->pools[pBuf->poolInd].enqueue(m_shmemPtr,pBuf,NULL,false);
}

inline bool
shmempipe :: send(shmempipeMessage * pBuf)
{
    if (m_bConnected == false)
        return false;
    bool signalSent = false;
    if (m_otherBufferList->enqueue(m_shmemPtr,pBuf,&signalSent,true) == false)
        return false;
    lockStats();
    m_stats.sent_bytes += pBuf->messageSize;
    m_stats.sent_packets ++;
    if (signalSent)
        m_stats.sent_signals ++;
    unlockStats();
    return true;
}

/** \mainpage shmempipe API user's manual

This is the user's manual for the shmempipe API.

Interesting data structures are:

<ul>
<li> \ref shmempipeMessage
<li> \ref shmempipeMasterConfig  (contains \ref shmempipeFilename,
      \ref shmempipeCallbacks, \ref shmempipePoolInfo)
<li> \ref shmempipeSlaveConfig (contains \ref shmempipeFilename,
      \ref shmempipeCallbacks)
<li> \ref shmempipe
</ul>

sample code

\code

#define SHMEMFILE "SHMEMPIPE"
#define FIFOMASTERSLAVE "FIFOMASTERSLAVE"
#define FIFOSLAVEMASTER "FIFOSLAVEMASTER"

struct MyTestMsg : public shmempipeMessage
{
    static MyTestMsg * allocSize(shmempipe * pPipe) {
        MyTestMsg * ret;
        ret = (MyTestMsg *) pPipe->allocSize(sizeof(MyTestMsg), -1);
        ret->messageSize = sizeof(MyTestMsg);
        return ret;
    }
    static const int max_size = 184;
    int seqno;
    char data[max_size];
};

\endcode

\code

bool connected = false;
int count = 0;

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
    pMsg = MyTestMsg::allocSize(pPipe);
    if (pMsg)
    {
        pMsg->seqno = 8;
        pPipe->send(pMsg);
    }
    count ++;
}
int
main()
{
    shmempipeMasterConfig  CONFIG;
    shmempipe * pPipe;
    CONFIG.setPipeName( "shmempipe_test" );
    CONFIG.addPool(500, sizeof(MyTestMsg));
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
    while (!connected)
        usleep(1);
    do {
        shmempipeStats stats;
        pPipe->getStats(&stats,true);
        printf("sb %lld sp %lld ss %lld rb %lld rp %lld rs %lld "
               "af %lld fb %lld\n",
               stats.sent_bytes, stats.sent_packets, stats.sent_signals,
               stats.rcvd_bytes, stats.rcvd_packets, stats.rcvd_signals,
               stats.alloc_fails, stats.free_buffers);
        usleep(100000);
    } while (connected);
    delete pPipe;
    return 0;
}

\endcode

\code

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
    while (!connected)
        usleep(1);
    for (int counter=0; counter < 50; counter++)
    {
        MyTestMsg * pMsg = MyTestMsg::allocSize(pPipe);
        pMsg->seqno = 1;
        pPipe->send(pMsg);
    }
    do {
        shmempipeStats stats;
        pPipe->getStats(&stats,true);
        printf("sb %lld sp %lld ss %lld rb %lld rp %lld rs %lld "
               "af %lld fb %lld\n",
               stats.sent_bytes, stats.sent_packets, stats.sent_signals,
               stats.rcvd_bytes, stats.rcvd_packets, stats.rcvd_signals,
               stats.alloc_fails, stats.free_buffers);
        usleep(100000);
    } while (connected && !done);
    delete pPipe;
    return 0;
}

\endcode

 */
