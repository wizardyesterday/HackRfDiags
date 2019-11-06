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

#ifndef __FDTHREADLAUNCHER_H__
#define __FDTHREADLAUNCHER_H__

#include <iostream>
#include <inttypes.h>

#ifdef __GNUC__
# if __GNUC__ >= 6
#  define ALLOW_THROWS noexcept(false)
# else
#  define ALLOW_THROWS
# endif
#else
# define ALLOW_THROWS
#endif

/** utility class for managing a file descriptor with a thread.
 * this class spawns a thread internally and calls 'select' on
 * the provided file descriptor, calling user callback methods
 * when 'select' wakes up for read or write. it is up to the user's
 * methods to choose how to handle the read or write.
 */
class fdThreadLauncher {
public:
    enum fdThreadState {
        INIT, STARTING, RUNNING, STOPPING, DEAD
    } state;
private:
    bool threadRunning;
    pthread_t threadId;
    enum { CMD_CLOSE, CMD_CHANGEPOLL };
    int startSyncFds[2];
    int cmdFds[2];
    static void * _threadEntry(void *);
    void threadEntry(void);
    int pollInterval;
protected:
    /** the fd this thread/object is managing. you dont have to 
     * fill this in, it will be populated by startFdThread */
    int fd;

    /** whenever the thread is going to enter select, it is going
     * to ask the user what to select for by calling this method.
     * \note this method is called in the context of this object's helper
     *    thread, thus mutex locking on shared data may be required.
     * \param forRead the user's method should set *forRead to true if
     *    the user wants the thread to select for read; if select wakes
     *    up because a descriptor needs reading, the thread will then
     *    call the user's handleReadSelect method. if *forRead is set
     *    to false, handleReadSelect will never be called.
     * \param forWrite the user's method should set *forWrite to true if
     *    the user wants the thread to select for write; if select wakes
     *    up because a descriptor has space for writing, the thread will then
     *    call the user's handleWriteSelect method. if *forWrite is set
     *    to false, handleWriteSelect will never be called.
     * \return if the user wants this connection to close, return false,
     *    else return true. */
    virtual bool doSelect(bool *forRead, bool *forWrite) = 0;

    /** if select indicates a descriptor has data waiting to read,
     * the thread will call this method implemented by the user.
     * it is expected the user will handle this event, i.e. by calling
     * read(2) or, if it is a listening tcp socket, by calling accept(2).
     * \note this method is called in the context of this object's helper
     *    thread, thus mutex locking on shared data may be required.
     * \param fd the fd is passed as an arg; note it is the same fd that
     *    is a member of this class.
     * \return if the user wants this connection to close, return false,
     *    else return true. */
    virtual bool handleReadSelect(int fd) = 0;

    /** if select indicates a descriptor has room for writing,
     * the thread will call this method implemented by the user.
     * it is expected the user will handle this event, i.e. by calling
     * write(2) or sendto(2) (for a udp socket).
     * \note this method is called in the context of this object's helper
     *    thread, thus mutex locking on shared data may be required.
     * \note it is not required that the user always use forWrite and
     *    handleWriteSelect; the user can just call write(2) directly on
     *    the fd member of this class, if the user doesn't care about
     *    possible blocking time on the descriptor.
     * \note if writes are done directly onto this object's fd, the user
     *    must either (a) never mix-and-match direct writes with use of 
     *    handleWriteSelect, or (b) add mutexing to ensure the data written
     *    to fd is done so in the proper interleaving.
     * \param fd the fd is passed as an arg; note it is the same fd that
     *    is a member of this class.
     * \return if the user wants this connection to close, return false,
     *    else return true. */
    virtual bool handleWriteSelect(int fd) = 0;

    /** perform periodic poll. if startFdThread was given a positive
     * pollInterval, or setPollInterval was given a positive value,
     * the helper thread will make a best-effort attempt to call this
     * method on that interval.
     * \note this method is called in the context of this object's helper
     *    thread, thus mutex locking on shared data may be required.
     * \note the pollInterval will not be exact; no real-time guarantees
     *     are provided.
     * \return if the user wants this connection to close, return false,
     *    else return true. */
    virtual bool doPoll(void) = 0;

    /** this method is called when the helper thread is exiting due to
     * one of the user handler functions returning false, once the helper
     * thread has completed its own cleanup.
     * \note this method is called in the context of this object's helper
     *    thread, thus mutex locking on shared data may be required.
     * \note this method is not called in the case of the user invoking
     *    the stopFdThread method or the destructor (it is assumed if the
     *    user is invoking the destructor, he already knows the connection
     *    is going to close).
     */
    virtual void done(void) = 0;

public:

    /** constructor initializes internal state. */
    fdThreadLauncher(void);

    /** destructor stops the thread if it is running and releases
     * most resources.
     * \note the fd is NOT closed by this method! this is left up to
     *    the user's derived virtual destructor, if required. */
    virtual ~fdThreadLauncher(void) ALLOW_THROWS;

    /** start the thread, managing the given descriptor.
     * \note this function blocks until the thread is confirmed to have
     *    started and taken control of the descriptor.
     * \warning if you derived an object from this class, you
     *    CANNOT call startFdThread from your derived class
     *    constructor!  you may enter a race where the fdThread
     *    receives a message and calls handleReadSelect before
     *    your constructor completes, causing a null virtual
     *    function pointer exception.
     * \param _fd the descriptor to manage.  this is copied into 
     *    this object's fd data member.
     * \param _pollInterval if desired, the doPoll will be called on
     *    a regular interval. units are milliseconds; granularity depends
     *    on the capability of the kernel. -1 means disable polling. */
    void startFdThread(int _fd, int _pollInterval = -1);

    /** the user may call this to terminate the thread and the connection.
     * \note this function blocks until it is confirmed that the thread has
     *     exited (thus if any of the user's virtual methods take time or 
     *     block, this function may take at least that long).
     * \note this function does NOT close the descriptor.  that is left to
     *     the user to decide when is appropriate to close fd. */
    void stopFdThread(void);

    /** once startFdThread has been called, the poll interval can be changed.
     * \param _pollInterval set to a positive value (in milliseconds)
     *      to start the thread periodically calling the user's doPoll
     *      method; set to -1 to stop polling. */
    void setPollInterval(int _pollInterval);

    /** a shortcut helper that calls accept(2). only useful if the
     * managed fd is a tcp listener, and this should be called from
     * handleReadSelect.
     * \return a new descriptor for the accepted connection, or -1 
     *    if there was an error (errno is set to the error codes for
     *    accept(2)). */
    int acceptConnection(void);

    /** a helper function to make a listening tcp socket.
     * \note this is a static method that does not depend on
     *    any data in this class.
     * \param port the tcp port number to listen on.
     * \return a descriptor, or -1 if there was a failure. if this
     *     returns -1, errno is set to any of the possible error codes
     *     for socket(2) or bind(2). */
    static int makeListeningSocket(int port);

    /** a helper function for trying to connect to a listening tcp port.
     * \note this is a static method that does not depend on
     *    any data in this class.
     * \note this is a blocking call that does not return until
     *    success or failure is determined (i.e. connection attempt to
     *    a host which is currently offline will take a couple of minutes
     *    to properly time out at the TCP protocol layer).
     * \param ip  the ip address in native byte order form, i.e.
     *    address 192.168.1.1 would be 0xC0A80101.
     * \param port the tcp port number to attempt to connect to.
     * \return a descriptor for the new connection, or -1 if failure.
     *    in case of failure, errno is set to any possible error code
     *    for socket(2) or connect(2). */
    static int makeConnectingSocket(uint32_t ip, int port);

    /** a helper function for trying to connect to a listening tcp port.
     * \note this is a static method that does not depend on
     *    any data in this class.
     * \note this is a blocking call that does not return until
     *    success or failure is determined (i.e. connection attempt to
     *    a host which is currently offline will take a couple of minutes
     *    to properly time out at the TCP protocol layer).
     * \param host the internet DNS hostname of the host you wish to
     *    connect to. this function will attempt to resolve the host name
     *    using the native system's DNS configuration.
     * \param port the tcp port number to attempt to connect to.
     * \return a descriptor for the new connection, or -1 if failure.
     *    in case of failure, errno is set to any possible error code
     *    for gethostbyname(2), socket(2), or connect(2). */
    static int makeConnectingSocket(const std::string &host, int port);
};
std::ostream &operator<<(std::ostream &ostr,
                         const fdThreadLauncher::fdThreadState state);

#endif /* __FDTHREADLAUNCHER_H__ */
