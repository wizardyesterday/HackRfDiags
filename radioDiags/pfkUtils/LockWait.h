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

#ifndef __LOCKABLE_H__
#define __LOCKABLE_H__

#include <pthread.h>
#include <time.h>

#include "BackTrace.h"

/** a collection of mutex locks and waiter/semaphore abstractions */
namespace WaitUtil {

/** errors that might be thrown during certain operations */
struct LockableError : BackTraceUtil::BackTrace {
    /** the possible errors that might get thrown */
    enum LockableErrValue {
        MUTEX_LOCKED_IN_DESTRUCTOR,  //!< dont destroy a lock while its locked
        RECURSION_ERROR,             //!< this sucks
        __NUMERRS
    } err;
    static const std::string errStrings[__NUMERRS];
    LockableError(LockableErrValue _e) : err(_e) { }
    /** returns a descriptive string for the error
     * \return a descriptive string matching the err */
    /*virtual*/ const std::string _Format(void) const;
};

#define LOCKABLERR(e) throw LockableError(LockableError::e)

/** a mutex container. you can either derive from this or
 * instantiate it on its own. */
class Lockable {
    friend class Waiter;
    friend class Lock;
    pthread_mutex_t  lockableMutex;
    bool   locked;
    void   lock(void) throw ();
    void unlock(void) throw ();
public:
    /** constructor initializes the mutex to an unlocked state */
    Lockable(void) throw ();
    /** destructor checks that the mutex is not locked.
     * \throw may throw LockableError if the mutex is locked. */
    ~Lockable(void) throw (LockableError);
    /** indicates if the lock is held or not. 
     * \return true if the mutex is currently locked.
     * \note this is itself not protected so its only advisory;
     *     by the time you do something based on this return value,
     *     it may have changed. really only useful for catching errors
     *     like not locking something that should be locked. */
    bool isLocked() const throw ();
};

/** an instance of a lock (a critical region) should be framed
 * by this object's lock. all occurrances protecting some shared
 * data must reference the same Lockable object associated with
 * that shared data. */
class Lock {
    friend class Waiter;
    Lockable *lobj;
    int lockCount;
public:
    /** constructor; by default, the constructor locks the 
     * referenced mutex.
     * \param _lobj the Lockable that we are locking.
     * \param dolock indicates whether constructor should do the
     *   locking (implies destructor will guarantee an unlock). 
     *   defaults to true. */
    Lock( Lockable *_lobj, bool dolock=true );
    /** destructor makes sure the Lockable is unlocked before returning */
    ~Lock(void);
    /** a lock may be locked at any time if it is currently unlocked. */
    void lock(void) throw ();
    /** a lock may be unlocked at any time if it is currently locked. */
    void unlock(void) throw (LockableError);
};

/** an object which can be waited for by a waiter. you can either
 * derive from this or instantiate it on its own.
 * \note since a Waitable IS a Lockable, don't derive from both. */
class Waitable : public Lockable {
    friend class Waiter;
    pthread_cond_t  waitableCond;
public:
    Waitable(void);
    ~Waitable(void);
    /** if anyone is waiting on this object, signal one of them to wake up.
     * \note there's no control over which one wakes up; it is up to the
     *     operating system to pick one of the waiters */
    void waiterSignal(void);
    /** if anyone is waiting on this object, signal all of them to wake up. */
    void waiterBroadcast(void);
};

/** when someone wants to wait for a Waitable, use this to do the waiting. */
class Waiter : public Lock {
    Waitable * wobj;
public:
    /** constructor must reference the Waitable.
     * \param _wobj the Waitable to wait for.
     *   \note nothing is done to the Waitable yet. */
    Waiter(Waitable * _wobj);
    ~Waiter(void);
    /** wait until signal, no conditions. */
    void wait(void);
    /** wait until the given time for a wakeup signal.
     * \param expire the absolute clock time of the expiry (i.e.
     *     clock_gettime plus relative amount of time).
     * \return false if timeout, true if signaled. */
    bool wait(struct timespec *expire);
    /** wait for a specific amount of time using int sec/int nanosec.
     * \param sec the amount of time to wait in seconds
     * \param nsec the amount of time to wait in nanoseconds
     * \return false if timeout, true if signaled. */
    bool wait(int sec, int nsec);
};

/** a classical counting semaphore, like P and V from school */
class Semaphore {
    int value;
    Waitable semawait;
public:
    /** constructor initializes semaphore value to 0 */
    Semaphore(void);
    ~Semaphore(void);
    /** the value can be reinitialized at any time; note the 
     * behavior of anyone blocked in take is undefined.
     * \param init_val the value to set the semaphore counter to */
    void init(int init_val);
    /** give one count; if someone is blocked in take (value is 0),
     * wake them up. otherwise, value is incremented by one. */
    void give(void);
    /** try to take one count; if value is greator than 0, it will
     * be decremented by 1 and this will return immediately. otherwise
     * the thread will block until the specified absolute time waiting
     * for a give.
     * \param expire the absolute time at which to give up waiting
     *     (i.e. clock_gettime plus a relative amount of time). if this
     *     param is NULL, wait forever.
     * \return true if the give occurred, false if the expire time
     *      was reached. */
    bool take(struct timespec * expire);
    /** the same as take(NULL).
     * \return true if the give occurred, false if the expire time
     *      was reached. */
    bool take(void) { return take(NULL); }
};

// inline impl below this line

inline Lockable::Lockable(void) throw ()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&lockableMutex, &attr);
    pthread_mutexattr_destroy(&attr);
    locked = false;
}

inline Lockable::~Lockable(void) throw (LockableError)
{
    if (locked)
        LOCKABLERR(MUTEX_LOCKED_IN_DESTRUCTOR);
    pthread_mutex_destroy(&lockableMutex);
}

inline void
Lockable::lock(void) throw ()
{
    pthread_mutex_lock  (&lockableMutex);
    locked = true;
}

inline void
Lockable::unlock(void) throw ()
{
    locked = false;
    pthread_mutex_unlock(&lockableMutex);
}

inline bool
Lockable::isLocked(void) const throw ()
{
    return locked;
}

inline Lock::Lock( Lockable *_lobj, bool dolock /*=true*/ )
    : lobj(_lobj)
{
    lockCount = 0;
    if (dolock)
    {
        lockCount++;
        lobj->lock();
    }
}

inline Lock::~Lock(void)
{
    if (lockCount > 0)
        lobj->unlock();
}

inline void
Lock::lock(void) throw ()
{
    if (lockCount++ == 0)
        lobj->lock();
}

inline void
Lock::unlock(void) throw (LockableError)
{
    if (lockCount <= 0)
        LOCKABLERR(RECURSION_ERROR);
    if (--lockCount == 0)
        lobj->unlock();
}

inline Waitable::Waitable(void)
{
    pthread_condattr_t   cattr;
    pthread_condattr_init( &cattr );
    pthread_cond_init( &waitableCond, &cattr );
    pthread_condattr_destroy( &cattr );
}

inline Waitable::~Waitable(void)
{
    pthread_cond_destroy( &waitableCond );
}

inline void
Waitable::waiterSignal(void)
{
    pthread_cond_signal(&waitableCond);
}

inline void
Waitable::waiterBroadcast(void)
{
    pthread_cond_broadcast(&waitableCond);
}

inline Waiter::Waiter(Waitable * _wobj)
    : Lock(_wobj), wobj(_wobj)
{
}

inline Waiter::~Waiter(void)
{
}

inline void
Waiter::wait(void)
{
    pthread_cond_wait(&wobj->waitableCond, &lobj->lockableMutex);
}

// return false if timeout, true if signaled
inline bool
Waiter::wait(struct timespec *expire)
{
    int ret = pthread_cond_timedwait(&wobj->waitableCond,
                                     &lobj->lockableMutex, expire);
    if (ret != 0)
        return false;
    return true;
}

// return false if timeout, true if signaled
inline bool
Waiter::wait(int sec, int nsec)
{
    if (nsec > 1000000000)
    {
        sec += nsec / 1000000000;
        nsec = nsec % 1000000000;
    }
    struct timespec  expire;
    clock_gettime(CLOCK_REALTIME, &expire);
    expire.tv_sec += sec;
    expire.tv_nsec += nsec;
    if (expire.tv_nsec > 1000000000)
    {
        expire.tv_nsec -= 1000000000;
        expire.tv_sec += 1;
    }
    return wait(&expire);
}

inline Semaphore::Semaphore(void)
{
    value = 0;
}

inline Semaphore::~Semaphore(void)
{
}

inline void
Semaphore::init(int init_val)
{
    value = init_val;
}

inline void
Semaphore::give(void)
{
    {
        Lock  lock(&semawait);
        value++;
    }
    semawait.waiterSignal();
}

inline bool
Semaphore::take(struct timespec * expire)
{
    Waiter  waiter(&semawait);
    while (value <= 0)
    {
        if (expire)
        {
            if (waiter.wait(expire) == false)
                return false;
        }
        else
            waiter.wait();
    }
    value--;
    return true;
}

#undef   LOCKABLERR

}; // namespace HSMWait

#endif /* __LOCKABLE_H__ */
