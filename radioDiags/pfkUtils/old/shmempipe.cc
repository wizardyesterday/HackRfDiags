
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/time.h>
#include <signal.h>

#include "shmempipe.H"

shmempipe :: shmempipe( shmempipeMasterConfig * pConfig )
{
    int poolInd;
    m_bMaster = true;
    m_bReaderThreadRunning = false;
    pConfig->bInitialized = false;
    m_writePipe = -1;
    m_readPipe = -1;
    m_shmemFd = -1;
    m_shmemPtr = 0;
    m_writeBuffer = NULL;
    m_pHandler = pConfig->pHandler;
    m_pHandler->pPipe = this;
    shmempipeFilenames * pFilenames = pConfig;
    memcpy(&m_filenames, pFilenames, sizeof(m_filenames));
    (void) unlink( m_filenames.masterSlaveFifo );
    (void) unlink( m_filenames.slaveMasterFifo );
    if (pConfig->master2slave.numPools == 0)
    {
        fprintf(stderr, "no master-to-slave buffer pools defined\n");
        return;
    }
    if (pConfig->slave2master.numPools == 0)
    {
        fprintf(stderr, "no slave-to-master buffer pools defined\n");
        return;
    }
    if (mkfifo(m_filenames.masterSlaveFifo, 0600) < 0)
    {
        fprintf(stderr, "unable to create fifo %s: %s\n",
                m_filenames.masterSlaveFifo, strerror(errno));
        return;
    }
    if (mkfifo(m_filenames.slaveMasterFifo, 0600) < 0)
    {
        fprintf(stderr, "unable to create fifo %s: %s\n",
                m_filenames.masterSlaveFifo, strerror(errno));
        return;
    }
    m_readPipe = open(m_filenames.slaveMasterFifo, O_RDONLY | O_NONBLOCK);
    if (m_readPipe < 0)
    {
        fprintf(stderr, "open %s: %s\n", m_filenames.slaveMasterFifo,
                strerror(errno));
        return;
    }
    (void) unlink( m_filenames.filename );
    m_shmemFd = open(m_filenames.filename, O_RDWR | O_CREAT, 0600);
    if (m_shmemFd < 0)
    {
        fprintf(stderr, "unable to create file %s: %s\n",
                m_filenames.filename, strerror(errno));
        return;
    }
    m_fileSize = sizeof(shmempipeHeader);

    int masterReleaseQueueOffset;
    int slaveReleaseQueueOffset;
    int master2slavePoolOffset;
    int slave2masterPoolOffset;

    masterReleaseQueueOffset = m_fileSize;
    m_fileSize += pConfig->masterReleaseQueueSize * sizeof(uint32_t);
    myReleaseQueueSize = pConfig->masterReleaseQueueSize;
    myReleaseQueueIndex = 0;

    slaveReleaseQueueOffset = m_fileSize;
    m_fileSize += pConfig->slaveReleaseQueueSize * sizeof(uint32_t);
    otherReleaseQueueSize = pConfig->slaveReleaseQueueSize;
    otherReleaseQueueIndex = 0;

    master2slavePoolOffset = m_fileSize;
    for (poolInd = 0;
         poolInd < pConfig->master2slave.numPools;
         poolInd++)
    {
        m_fileSize +=
            pConfig->master2slave.bufSizes[poolInd] *
            pConfig->master2slave.numBufs[poolInd];
    }

    slave2masterPoolOffset = m_fileSize;
    for (poolInd = 0;
         poolInd < pConfig->slave2master.numPools;
         poolInd++)
    {
        m_fileSize +=
            pConfig->slave2master.bufSizes[poolInd] *
            pConfig->slave2master.numBufs[poolInd];
    }

    ftruncate(m_shmemFd, (off_t) m_fileSize);
    m_shmemPtr = (uintptr_t)
        mmap(NULL, m_fileSize, PROT_READ | PROT_WRITE,
             MAP_SHARED, m_shmemFd, 0);
    if (m_shmemPtr == (uintptr_t)MAP_FAILED)
    {
        m_shmemPtr = 0;
        fprintf(stderr, "unable to mmap file: %s: %s\n",
                m_filenames.filename, strerror(errno));
        return;
    }

    m_shmemLimit = m_shmemPtr + m_fileSize;
    m_pHeader = (shmempipeHeader *) m_shmemPtr;

    // above this line, no writes to shared memory.

    memcpy(m_pHeader, pConfig, sizeof(shmempipeFilenames));

    m_pHeader->masterReleaseQueueOffset = masterReleaseQueueOffset;
    m_pHeader->slaveReleaseQueueOffset = slaveReleaseQueueOffset;
    m_pHeader->master2slavePoolOffset = master2slavePoolOffset;
    m_pHeader->slave2masterPoolOffset = slave2masterPoolOffset;

    memcpy(&m_poolInfo, &pConfig->master2slave, sizeof(shmempipePoolInfo));
    initPools(m_shmemPtr + m_pHeader->master2slavePoolOffset);
    memcpy(&m_pHeader->slave2master, &pConfig->slave2master, 
           sizeof(shmempipePoolInfo));
    m_bufferSize = pConfig->bufferSize;
    m_writeBuffer = new circular_buffer( m_bufferSize );

    m_pHeader->masterReleaseQueueSize = pConfig->masterReleaseQueueSize;
    m_pHeader->slaveReleaseQueueSize = pConfig->slaveReleaseQueueSize;

    pMyReleaseQueue = (uint32_t*)
        (m_shmemPtr + m_pHeader->masterReleaseQueueOffset);
    pOtherReleaseQueue = (uint32_t*)
        (m_shmemPtr + m_pHeader->slaveReleaseQueueOffset);
    memset(pMyReleaseQueue, 0,
           pConfig->masterReleaseQueueSize * sizeof(uint32_t));
    memset(pOtherReleaseQueue, 0,
           pConfig->slaveReleaseQueueSize * sizeof(uint32_t));

    m_pHeader->flushUsecs = pConfig->flushUsecs;
    m_bConnected = false;

    pthread_condattr_t cattr;
    pthread_mutexattr_t mattr;
    pthread_condattr_init( &cattr );
    pthread_mutexattr_init( &mattr );
    pthread_cond_init(&m_allocWaiters, &cattr);
    pthread_mutex_init( &m_mutex, &mattr );
    pthread_mutexattr_destroy( &mattr );
    pthread_condattr_destroy( &cattr );

    if (createReaderThread())
        pConfig->bInitialized = true;
}

shmempipe :: shmempipe( shmempipeSlaveConfig * pConfig )
{
    struct stat sb;
    m_bMaster = false;
    m_bReaderThreadRunning = false;
    pConfig->bInitialized = false;
    m_writePipe = -1;
    m_readPipe = -1;
    m_shmemFd = -1;
    m_shmemPtr = 0;
    m_writeBuffer = NULL;
    m_pHandler = pConfig->pHandler;
    m_pHandler->pPipe = this;
    m_shmemFd = open(pConfig->filename, O_RDWR);
    if (m_shmemFd < 0)
    {
        fprintf(stderr, "unable to open file %s: %s\n",
                pConfig->filename, strerror(errno));
        return;
    }
    if (fstat(m_shmemFd, &sb) < 0)
    {
        fprintf(stderr, "unable to fstat file: %s: %s\n",
                pConfig->filename, strerror(errno));
        return;
    }
    m_fileSize = (int)sb.st_size;
    m_shmemPtr = (uintptr_t)
        mmap(NULL, m_fileSize, PROT_READ | PROT_WRITE,
             MAP_SHARED, m_shmemFd, 0);
    if (m_shmemPtr == (uintptr_t)MAP_FAILED)
    {
        m_shmemPtr = 0;
        fprintf(stderr, "unable to mmap file: %s: %s\n",
                pConfig->filename, strerror(errno));
        return;
    }
    m_shmemLimit = m_shmemPtr + m_fileSize;
    m_pHeader = (shmempipeHeader *) m_shmemPtr;
    memcpy(&m_filenames, (shmempipeFilenames *)m_pHeader, sizeof(m_filenames));
    m_readPipe = open(m_filenames.masterSlaveFifo, O_RDONLY | O_NONBLOCK);
    memcpy(&m_poolInfo, &m_pHeader->slave2master, sizeof(shmempipePoolInfo));
    initPools(m_shmemPtr + m_pHeader->slave2masterPoolOffset);
    m_bufferSize = pConfig->bufferSize;
    m_writeBuffer = new circular_buffer( m_bufferSize );

    myReleaseQueueSize = m_pHeader->slaveReleaseQueueSize;
    otherReleaseQueueSize = m_pHeader->masterReleaseQueueSize;
    pMyReleaseQueue = (uint32_t*)
        (m_shmemPtr + m_pHeader->slaveReleaseQueueOffset);
    pOtherReleaseQueue = (uint32_t*)
        (m_shmemPtr + m_pHeader->masterReleaseQueueOffset);
    myReleaseQueueIndex = otherReleaseQueueIndex = 0;

    m_writePipe = open(m_filenames.slaveMasterFifo, O_WRONLY);
    sendMessage(INIT_FLAG);
    m_pHeader->flushUsecs = pConfig->flushUsecs;
    m_bConnected = false;

    pthread_condattr_t cattr;
    pthread_mutexattr_t mattr;
    pthread_condattr_init( &cattr );
    pthread_mutexattr_init( &mattr );
    pthread_cond_init(&m_allocWaiters, &cattr);
    pthread_mutex_init( &m_mutex, &mattr );
    pthread_mutexattr_destroy( &mattr );
    pthread_condattr_destroy( &cattr );

    if (createReaderThread())
        pConfig->bInitialized = true;
}

shmempipe :: ~shmempipe(void)
{
    stopReaderThread();
    if (m_writePipe > 0)
        close(m_writePipe);
    if (m_readPipe > 0)
        close(m_readPipe);
    if (m_shmemFd > 0)
        close(m_shmemFd);
    if (m_shmemPtr != 0)
        munmap((void*)m_shmemPtr, m_fileSize);
    if (m_bMaster)
    {
        (void) unlink(m_filenames.filename);
        (void) unlink(m_filenames.masterSlaveFifo);
        (void) unlink(m_filenames.slaveMasterFifo);
    }
    if (m_writeBuffer)
        delete m_writeBuffer;
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_allocWaiters);
}

void
shmempipe :: getStats(shmempipeStats * pStats, bool zero)
{
    lock();
    *pStats = m_stats;
    if (zero)
        m_stats.init();
    unlock();
}

void
shmempipe :: initPools(uintptr_t walkingPtr)
{
    shmempipePoolInfo * pPoolInfo = &m_poolInfo;
    uint16_t bufInd;
    uint8_t poolInd;
    int poolCount;
    for (poolInd = 0; poolInd < pPoolInfo->numPools; poolInd++)
    {
        poolCount = 0;
        m_pools[poolInd] = NULL;
        uint16_t bufSize;
        uint16_t numBufs;
        bufSize = pPoolInfo->bufSizes[poolInd];
        numBufs = pPoolInfo->numBufs[poolInd];
        m_poolMins[poolInd] = walkingPtr;
        for (bufInd = 0; bufInd < numBufs; bufInd++)
        {
            shmempipeMessage * pBuf;
            pBuf = (shmempipeMessage *) walkingPtr;
            pBuf->next = m_pools[poolInd];
            pBuf->bMaster = m_bMaster;
            pBuf->poolInd = poolInd;
            pBuf->bufInd = bufInd;
            pBuf->bufferSize = bufSize;
            pBuf->messageSize = 0;
            pBuf->owner = shmempipeMessage::FREE;
            m_pools[poolInd] = pBuf;
            walkingPtr += bufSize;
            poolCount++;
        }
        m_poolMaxs[poolInd] = walkingPtr;
        m_poolCounts[poolInd] = poolCount;
    }
}

inline bool
shmempipe :: validateBuf(shmempipeMessage * pBuf)
{
    uintptr_t  value = (uintptr_t) pBuf;
    if (value < m_shmemPtr  ||  value >= m_shmemLimit)
    {
        fprintf(stderr, "shmempipe :: validateBuf : case 0\n");
        return false;
    }
    if (pBuf->bMaster != 0 && pBuf->bMaster != 1)
    {
        fprintf(stderr, "shmempipe :: validateBuf : case 1\n");
        return false;
    }
    if (pBuf->bMaster != m_bMaster)
        return true; // not much we can validate, not our pools
    int poolInd = pBuf->poolInd;
    if (poolInd >= m_poolInfo.numPools)
    {
        fprintf(stderr, "shmempipe :: validateBuf : case 2\n");
        return false;
    }
    if (pBuf->bufInd >= m_poolInfo.numBufs[poolInd])
    {
        fprintf(stderr, "shmempipe :: validateBuf : case 3\n");
        return false;
    }
    if (value < m_poolMins[poolInd]  ||  value > m_poolMaxs[poolInd])
    {
        fprintf(stderr, "shmempipe :: validateBuf : case 4\n");
        return false;
    }
    uintptr_t poolOffset = value - m_poolMins[poolInd];
    if ((poolOffset % m_poolInfo.bufSizes[poolInd]) != 0)
    {
        fprintf(stderr, "shmempipe :: validateBuf : case 5\n");
        return false;
    }
    if ((poolOffset / m_poolInfo.bufSizes[poolInd]) != pBuf->bufInd)
    {
        fprintf(stderr, "shmempipe :: validateBuf : case 6 (%ld != %d)\n",
                (poolOffset / m_poolInfo.numBufs[poolInd]), pBuf->bufInd);
        return false;
    }
    return true;
}

inline void 
shmempipe :: releaseBuffer(shmempipeMessage * pBuf,
                           bool needs_lock,
                           bool rcvd_stat_bump)
{
    pBuf->owner = shmempipeMessage::FREE;
    if (needs_lock)
        lock();
    pBuf->next = m_pools[pBuf->poolInd];
    m_pools[pBuf->poolInd] = pBuf;
    m_poolCounts[pBuf->poolInd]++;
    if (rcvd_stat_bump)
        m_stats.rcvd_messages++;
    if (needs_lock)
        unlock();
    pthread_cond_signal(&m_allocWaiters);
}

// requires lock already taken
inline void 
shmempipe :: checkReleaseQueue(void)
{
    while (pMyReleaseQueue[myReleaseQueueIndex] != 0)
    {
        uint32_t message = pMyReleaseQueue[myReleaseQueueIndex];
        pMyReleaseQueue[myReleaseQueueIndex] = 0;

        shmempipeMessage * pBuf = messageToBuffer(message);
        if ((message & FREE_FLAG) && validateBuf(pBuf))
            releaseBuffer(pBuf, false, true);

        if (++myReleaseQueueIndex >= myReleaseQueueSize)
            myReleaseQueueIndex = 0;
    }
}

shmempipeMessage * 
shmempipe :: allocPool(int poolInd, bool bWait)
{
    if (poolInd >= m_poolInfo.numPools)
    {
        fprintf(stderr, "shmempipe :: allocpool : bogus poolInd %d\n", poolInd);
        return NULL;
    }
    lock();
    checkReleaseQueue();
    while (m_pools[poolInd] == NULL)
    {
        if (bWait == false)
        {
            unlock();
            m_stats.alloc_fails ++;
            return NULL;
        }
        else
        {
            pthread_cond_wait(&m_allocWaiters, &m_mutex);
            m_stats.alloc_waits ++;
        }
    }
    shmempipeMessage * ret = m_pools[poolInd];
    m_pools[poolInd] = ret->next;
    m_poolCounts[poolInd]--;
    unlock();
    ret->next = NULL;
    if (ret->owner != shmempipeMessage::FREE)
    {
        fprintf(stderr, "shmempipe :: allocpool : pool corruption!\n");
        return NULL;
    }
    ret->owner = shmempipeMessage::APP;
    return ret;
}

shmempipeMessage * 
shmempipe :: allocSize(int size, bool bWait, bool lookToNextPool)
{
    int poolInd;
    bool bFound = false;
    lock();
    checkReleaseQueue();
    while (!bFound)
    {
        for (poolInd=0; poolInd < m_poolInfo.numPools; poolInd++)
        {
            if (m_poolInfo.bufSizes[poolInd] >= size)
            {
                if (m_pools[poolInd] != NULL)
                {
                    bFound = true;
                    break;
                }
                if (!lookToNextPool)
                    break;
            }
        }
        if (!bFound)
        {
            if (bWait == false)
            {
                unlock();
                m_stats.alloc_fails ++;
                return NULL;
            }
            else
            {
                pthread_cond_wait(&m_allocWaiters, &m_mutex);
                m_stats.alloc_waits ++;
            }
        }
    }
    shmempipeMessage * ret = m_pools[poolInd];
    m_pools[poolInd] = ret->next;
    m_poolCounts[poolInd]--;
    unlock();
    ret->next = NULL;
    if (ret->owner != shmempipeMessage::FREE)
    {
        fprintf(stderr, "shmempipe :: allocpool : pool corruption!\n");
        return NULL;
    }
    ret->owner = shmempipeMessage::APP;
    return ret;
}

// requires lock already taken
inline bool
shmempipe :: flushWriteBuf(void)
{
    while (m_writeBuffer->used_space() > 0)
    {
        int towrite = m_writeBuffer->contig_readable();
        int cc = write(m_writePipe,
                       m_writeBuffer->read_pos(),
                       towrite);
        if (cc > 0)
            m_writeBuffer->record_read(cc);
        else
        {
//            fprintf(stderr, "write error: %s\n", strerror(errno));
            return false;
        }
        m_stats.buffers_flushed ++;
    }
    return true;
}

// requires lock already taken
inline void
shmempipe :: sendMessage(uint32_t message, bool flush /*=true*/ )
{
    if (m_writeBuffer->free_space() < 4)
        flushWriteBuf();
    m_writeBuffer->write((char*)&message, 4);
    if (flush)
        flushWriteBuf();
    m_stats.sent_messages++;
}

void 
shmempipe :: release(shmempipeMessage * pBuf, bool flush /*=true*/ )
{
    lock();
    if (!validateBuf(pBuf))
    {
        unlock();
        return;
    }
    if ((bool) pBuf->bMaster == m_bMaster)
    {
        if (pBuf->owner != shmempipeMessage::APP)
        {
            unlock();
            fprintf(stderr, "shmempipe :: release : pool corruption!\n");
            return;
        }
        releaseBuffer(pBuf, false, false);
    }
    else
    {
        uint32_t  message;
        message = (uint32_t) (((uintptr_t)pBuf) - m_shmemPtr);
        message |= FREE_FLAG;
        pBuf->owner = shmempipeMessage::QUEUE;

        if (pOtherReleaseQueue[otherReleaseQueueIndex] != 0)
            sendMessage(message,flush);
        else
        {
            pOtherReleaseQueue[otherReleaseQueueIndex] = message;
            if (++otherReleaseQueueIndex >= otherReleaseQueueSize)
                otherReleaseQueueIndex = 0;
        }
    }
    unlock();
}

bool
shmempipe :: send(shmempipeMessage * pBuf, bool flush /*=true*/ )
{
    uint32_t  message;
    lock();
    if (!validateBuf(pBuf))
    {
        unlock();
        return false;
    }
    if (!m_bConnected)
    {
        unlock();
        fprintf(stderr, "shmempipe :: send : not connected\n");
        return false;
    }
    if (pBuf->owner != shmempipeMessage::APP)
    {
        unlock();
        fprintf(stderr, "shmempipe :: send : pool corruption!\n");
        return false;
    }
    pBuf->owner = shmempipeMessage::QUEUE;
    message = (uint32_t) (((uintptr_t)pBuf) - m_shmemPtr);
    sendMessage(message,flush);
    m_stats.sent_bytes += pBuf->messageSize;
    m_stats.sent_packets ++;
    unlock();
    return true;
}

void
shmempipe :: flush(void)
{
    lock();
    flushWriteBuf();
    unlock();
}

bool
shmempipe :: createReaderThread(void) 
{
    pthread_attr_t tattr;
    int counter;
    pipe(m_closerPipe);
    pthread_attr_init( &tattr );
    pthread_attr_setdetachstate( &tattr, PTHREAD_CREATE_DETACHED);
    pthread_create( &m_readerThreadId,  &tattr, 
                    &readerThreadEntry,  (void*) this);
    counter = 50000;
    while (!m_bReaderThreadRunning && counter-- > 0)
        usleep(1);
    if (counter <= 0)
    {
        fprintf(stderr, "shmempipe thread failed to init!\n");
        return false;
    }
    signal(SIGPIPE, SIG_IGN);
    pthread_create( &m_flusherThreadId,  &tattr, 
                    &flusherThreadEntry,  (void*) this);
    pthread_attr_destroy( &tattr );
    return true;
}

void
shmempipe :: stopReaderThread(void) 
{
    int counter;
    if (m_bReaderThreadRunning || m_bFlusherThreadRunning)
    {
        char dummy = 1;
        write(m_closerPipe[1], &dummy, 1);
        counter = 50000;
        while (m_bReaderThreadRunning && counter-- > 0)
            usleep(1);
        if (counter <= 0)
            pthread_cancel(m_readerThreadId);
        counter = 50000;
        while (m_bFlusherThreadRunning && counter-- > 0)
            usleep(1);
        if (counter <= 0)
            pthread_cancel(m_flusherThreadId);
        close(m_closerPipe[0]);
        close(m_closerPipe[1]);
    }
}

inline void
shmempipe :: processMessage(uint32_t message)
{
    if (message & INIT_FLAG)
    {
        lock();
        m_stats.rcvd_messages++;
        unlock();
        if (!m_bConnected)
        {
            m_bConnected = true;
            if (m_bMaster)
            {
                m_writePipe = open(m_filenames.masterSlaveFifo, O_WRONLY);
                sendMessage(INIT_FLAG);
            }
            m_pHandler->connectHandler();
        }
    }
    else
    {
        shmempipeMessage * pBuf = messageToBuffer(message);
        if (validateBuf(pBuf))
        {
            if (pBuf->owner != shmempipeMessage::QUEUE)
            {
                fprintf(stderr, "shmempipe :: receive : pool corruption!\n");
            }
            else if (message & FREE_FLAG)
            {
                lock();
                checkReleaseQueue();
                releaseBuffer(pBuf, false, true);
                unlock();
            }
            else
            {
                pBuf->owner = shmempipeMessage::APP;
                lock();
                m_stats.rcvd_bytes += pBuf->messageSize;
                m_stats.rcvd_packets++;
                m_stats.rcvd_messages++;
                unlock();
                m_pHandler->messageHandler(pBuf);
            }
        }
    }
}

//static
void *
shmempipe :: readerThreadEntry( void * pObject )
{
    shmempipe * pPipe = (shmempipe *) pObject;
    pPipe->readerThreadMain();
    return NULL;
}

void
shmempipe :: readerThreadMain( void )
{
    int fd1 = m_closerPipe[0];
    int fd2 = m_readPipe;
    int maxfd = fd1 + 1;
    if (fd2 > fd1)
        maxfd = fd2 + 1;
    fd_set  rfds;
    int gotBytes, spaceRemaining;
    char buffer[m_bufferSize];
    gotBytes = 0;
    spaceRemaining = m_bufferSize;
    FD_ZERO(&rfds);
    m_bReaderThreadRunning = true;
    while (1)
    {
        FD_SET(fd1, &rfds);
        FD_SET(fd2, &rfds);
        select(maxfd, &rfds, NULL, NULL, NULL);
        if (FD_ISSET(fd1, &rfds))
            break;
        if (FD_ISSET(fd2, &rfds))
        {
            int cc = read(m_readPipe, buffer+gotBytes, spaceRemaining);
            if (cc == 0)
            {
                // normal close by remote side
                break;
            }
            if (cc < 0)
            {
                int error = errno;
                char * errStr = strerror(error);
                fprintf(stderr, "read error: %d:%s\n", error, errStr);
                break;
            }
            lock();
            m_stats.buffers_read ++;
            unlock();
            gotBytes += cc;
            spaceRemaining -= cc;
            if ((gotBytes & 3) == 0)
            {
                for (int ind = 0; ind < gotBytes; ind += 4)
                {
                    uint32_t message = *(uint32_t*)(buffer + ind);
                    processMessage(message);
                }
                gotBytes = 0;
                spaceRemaining = sizeof(buffer);
            }
        }
    }
    m_bReaderThreadRunning = false;
    if (m_bConnected)
    {
        m_bConnected = false;
        m_pHandler->disconnectHandler();
    }
}

//static
void *
shmempipe :: flusherThreadEntry( void * pObject )
{
    shmempipe * pPipe = (shmempipe *) pObject;
    pPipe->flusherThreadMain();
    return NULL;
}

void
shmempipe :: flusherThreadMain( void )
{
    int fd1 = m_closerPipe[0];
    fd_set rfds;
    FD_ZERO(&rfds);
    struct timeval tv;
    bool okay = true;
    m_bFlusherThreadRunning = true;
    while (okay && m_bReaderThreadRunning)
    {
        FD_SET(fd1, &rfds);
        tv.tv_sec  = m_pHeader->flushUsecs / 1000000;
        tv.tv_usec = m_pHeader->flushUsecs % 1000000;
        if (select(fd1+1,&rfds,NULL,NULL,&tv) > 0)
            break;
        lock();
        checkReleaseQueue();
        okay = flushWriteBuf();
        unlock();
    }
    m_bFlusherThreadRunning = false;
    stopReaderThread();
}
