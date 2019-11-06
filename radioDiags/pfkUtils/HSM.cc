
/** \file HSM.cc */
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

#include "HSM.h"

using namespace HSM;

const std::string HSMError::errStrings [__NUMERRS] = {
    "user states should not handle HSM_PROBE",
    "bogus action type value",
    "initial function must return TRANS",
    "state entry handler must return HANDLED",
    "state exit handler must return HANDLED"
};

//virtual
const std::string
HSMError::_Format(void) const
{
    std::string ret = "HSM ERROR: ";
    ret += errStrings[type];
    ret += " at:\n";
    return ret;
}

HSMScheduler::HSMScheduler(void)
{
}

HSMScheduler::~HSMScheduler(void)
{
}

void
HSMScheduler::registerHSM(ActiveHSMBase *sm)
{
    WaitUtil::Lock lock(&active_hsms);
    active_hsms.add_tail(sm);
}

void
HSMScheduler::deregisterHSM(ActiveHSMBase *sm)
{
    WaitUtil::Lock lock(&active_hsms);
    active_hsms.remove(sm);
}

void
HSMScheduler::subscribe(ActiveHSMBase *sm, HSMEvent::Type type)
{
    WaitUtil::Lock lock(&subHash);
    HSMSubEntry * se;
    se = subHash.find(type);
    if (se == NULL)
    {
        se = new HSMSubEntry(type);
        subHash.add(se);
    }
    se->activeObjs.push_back(sm);
}

void
HSMScheduler::publish(HSMEvent * evt)
{
    WaitUtil::Lock lock(&subHash);
    HSMSubEntry * se = subHash.find((HSMEvent::Type)evt->type);
    if (se == NULL)
    {
        lock.unlock();
        std::cout << "no subscribers to event "
                  << evt->evtName()
                  << ", discarding\n";
        evt->ref();
        evt->deref();
    }
    else
    {
        for (size_t ind = 0; ind < se->activeObjs.size(); ind++)
        {
            se->activeObjs[ind]->enqueue(evt);
        }
    }
}

void
HSMScheduler::start(void)
{
    {
        WaitUtil::Lock  lock(&active_hsms);
        for (ActiveHSMBase * h = active_hsms.get_head();
             h != NULL;
             h = active_hsms.get_next(h))
        {
            h->init();
        }
    }
    HSMThread::Threads::start();
}

void
HSMScheduler::stop(void)
{
    HSMThread::Threads::cleanup();
}

HSMEventEnvelope *
HSMScheduler::allocEnv(void)
{
    return eventEnvelopePool.alloc(0,true);
}

void
HSMScheduler::releaseEnv(HSMEventEnvelope *env)
{
    eventEnvelopePool.release(env);
}

ActiveHSMBase::ActiveHSMBase(HSMScheduler * _sched,
                             const std::string &name)
    : Thread(name),
      sched(_sched),
      termEvent(HSMEvent::HSM_TERMINATE)
{
    sched->registerHSM(this);
}

//virtual
ActiveHSMBase::~ActiveHSMBase(void)
{
    sched->deregisterHSM(this);
}

void
ActiveHSMBase::entry(void)
{
    bool done = false;
    while (!done)
    {
        HSMEventEnvelope * env = eventQueue.dequeue(-1);
        AHSMdispatch(env->evt);
        if (env->evt->type == HSMEvent::HSM_TERMINATE)
            done = true;
        env->evt->deref();
        sched->releaseEnv(env);
    }
}

void
ActiveHSMBase::stopReq(void)
{
    // the termEvent doesn't come from a pool,
    // so we don't want deref trying to free it to
    // a nonexistent pool. give it an extra ref
    // so that it survives.
    termEvent.ref();
    enqueue(&termEvent);
}

void
ActiveHSMBase::enqueue(HSMEvent * evt)
{
    HSMEventEnvelope * env = sched->allocEnv();
    env->evt = evt;
    evt->ref();
    eventQueue.enqueue(env);
}
