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

#include "msgr.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <fcntl.h>

using namespace std;

bool
PfkMsg :: parse(void * _buf, uint32_t len, uint32_t *consumed)
{
    uint8_t * buf = (uint8_t *) _buf;
    uint32_t pos;
    uint32_t remain = len;
    max_msg_len = len;
    buffer = buf;
    for (pos = 0; pos < (len-1); pos++)
        if (buf[pos] == MAGIC1 && buf[pos+1] == MAGIC2)
            break;
    if (pos == (len-1))
    {
        *consumed = pos;
        return false;
    }
    remain -= pos;
    if (remain < 6)
    {
        // not enough for a header
        *consumed = pos;
        return false;
    }
    msg_len = ((uint32_t)(buf[pos+2]) << 8) + buf[pos+3];
    if (msg_len > max_msg_len)
    {
        cerr << "bogus msg len " << msg_len << " > "
             << max_msg_len << endl;
        *consumed = pos;
        return false;
    }
    if (remain < msg_len)
    {
        // not enough for the body
        *consumed = pos;
        return false;
    }
    num_fields = ((uint32_t)(buf[pos+4]) << 8) + buf[pos+5];
    if (num_fields > MAX_FIELDS)
    {
        cerr << "bogus num fields " << num_fields << endl;
        *consumed = pos;
        return false;
    }
    buf += 6;
    pos += 6;
    remain = msg_len - 6;
    uint32_t fld = 0;
    while (remain > 0)
    {
        if (remain < 3)
        {
            cerr << "framing error\n";
            *consumed = pos;
            return false;
        }
        uint16_t len = ((uint16_t)(buf[0]) << 8) + buf[1];
        fields[fld].len = len;
        buf += 2;
        pos += 2;
        remain -= 2;
        if (len >= max_msg_len || len > remain)
        {
            cerr << "bogus field " << fld << " len " << len << endl;
            *consumed = pos;
            return false;
        }
        fields[fld].data = buf;
        buf += len;
        pos += len;
        remain -= len;
        fld++;
    }
    *consumed = pos;
    if (fld != num_fields)
    {
        cerr << "num fields mismatch " << fld
             <<  " != " << num_fields << endl;
        return false;
    }
    return true;
}

//

PfkMsgr :: PfkMsgr(int port)
{
    isServer = true;
    common_init("",port);
}

PfkMsgr :: PfkMsgr(const std::string &host, int port)
{
    isServer = false;
    common_init(host,port);
}

void
PfkMsgr :: common_init(const std::string &host, int port)
{
    if (pipe(closerPipe) < 0)
        fprintf(stderr, "PfkMsgr :: common_init: pipe failed\n");
    threadRunning = false;
    fd = -1;
    dataFd = -1;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (isServer)
    {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
        {
            int e = errno;
            cerr << "error " << e << " making socket: "
                 << strerror(e) << endl;
            return;
        }
        int v = 1;
        setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, (void*) &v, sizeof( v ));
        sa.sin_addr.s_addr = INADDR_ANY;
        if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
        {
            int e = errno;
            cerr << "error " << e << " on bind: "
                 << strerror(e) << endl;
            return;
        }
        listen(fd,1);
    }
    else
    {
        if (inet_aton(host.c_str(), &sa.sin_addr) == 0)
        {
            cerr << "error converting ipaddr (" << host << ")" << endl;
            return;
        }
        // relegate the connect to the thread
    }
    pthread_t id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&id, &attr, _thread_entry, (void*) this);
    pthread_attr_destroy(&attr);
}

PfkMsgr :: ~PfkMsgr(void)
{
    if (threadRunning)
        cerr << "ERROR : call PfkMsg::stop before deleting!\n";
    if (fd > 0)
        close(fd);
    close(closerPipe[0]);
    close(closerPipe[1]);
}

void
PfkMsgr :: stop(void)
{
    if (threadRunning)
    {
        char dummy = 0;
        if (write(closerPipe[1],&dummy,1) < 0)
            fprintf(stderr, "PfkMsgr :: stop: write failed\n");
        while (threadRunning)
            // xxx need timeout
            usleep(1);
    }
}

//static
void *
PfkMsgr :: _thread_entry(void *arg)
{
    PfkMsgr * obj = (PfkMsgr *)arg;
    obj->threadRunning = true;
    obj->thread_main();
    obj->threadRunning = false;
    return NULL;
}

void
PfkMsgr :: thread_main(void)
{
    bool mainDone = false;
    fd_set  rfds;
    int maxfd;
    int cc;
    while (!mainDone)
    {
        dataFd = -1;
        if (isServer)
        {
            // wait for accept or close
            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);
            FD_SET(closerPipe[0], &rfds);
            if (closerPipe[0] > fd)
                maxfd = closerPipe[0];
            else
                maxfd = fd;
            cc = select(maxfd+1, &rfds, NULL, NULL, NULL);
            if (cc > 0)
            {
                if (FD_ISSET(closerPipe[0], &rfds))
                    mainDone = true;
                else if (FD_ISSET(fd, &rfds))
                {
                    socklen_t salen = sizeof(sa);
                    dataFd = accept(fd, (struct sockaddr *)&sa, &salen);
                    if (dataFd < 0)
                    {
                        int e = errno;
                        cerr << "error " << e << " in accept: "
                             << strerror(e) << endl;
                    }
                }
            }
        }
        else
        {
            // attempt connect or wait for close if fail
            dataFd = socket(AF_INET, SOCK_STREAM, 0);
            if (dataFd < 0)
            {
                int e = errno;
                cerr << "error " << e << " making socket: "
                     << strerror(e) << endl;
            }
            else
            {
                if (connect(dataFd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
                {
                    int e = errno;
                    cerr << "error " << e << " in connect: "
                         << strerror(e) << endl;
                    close(dataFd);
                    dataFd = -1;
                }
            }
        }
        if (dataFd > 0)
        {
            bool connectionDone = false;
            handle_connected();
            FD_ZERO(&rfds);
            rcvBufferSize = 0;
            fcntl(dataFd, F_SETFL,
                  fcntl(dataFd, F_GETFL, 0) | O_NONBLOCK);
            while (!connectionDone)
            {
                FD_SET(dataFd, &rfds);
                FD_SET(closerPipe[0], &rfds);
                if (closerPipe[0] > dataFd)
                    maxfd = closerPipe[0];
                else
                    maxfd = dataFd;
                cc = select(maxfd+1, &rfds, NULL, NULL, NULL);
                if (cc <= 0)
                    continue;
                if (FD_ISSET(dataFd, &rfds))
                {
                    int e;
                    int rdcc = read(dataFd, 
                                    rcvBuffer + rcvBufferSize,
                                    RCV_BUFFER_SIZE - rcvBufferSize);
                    e = errno;
                    if (rdcc <= 0)
                    {
                        cerr << "read returned " << rdcc << " err "
                             << e << " : " << strerror(errno) << endl;
                        connectionDone = true;
                    }
                    else
                    {
                        rcvBufferSize += rdcc;
                        uint32_t remaining = rcvBufferSize;
                        uint32_t consumed;
                        int pos = 0;
                        bool isMsg = true;
                        while (remaining > 0 && isMsg)
                        {
                            isMsg = rcvd_msg.parse(rcvBuffer + pos,
                                                   remaining,
                                                   &consumed);
                            if (isMsg)
                                handle_msg(rcvd_msg);
                            pos += consumed;
                            remaining -= consumed;
                            if (!isMsg && consumed > 0)
                            {
                                // meh. corrupt. bail.
                                cout << "rcvd corrupt msg\n";
                                connectionDone = true;
                            }
                        }
                        if (remaining > 0)
                        {
                            // save unused portion for next time.
                            if (pos > 0)
                                memmove(rcvBuffer,
                                        rcvBuffer + pos,
                                        remaining);
                        }
                        rcvBufferSize = remaining;
                        if (rcvBufferSize == RCV_BUFFER_SIZE)
                        {
                            cerr << "rcv buffer full: oversized msg?\n";
                            connectionDone = true;
                        }
                    }
                }
                if (FD_ISSET(closerPipe[0], &rfds))
                {
                    connectionDone = true;
                    mainDone = true;
                }
            }
            close(dataFd);
            dataFd = -1;
            {
                WaitUtil::Lock key(&lock);
                unsent_portion.clear();
            }
            handle_disconnected();
        }
        else
        {
            // sleep 1 second, waiting for close
            FD_ZERO(&rfds);
            FD_SET(closerPipe[0], &rfds);
            maxfd = closerPipe[0];
            struct timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            if (select(maxfd+1, &rfds, NULL, NULL, &tv) > 0)
                mainDone = true;
        }
    }
}

bool
PfkMsgr :: send_msg(const PfkMsg &msg)
{
    if (dataFd < 0)
        return false;
    uint32_t len;
    const void * ptr = msg.getBuffer(&len);
    return send_buf(ptr,len);
}

bool
PfkMsgr :: send_buf(const void *_buf, int len)
{
    const char * buf = (const char *) _buf;
    int writecc, e;
    WaitUtil::Lock key(&lock);
    if (dataFd < 0)
    {
        unsent_portion.clear();
        return false;
    }
    while (unsent_portion.length() > 0)
    {
        writecc = write(dataFd,
                        unsent_portion.c_str(),
                        unsent_portion.length());
        e = errno;
        if (writecc > 0)
            unsent_portion.erase(0,writecc);
        else
        {
            if (writecc < 0)
                if (e != EWOULDBLOCK)
                    cerr << "write " << len << " bytes returns " << writecc
                         << " err " << e << " : " << strerror(errno) << endl;
            return false;
        }
    }
    writecc = write(dataFd, buf, len);
    e = errno;
    if (writecc != (int)len)
    {
        if (writecc <= 0)
            if (e != EWOULDBLOCK)
                cerr << "write " << len << " bytes returns " << writecc
                     << " err " << e << " : " << strerror(errno) << endl;
        if (writecc > 0)
        {
            unsent_portion.assign((char*)buf+writecc, len-writecc);
            return true;
        }
        return false;
    }
    return true;
}
