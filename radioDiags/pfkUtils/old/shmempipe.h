
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>

#include "circular_buffer.H"

struct shmempipePoolInfo {
    static const int maxPools = 64;
    uint8_t numPools;
    uint16_t bufSizes[maxPools]; // specify in increasing order.
    uint16_t numBufs[maxPools];
    shmempipePoolInfo(void) { numPools = 0; }
    bool addPool(uint16_t numBuf, uint16_t bufSize) {
        if (numPools == maxPools)
            return false;
        bufSizes[numPools] = bufSize;
        numBufs[numPools] = numBuf;
        numPools++;
        return true;
    }
};

class shmempipeMessage;

#define SHMEMPIPE_FILEPATH_FMT "/tmp/p2p.%s.shmem"
#define SHMEMPIPE_M2SPATH_FMT "/tmp/p2p.%s.m2s.fifo"
#define SHMEMPIPE_S2MPATH_FMT "/tmp/p2p.%s.s2m.fifo"

struct shmempipeFilenames {
    static const int FILENAMEMAX = 512;
    char filename[FILENAMEMAX];
    char masterSlaveFifo[FILENAMEMAX];
    char slaveMasterFifo[FILENAMEMAX];
    shmempipeFilenames(void) {
        filename[0] = 0;
        masterSlaveFifo[0] = 0;
        slaveMasterFifo[0] = 0;
    }
    void setPipeName(const char * pipename) {
        snprintf(filename,sizeof(filename),
                 SHMEMPIPE_FILEPATH_FMT, pipename);
        snprintf(masterSlaveFifo, sizeof(masterSlaveFifo),
                 SHMEMPIPE_M2SPATH_FMT, pipename);
        snprintf(slaveMasterFifo, sizeof(slaveMasterFifo),
                 SHMEMPIPE_S2MPATH_FMT, pipename);
        filename[FILENAMEMAX-1] = 0;
        masterSlaveFifo[FILENAMEMAX-1] = 0;
        slaveMasterFifo[FILENAMEMAX-1] = 0;
    }
};

class shmempipe;
class shmempipeHandler {
public:
    shmempipe * pPipe;
    virtual void connectHandler(void) = 0;
    virtual void disconnectHandler(void) = 0;
    virtual void messageHandler(shmempipeMessage *pBuf) = 0;
};

struct shmempipeMasterConfig : shmempipeFilenames {
    static const int DEFAULT_FLUSH_USECS = 10000;
    static const int DEFAULT_BUFFERSIZE = 4096;
    static const int DEFAULT_RELEASEQUEUESIZE = 1024;
    shmempipePoolInfo master2slave;
    shmempipePoolInfo slave2master;
    int  masterReleaseQueueSize;
    int  slaveReleaseQueueSize;
    int  flushUsecs;
    int  bufferSize;
    bool bInitialized;
    shmempipeHandler * pHandler;
    shmempipeMasterConfig(void) {
        bInitialized = false;
        pHandler = NULL;
        flushUsecs = DEFAULT_FLUSH_USECS;
        bufferSize = DEFAULT_BUFFERSIZE;
        masterReleaseQueueSize = DEFAULT_RELEASEQUEUESIZE;
        slaveReleaseQueueSize = DEFAULT_RELEASEQUEUESIZE;
    }
};

struct shmempipeSlaveConfig {
    static const int DEFAULT_FLUSH_USECS = 10000;
    static const int DEFAULT_BUFFERSIZE = 4096;
    char filename[shmempipeFilenames::FILENAMEMAX];
    int flushUsecs;
    int bufferSize;
    bool bInitialized;
    shmempipeHandler * pHandler;
    shmempipeSlaveConfig(void) {
        bInitialized = false;
        filename[0] = 0;
        flushUsecs = DEFAULT_FLUSH_USECS;
        bufferSize = DEFAULT_BUFFERSIZE;
        pHandler = NULL;
    }
    void setPipeName(const char * pipename) {
        snprintf(filename,sizeof(filename),
                 SHMEMPIPE_FILEPATH_FMT, pipename);
        filename[shmempipeFilenames::FILENAMEMAX-1] = 0;
    }
};

struct shmempipeMessage {
private:
    shmempipeMessage * next;
    uint8_t bMaster;
    uint8_t poolInd;
    uint16_t bufInd;
    uint16_t bufferSize;
public:
    uint16_t messageSize;
private:
    enum { FREE=1, APP, QUEUE } owner;
    friend class shmempipe;
};

struct shmempipeHeader : shmempipeFilenames {
    shmempipePoolInfo slave2master;
    int flushUsecs;
    int  masterReleaseQueueSize;
    int  masterReleaseQueueOffset;
    int  slaveReleaseQueueSize;
    int  slaveReleaseQueueOffset;
    int  master2slavePoolOffset;
    int  slave2masterPoolOffset;
    // master message queue
    // slave message queue
    // master release queue
    // slave release queue
    // master buffer pool
    // slave buffer pool
};

struct shmempipeStats {
    uint64_t sent_bytes;
    uint64_t sent_packets;
    uint64_t rcvd_bytes;
    uint64_t rcvd_packets;
    uint64_t sent_messages;
    uint64_t rcvd_messages;
    uint64_t buffers_read;
    uint64_t buffers_flushed;
    uint64_t alloc_waits;
    uint64_t alloc_fails;
    shmempipeStats(void) { init(); }
    void init(void) { 
        memset(this, 0, sizeof(*this));
    }
};

class shmempipe {
    bool m_bMaster;
    bool m_bConnected;
    shmempipeFilenames m_filenames;
    shmempipeStats m_stats;
    shmempipeHandler * m_pHandler;
    int m_writePipe;
    int m_readPipe;
    int m_shmemFd;
    int m_fileSize;
    uintptr_t m_shmemPtr;
    uintptr_t m_shmemLimit;
    shmempipeHeader * m_pHeader;
    int myReleaseQueueSize;
    int myReleaseQueueIndex;
    uint32_t * pMyReleaseQueue; // i read from this and write 0s
    int otherReleaseQueueIndex;
    int otherReleaseQueueSize;
    uint32_t * pOtherReleaseQueue; // i write non-0s to this
    shmempipePoolInfo m_poolInfo;
    shmempipeMessage * m_pools[shmempipePoolInfo::maxPools];
    int m_poolCounts[shmempipePoolInfo::maxPools];
    uintptr_t  m_poolMins[shmempipePoolInfo::maxPools];
    uintptr_t  m_poolMaxs[shmempipePoolInfo::maxPools];
    circular_buffer * m_writeBuffer;
    int m_bufferSize;
    static const uint32_t  FREE_FLAG   = 0x80000000;
    static const uint32_t  INIT_FLAG   = 0x40000000;
    static const uint32_t  OFFSET_MASK = 0x3fffffff;
    pthread_mutex_t m_mutex;
    void   lock(void) { pthread_mutex_lock  ( &m_mutex ); }
    void unlock(void) { pthread_mutex_unlock( &m_mutex ); }
    int m_closerPipe[2];
    pthread_t m_readerThreadId;
    pthread_t m_flusherThreadId;
    bool m_bReaderThreadRunning;
    bool m_bFlusherThreadRunning;
    pthread_cond_t m_allocWaiters;
    bool validateBuf(shmempipeMessage * pBuf);
    void releaseBuffer(shmempipeMessage * pBuf, bool needs_lock, 
                       bool rcvd_stat_bump);
    void checkReleaseQueue(void);
    void initPools(uintptr_t walkingPtr);
    void sendMessage(uint32_t message, bool flush=true);
    void processMessage(uint32_t message);
    bool createReaderThread(void);
    void stopReaderThread(void);
    static void * readerThreadEntry( void * pObject );
    void readerThreadMain(void);
    static void * flusherThreadEntry( void * pObject );
    void flusherThreadMain(void);
    bool flushWriteBuf(void); // false if failed
    shmempipeMessage * messageToBuffer(uint32_t message) {
        return (shmempipeMessage *) (m_shmemPtr + (message & OFFSET_MASK));
    }
public:
    shmempipe( shmempipeMasterConfig * pConfig );
    shmempipe( shmempipeSlaveConfig * pConfig );
    ~shmempipe( void );
    shmempipeMessage * allocSize(int size,    bool bWait,
                                bool lookToNextPool=true);
    shmempipeMessage * allocPool(int poolInd, bool bWait);
    void release(shmempipeMessage * pBuf, bool flush=true);
    bool send(shmempipeMessage * pBuf, bool flush=true);
    void flush(void);
    void getStats(shmempipeStats * pStats, bool zero=false);
};
