#if 0
set -e -x
g++ -g3 -DCHILDPROCESSMANAGERTESTMAIN childprocessmanager.cc LockWait.cc -o cpm -lpthread
exit 0
#endif

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

#include "childprocessmanager.h"
#include "bufprintf.h"
#include "posix_fe.h"

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>

#include <iostream>

using namespace std;

namespace ChildProcessManager {

/************************ Handle ******************************/

Handle :: Handle(void)
{
    if (pipe(fromChildPipe) < 0)
        cerr << "ChildProcessManager::Handle pipe 1 failed\n";
    if (pipe(toChildPipe) < 0)
        cerr << "ChildProcessManager::Handle pipe 2 failed\n";
    open = false;
    fds[0] = fromChildPipe[0];
    fds[1] = toChildPipe[1];
    pid = -1;
    inManager = false;
}

Handle :: ~Handle(void)
{
    if (inManager)
        cerr << "ChildProcessManager::~Handle: still in Manager's map! "
             << "did you delete Handle before child had exited?"
             << endl;
    open = false;
#define CLOSEFD(fd) if (fd != -1) { close(fd); fd = -1; }
    CLOSEFD(fromChildPipe[0]);
    CLOSEFD(fromChildPipe[1]);
    CLOSEFD(toChildPipe[0]);
    CLOSEFD(toChildPipe[1]);
}

bool
Handle :: createChild(void)
{
    return Manager::instance()->createChild(this);
}

/************************ Manager ******************************/

Manager * Manager::_instance = NULL;

Manager * Manager::instance(void)
{
    if (_instance == NULL)
        _instance = new Manager();
    return _instance;
}

void
Manager :: cleanup(void)
{
    if (_instance != NULL)
    {
        delete _instance;
        _instance = NULL;
    }
}

Manager :: Manager(void)
{
    struct sigaction sa;
    sa.sa_handler = &Manager::sigChildHandler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDSTOP | SA_RESTART;
    sigaction(SIGCHLD, &sa, &sigChildOact);
    if (pipe(signalFds) < 0)
        cerr << "ChildProcessManager::Manager pipe 1 failed\n";
    if (pipe(rebuildFds) < 0)
        cerr << "ChildProcessManager::Manager pipe 2 failed\n";
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&notifyThreadId, &attr,
                   &Manager::notifyThread, (void*) this);
    pthread_attr_destroy(&attr);
}

Manager :: ~Manager(void)
{
    char dummy = 2; // code 2 means die
    if (write(rebuildFds[1], &dummy, 1) < 0)
        cerr << "ChildProcessManager::~Manager: write failed\n";
    sigaction(SIGCHLD, &sigChildOact, NULL);
    void *threadRet = NULL;
    pthread_join(notifyThreadId, &threadRet);
    close(signalFds[0]);
    close(signalFds[1]);
    close(rebuildFds[0]);
    close(rebuildFds[1]);
}

bool
Manager :: createChild(Handle *handle)
{
    int forkErrorPipe[2];
    if (pipe(forkErrorPipe) < 0)
        cerr << "createChild: pipe failed\n";

    sigset_t  oldset, newset;
    sigfillset(&newset);
    pthread_sigmask(SIG_SETMASK, &newset, &oldset);

    // google "vfork considered dangerous". the article on
    // EWONTFIX is particularly informative.
    handle->pid = fork();
    if (handle->pid < 0)
    {
        int e = errno;
        cerr << "fork: " << e << ": " << strerror(errno) << endl;
        sigprocmask(SIG_SETMASK, &oldset, NULL);
        return false;
    }

    if (handle->pid == 0)
    {
        // child

        // move pipe ends we do use to the
        // fd numbers where they're used by
        // the child.

        dup2(handle->toChildPipe[0], 0);
        dup2(handle->fromChildPipe[1], 1);
        dup2(handle->fromChildPipe[1], 2);

        // don't allow the child to inhert any
        // "interesting" file descriptors. note this
        // also closes all the pipe ends we don't use in the child.
        for (int i = 3; i < sysconf(_SC_OPEN_MAX); i++)
            if (i != forkErrorPipe[1])
                close(i);

        // mark the 'fork error' pipe as close-on-exec,
        // so if the exec succeeds, the parent gets a zero
        // read, but if it fails, it gets a 1-read.
        fcntl(forkErrorPipe[1], F_SETFD, FD_CLOEXEC);

        execvp(handle->cmd[0], (char *const*)handle->cmd.data());

        // dont print stuff! fork is weird, don't trust anything
        // to work.

        // send the errno to parent.
        int e = errno;
        if (write(forkErrorPipe[1], &e, sizeof(int)) < 0)
            cerr << "write to forkerror pipe failed\n";

        // call _exit because we dont want to call any atexit handlers.
        _exit(99);
    }
    //parent

    // close pipe ends we don't use.
    CLOSEFD(handle->fromChildPipe[1]);
    CLOSEFD(handle->toChildPipe[0]);
    close(forkErrorPipe[1]);

    bool ret = false;
    int e;
    int cc = read(forkErrorPipe[0], &e, sizeof(e));
    close(forkErrorPipe[0]);

    pthread_sigmask(SIG_SETMASK, &oldset, NULL);

    if (cc == 0)
    {
        // zero read means the pipe was closed-on-exec,
        // so child success.
        handle->open = true;
        {
            WaitUtil::Lock key(&handleLock);
            openHandles[handle->pid] = handle;
            handle->inManager = true;
        } // key destroyed here
        // wake up notify thread so it knows to
        // rebuild its fd_sets to include the 
        // new handle.
        char dummy = 1; // code 1 means rebuild
        if (write(rebuildFds[1], &dummy, 1) < 0)
            cerr << "rebuildFds write failed\n";
        ret = true;
    }
    else
    {
        // nonzero read means exec failed in the child.
        cerr << "execvp " << handle->cmd[0]
             << " failed with error " << e << ": "
             << strerror(e) << endl;
    }

    return ret;
}

//static
void
Manager :: sigChildHandler(int s)
{
    struct signalMsg msg;
    do {
        msg.pid = waitpid(/*wait for any child*/-1,
                          &msg.status, WNOHANG);

        if (msg.pid > 0)
        {
            if (0) // debug
            {
                Bufprintf<80>  bufp;
                bufp.print("sig handler got pid %d died, status %d\n",
                           msg.pid, msg.status);
                bufp.write(1);
            }
            if (_instance != NULL)
                // dont do any data structure manipulations here;
                // it is difficult to mutex between threads and 
                // signal handlers. (it can be done with sigprocmask but
                // that is expensive.) instead send a message to the
                // notify thread and let it deal with it in thread
                // context.
                if (write(_instance->signalFds[1], &msg, sizeof(msg)) < 0)
                    cerr << "sig handler write failed\n";
        }

    } while (msg.pid > 0);
}

//static
void *
Manager :: notifyThread(void *arg)
{
    Manager * mgr = (Manager *) arg;
    mgr->_notifyThread();
    return NULL;
}

void
Manager :: _notifyThread(void)
{
    bool done = false;
    std::vector<Handle*> handles;
    char buffer[4096];
    int cc;
    int maxfd;
    HandleMap::iterator it;
    Handle * h;
    struct signalMsg msg;
    char dummy;

    while (!done)
    {
        pxfe_select sel;

        sel.rfds.set(signalFds[0]);
        sel.rfds.set(rebuildFds[0]);

        { // add fds for all open handles to the fd_set.
            WaitUtil::Lock key(&handleLock);
            for (it = openHandles.begin();
                 it != openHandles.end();
                 it++)
            {
                h = it->second;
                if (h->open)
                    sel.rfds.set(h->fds[0]);
            }
        } // key destroyed here

        sel.tv.set(10,0);
        sel.select();

        if (sel.rfds.isset(rebuildFds[0]))
        {
            if (read(rebuildFds[0], &dummy, 1) != 1)
                done = true;
            else
                if (dummy == 2) // die code
                    // must be the manager destructor
                    done = true;

            if (0) // debug
            {
                Bufprintf<80> bufp;
                bufp.print("thread awakened on rebuild fd, done = %d\n",
                           done);
                bufp.write(1);
            }
            // otherwise this is just meant to make sure we rebuild
            // our fd_sets because the openHandles map has changed.
        }
        if (!done && sel.rfds.isset(signalFds[0]))
        {
            // the sigchld handler has sent us a pid and status.
            cc = read(signalFds[0], &msg, sizeof(msg));
            if (cc <= 0)
                done = true;
            else
            {
                if (0) // debug
                {
                    Bufprintf<80> bufp;
                    bufp.print("got msg pid %d died status %d\n",
                               msg.pid, msg.status);
                    bufp.write(1);
                }
                WaitUtil::Lock key(&handleLock);
                it = openHandles.find(msg.pid);
                if (it != openHandles.end())
                {
                    h = it->second;
                    h->open = false;
                    h->inManager = false;
                    openHandles.erase(it);
                    h->processExited(msg.status);
                }
            } // key destroyed here
        }
        if (!done)
        { // check all open handles
            // dont hold the key while calling h->handleOutput.
            // build the list of all handles that need servicing
            // first, then release the key.
            {
                WaitUtil::Lock key(&handleLock);
                for (it = openHandles.begin();
                     it != openHandles.end();
                     it++)
                {
                    h = it->second;
                    if (sel.rfds.isset(h->fds[0]))
                        handles.push_back(h);
                }
            } // key destroyed here
            for (unsigned int ind = 0; ind < handles.size(); ind++)
            {
                h = handles[ind];
                cc = read(h->fds[0], buffer, sizeof(buffer));
                if (cc > 0)
                    // call user's virtual method to handle the data.
                    h->handleOutput(buffer, cc);
                else
                    h->open = false;
            }
            handles.clear();
        }
    }
}

}; /* namespace ChildProcessManager */
