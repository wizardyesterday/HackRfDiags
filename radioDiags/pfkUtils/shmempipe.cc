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

#include "shmempipe.h"

shmempipe :: shmempipe( shmempipeMasterConfig * pConfig )
{
    pConfig->bInitialized = false;
    m_bMaster = true;
    m_bConnected = false;
    m_bufferListsInitialized = false;
    m_closerState = CLOSER_NOT_EXIST;
    pConfig->bInitialized = false;
    m_shmemFd = m_myPipeFd = m_otherPipeFd = -1;
    m_shmemPtr = 0;
    m_filename = *(shmempipeFilename*)pConfig;
    m_callbacks = *(shmempipeCallbacks*)pConfig;
    if (pConfig->numPools == 0)
    {
        fprintf(stderr, "no buffer pools defined\n");
        return;
    }
    (void) unlink(m_filename.filename);
    m_shmemFd = open(m_filename.filename, O_RDWR | O_CREAT, 0600);
    if (m_shmemFd < 0)
    {
        fprintf(stderr, "unable to create file %s: %s\n",
                m_filename.filename, strerror(errno));
        return;
    }
    (void) unlink(m_filename.m2sname);
    if (mkfifo(m_filename.m2sname, 0600) < 0)
    {
        fprintf(stderr, "unable to create pipe %s: %s\n",
                m_filename.m2sname, strerror(errno));
        return;
    }
    (void) unlink(m_filename.s2mname);
    if (mkfifo(m_filename.s2mname, 0600) < 0)
    {
        fprintf(stderr, "unable to create pipe %s: %s\n",
                m_filename.s2mname, strerror(errno));
        return;
    }
    m_fileSize = sizeof(shmempipeHeader);
    int poolInd;
    uint32_t poolOffset = m_fileSize;
    for (poolInd = 0;
         poolInd < pConfig->numPools;
         poolInd++)
    {
        m_fileSize +=
            (pConfig->bufSizes[poolInd] +
             sizeof(shmempipeMessage)) *
            pConfig->numBufs[poolInd];
    }

    if (ftruncate(m_shmemFd, (off_t) m_fileSize) < 0)
        fprintf(stderr, "ftruncate %s to %u failed: %s\n",
                m_filename.filename, (unsigned) m_fileSize,
                strerror(errno));
    m_shmemPtr = (uintptr_t)
        mmap(NULL, m_fileSize, PROT_READ | PROT_WRITE,
             MAP_SHARED, m_shmemFd, 0);
    if (m_shmemPtr == (uintptr_t)MAP_FAILED)
    {
        m_shmemPtr = 0;
        fprintf(stderr, "unable to mmap file: %s: %s\n",
                m_filename.filename, strerror(errno));
        return;
    }

    m_shmemLimit = m_shmemPtr + m_fileSize;
    m_pHeader = (shmempipeHeader *) m_shmemPtr;

    m_pHeader->poolInfo = *(shmempipePoolInfo*)pConfig;

    m_pHeader->master2slave.init();
    m_pHeader->slave2master.init();

    m_pHeader->attachedFlag = false;

    uintptr_t  buf = m_shmemPtr + poolOffset;

    for (poolInd = 0;
         poolInd < pConfig->numPools;
         poolInd++)
    {
        m_pHeader->pools[poolInd].init();
        for (uint32_t bufnum=0;
             bufnum < pConfig->numBufs[poolInd];
             bufnum++)
        {
            shmempipeMessage * msg = (shmempipeMessage *) buf;
            msg->bufInd = bufnum;
            msg->poolInd = poolInd;
            // add buf to pool
            m_pHeader->pools[poolInd].enqueue(
                m_shmemPtr, msg,
                NULL, false, false /*for speed*/);
            buf +=
                pConfig->bufSizes[poolInd] +
                sizeof(shmempipeMessage);
        }
    }

    m_myBufferList = &m_pHeader->slave2master;
    m_otherBufferList = &m_pHeader->master2slave;

    m_bufferListsInitialized = true;
    m_bReaderRunning = false;

    pthread_mutexattr_t mattr;
    pthread_mutexattr_init( &mattr );
    pthread_mutex_init( &m_statsMutex, &mattr );
    pthread_mutexattr_destroy( &mattr );
    startCloserThread();
    pConfig->bInitialized = true;
}

shmempipe :: shmempipe( shmempipeSlaveConfig * pConfig )
{
    pConfig->bInitialized = false;
    m_bMaster = false;
    m_bConnected = false;
    m_bufferListsInitialized = false; // i'm the slave, i didn't init them.
    m_closerState = CLOSER_NOT_EXIST;
    m_shmemFd = m_myPipeFd = m_otherPipeFd = -1;
    m_shmemPtr = 0;
    m_filename = *(shmempipeFilename*)pConfig;
    m_callbacks = *(shmempipeCallbacks*)pConfig;
    m_shmemFd = open(m_filename.filename, O_RDWR);
    if (m_shmemFd < 0)
    {
        fprintf(stderr, "unable to open file %s: %s\n",
                m_filename.filename, strerror(errno));
        return;
    }
    struct stat sb;
    fstat(m_shmemFd,&sb);
    m_fileSize = sb.st_size;
    m_shmemPtr = (uintptr_t)
        mmap(NULL, m_fileSize, PROT_READ | PROT_WRITE,
             MAP_SHARED, m_shmemFd, 0);
    if (m_shmemPtr == (uintptr_t)MAP_FAILED)
    {
        m_shmemPtr = 0;
        fprintf(stderr, "unable to mmap file: %s: %s\n",
                m_filename.filename, strerror(errno));
        return;
    }
    m_shmemLimit = m_shmemPtr + m_fileSize;
    m_pHeader = (shmempipeHeader *) m_shmemPtr;

    if (m_pHeader->attachedFlag == true)
    {
        fprintf(stderr, "shmem is already attached!\n",
                m_filename.filename, strerror(errno));
        return;
    }

    m_myBufferList = &m_pHeader->master2slave;
    m_otherBufferList = &m_pHeader->slave2master;

    pthread_mutexattr_t mattr;
    pthread_mutexattr_init( &mattr );
    pthread_mutex_init( &m_statsMutex, &mattr );
    pthread_mutexattr_destroy( &mattr );
    startCloserThread();
    m_pHeader->attachedFlag = true;
    pConfig->bInitialized = true;
}

shmempipe :: ~shmempipe(void)
{
    if (m_bReaderRunning)
        stopReaderThread();
    stopCloserThread();
    if (m_bufferListsInitialized)
    {
        m_pHeader->master2slave.cleanup();
        m_pHeader->slave2master.cleanup();
        for (int poolInd = 0;
             poolInd < m_pHeader->poolInfo.numPools;
             poolInd++)
        {
            m_pHeader->pools[poolInd].cleanup();
        }
    }
    pthread_mutex_destroy( &m_statsMutex );
    if (m_shmemFd > 0)
        close(m_shmemFd);
    if (m_myPipeFd > 0)
        close(m_myPipeFd);
    if (m_otherPipeFd > 0)
        close(m_otherPipeFd);
    if (m_shmemPtr != 0)
        munmap((void*)m_shmemPtr, m_fileSize);
    if (m_bMaster)
    {
        (void) unlink(m_filename.filename);
        (void) unlink(m_filename.m2sname);
        (void) unlink(m_filename.s2mname);
    }
}

void
shmempipeBufferList :: init(void)
{
    head = tail = 0;
    count = 0;
    needspoke = false;
    bIsWaiting = false;
    pthread_mutexattr_t mattr;
    pthread_condattr_t  cattr;
    pthread_mutexattr_init( &mattr );
    pthread_condattr_init( &cattr );
#if HAVE_PTHREAD_MUTEXATTR_SETPSHARED
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
#endif
#if HAVE_PTHREAD_MUTEXATTR_SETROBUST_NP
    pthread_mutexattr_setrobust_np(&mattr, PTHREAD_MUTEX_ROBUST_NP);
#endif
#if HAVE_PTHREAD_CONDATTR_SETPSHARED
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
#endif
    pthread_mutex_init( &mutex, &mattr );
    pthread_cond_init( &empty_cond, &cattr );
    pthread_mutexattr_destroy( &mattr );
    pthread_condattr_destroy( &cattr );
}

void
shmempipeBufferList :: cleanup(void)
{
// this hangs sometimes. 
//    pthread_cond_destroy(&empty_cond);
    pthread_mutex_destroy(&mutex);
}

void
shmempipe  :: startCloserThread(void)
{
    pthread_attr_t  attr;

    m_closerState = CLOSER_NOT_EXIST;
    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
    pthread_create(&m_closerId, &attr, &_closerThreadEntry, (void*) this);
    pthread_attr_destroy( &attr );

    while (m_closerState != CLOSER_RUNNING)
        usleep(1);
}

void
shmempipe  :: stopCloserThread(void)
{
    int count = 0;
    char c = 1;
    if (write(m_closerPipe.writeEnd, &c, 1) < 0)
        fprintf(stderr, "shmempipe  :: stopCloserThread: write failed\n");
    while (m_closerState != CLOSER_DEAD)
    {
        if (++count > 30) // wait 3 seconds
        {
            break;
        }
        usleep(100000);
    }
    m_closerState = CLOSER_NOT_EXIST;
}

//static
void *
shmempipe :: _closerThreadEntry(void * arg)
{
    shmempipe * pPipe = (shmempipe *) arg;
    pPipe->closerThread();
    return NULL;
}

void
shmempipe :: closerThread(void)
{
    shmempipeCallbacks callbacks = m_callbacks;
    char c;
    m_closerState = CLOSER_RUNNING;

    if (m_bMaster)
    {
        // open my pipe, wait for connect indication.
        // when i get one, open other pipe, write connect indication.
        m_myPipeFd = open(m_filename.s2mname, O_RDONLY | O_NONBLOCK);
        if (m_myPipeFd < 0)
        {
            printf("closerThread : Failure opening s2m\n");
            m_closerState = CLOSER_DEAD;
            callbacks.disconnectCallback(this, callbacks.arg);
            return;
        }
        while (1)
        {
            pxfe_select sel;
            sel.rfds.set(m_myPipeFd);
            sel.rfds.set(m_closerPipe.readEnd);
            sel.tv.set(1,0);
            if (sel.select() <= 0)
                continue;
            if (sel.rfds.isset(m_myPipeFd))
            {
                if (read(m_myPipeFd, &c, 1) <= 0)
                {
                    printf("shmempipe :: closerThead pipe closed\n");
                    m_closerState = CLOSER_DEAD;
                    callbacks.disconnectCallback(this, callbacks.arg);
                    return;
                }
                break;
            }
            if (sel.rfds.isset(m_closerPipe.readEnd))
            {
                printf("shmempipe :: closerThread told to exit from init\n");
                if (read(m_closerPipe.readEnd, &c, 1) < 0)
                    fprintf(stderr, "shmempipe :: closerThread: "
                            "read failed\n");
                m_closerState = CLOSER_DEAD;
                callbacks.disconnectCallback(this, callbacks.arg);
                return;
            }
        }
        m_otherPipeFd = open(m_filename.m2sname, O_WRONLY);
        if (m_otherPipeFd < 0)
        {
            printf("closerThread : Failure opening m2s\n");
            m_closerState = CLOSER_DEAD;
            callbacks.disconnectCallback(this, callbacks.arg);
            return;
        }
        c = 1;
        if (write(m_otherPipeFd, &c, 1) < 0)
            fprintf(stderr, "closerThread: write failed\n");
    }
    else
    {
        // open other pipe, write connect indication,
        // then try to open my pipe, wait for connect indication.
        int counter = 0;

        m_otherPipeFd = open(m_filename.s2mname, O_WRONLY | O_NONBLOCK);
        if (m_otherPipeFd < 0)
        {
            printf("shmempipe slave connect fail\n");
            m_closerState = CLOSER_DEAD;
            callbacks.disconnectCallback(this, callbacks.arg);
            return;
        }
        c = 1;
        if (write(m_otherPipeFd, &c, 1) < 0)
            fprintf(stderr, "closerThread: write failed\n");
        m_myPipeFd = open(m_filename.m2sname, O_RDONLY | O_NONBLOCK);
        if (m_myPipeFd < 0)
        {
            printf("closerThread : Failure opening m2s\n");
            m_closerState = CLOSER_DEAD;
            callbacks.disconnectCallback(this, callbacks.arg);
            return;
        }
        pxfe_select sel;
        sel.rfds.set(m_myPipeFd);
        sel.rfds.set(m_closerPipe.readEnd);
        sel.tv.set(1,0);
        if (sel.select() <= 0)
        {
            // failure negotiating with server
            printf("shmempipe slave connect ack timeout\n");
            m_closerState = CLOSER_DEAD;
            callbacks.disconnectCallback(this, callbacks.arg);
            return;
        }
        if (sel.rfds.isset(m_closerPipe.readEnd))
        {
            printf("shmempipe :: closerThread told to exit from init\n");
            if (read(m_closerPipe.readEnd, &c, 1) < 0)
                fprintf(stderr, "closerThread: read failed\n");
            m_closerState = CLOSER_DEAD;
            callbacks.disconnectCallback(this, callbacks.arg);
            return;
        }
        if (sel.rfds.isset(m_myPipeFd))
        {
            if (read(m_myPipeFd, &c, 1) <= 0)
            {
                printf("shmempipe slave connect lost\n");
                m_closerState = CLOSER_DEAD;
                callbacks.disconnectCallback(this, callbacks.arg);
                return;
            }
        }
    }

    m_bConnected = true;
    callbacks.connectCallback(this, callbacks.arg);
    startReaderThread();
    while (1)
    {
        fd_set rfds;
        int maxfd;

        FD_ZERO(&rfds);
        FD_SET(m_myPipeFd, &rfds);
        FD_SET(m_closerPipe.readEnd, &rfds);
        if (m_myPipeFd > m_closerPipe.readEnd)
            maxfd = m_myPipeFd + 1;
        else
            maxfd = m_closerPipe.readEnd + 1;
        select(maxfd, &rfds, NULL, NULL, NULL);
        if (FD_ISSET(m_closerPipe.readEnd, &rfds))
        {
            if (read(m_closerPipe.readEnd, &c, 1) < 0)
                fprintf(stderr, "closerThread: read failed\n");
            break;
        }
        if (FD_ISSET(m_myPipeFd, &rfds))
        {
            char buf[10];
            int cc = read(m_myPipeFd, buf, sizeof(buf));
            if (cc <= 0)
                break;
        }
    }
    m_closerState = CLOSER_EXITING;
    m_bConnected = false;
    stopReaderThread();

    if (m_myPipeFd != -1)
    {
        close(m_myPipeFd);
        m_myPipeFd = -1;
    }
    if (m_otherPipeFd != -1)
    {
        close(m_otherPipeFd);
        m_otherPipeFd = -1;
    }

    m_closerState = CLOSER_DEAD;
    callbacks.disconnectCallback(this, callbacks.arg);
}

void
shmempipe :: startReaderThread(void)
{
    pthread_attr_t  attr;
    pthread_t id;

    m_bReaderStop = false;
    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
    pthread_create(&id, &attr, &_readerThreadEntry, (void*) this);
    pthread_attr_destroy( &attr );

    while (m_bReaderRunning == false)
        usleep(1);
}

void
shmempipe :: stopReaderThread(void)
{
    if (m_bReaderRunning)
    {
        m_bReaderStop = true;
        m_myBufferList->poke();
        while (m_bReaderRunning)
        {
            usleep(1);
        }
    }
}

//static
void *
shmempipe :: _readerThreadEntry(void *arg)
{
    shmempipe * pPipe = (shmempipe *) arg;
    pPipe->readerThread();
    return NULL;
}

void
shmempipe :: readerThread(void)
{
    m_bReaderRunning = true;

    while (m_bReaderStop == false)
    {
        bool signalled = false;
        shmempipeMessage * pMsg
            = m_myBufferList->dequeue(m_shmemPtr,
                                      &signalled, true, true);
        if (pMsg)
        {
            lockStats();
            m_stats.rcvd_packets ++;
            m_stats.rcvd_bytes += pMsg->messageSize;
            if (signalled)
                m_stats.rcvd_signals ++;
            unlockStats();
            m_callbacks.messageCallback(this, m_callbacks.arg, pMsg);
        }
    }

    m_bReaderRunning = false;
}

void
shmempipe :: getStats(shmempipeStats * pStats, bool zero)
{
    lockStats();
    *pStats = m_stats;
    if (zero)
        m_stats.init();
    uint64_t free_buffers = 0;
    for (int poolInd = 0;
         poolInd < m_pHeader->poolInfo.numPools;
         poolInd++)
    {
        free_buffers += m_pHeader->pools[poolInd].get_count();
    }
    pStats->free_buffers = free_buffers;
    unlockStats();
}
