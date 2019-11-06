/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/** \file  HSM.h */
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

#ifndef __HSM_H__
#define __HSM_H__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include "dll3.h"
#include "BackTrace.h"
#include "hsmthread.h"
#include "thread_slinger.h"

namespace HSM {

/** HSM-related errors that may be encountered */
struct HSMError : BackTraceUtil::BackTrace {
    /** list of HSM-related errors that might be thrown */
    enum HSMErrorType {
        HSMErrorHandleProbe,  //!< states must NOT handle __HSM_PROBE
        HSMErrorBogusAct,     //!< must return HANDLED, TRANS, TOP, SUPER only
        HSMErrorInitialTrans, //!< initial transition must return TRANS
        HSMErrorEntryHandler, //!< entry handlers must only return HANDLED
        HSMErrorExitHandler,  //!< exit handlers must only return HANDLED
        __NUMERRS
    } type;
    static const std::string errStrings[__NUMERRS];
    std::string str;
    HSMError(HSMErrorType t) : type(t) { }
    /** utility function to format printable string of error and backtrace */
    /*virtual*/ const std::string _Format(void) const;
};
   
/** base class for an HSMEvent, see HSMEventT and HSM_EVENT_DECLARE. */
struct HSMEvent : public ThreadSlinger::thread_slinger_message {
    HSMEvent(int _type) : type((HSMEvent::Type)_type) { }
    virtual ~HSMEvent(void) { }
    /** event name, user events should override this, although
     * this is done automatically by HSM_EVENT_DECLARE */
    virtual const std::string evtName(void) const { return "HSMEvent"; }
    /** type of the event, actually HSMEvent::Type */
    int type;
    /*virtual*/ const std::string msgName(void) { return evtName(); }
    /** some event types are reserved by the HSM library */
    enum Type {
        __HSM_PROBE,
        HSM_ENTRY,    //!< states handle this on entry, return HANDLED
        HSM_EXIT,     //!< states handle this on exit, return HANDLED
        HSM_TERMINATE,
        HSM_USER_START  //!< define your own event types starting at this value
    };
};

/** base template for a user event, see \ref HSM_EVENT_DECLARE for a simpler
 * way to define your own events.
 * \param T   the user's data type
 * \param typeVal  the enum value of your type starting at HSM_USER_START */
template <class T, int typeVal>
struct HSMEventT : public HSMEvent {
    HSMEventT(void) : HSMEvent(typeVal) { }
    virtual ~HSMEventT(void) { }
    /** allocate a new instance of an object from a memory pool; 
     * the memory pool grows automatically, see
     * \ref ThreadSlinger::thread_slinger_pools::report_pools */
    static T * alloc(void);
};

/** this is how to declare your own event, an easier way than HSMEventT.
 * \param __className  the name of the class of the event you're creating
 * \param __typeValue  a type value from an enum starting at HSM_USER_START
 * \param __body   optionally define contents of the event */
#define HSM_EVENT_DECLARE(__className,__typeValue,__body) \
struct __className : ::HSM::HSMEventT<__className,__typeValue>      \
{ \
    const std::string evtName(void) const { return #__className; } \
    __body \
}

/** base class for a state machine without a thread */
template <class T>
class HSM {
public:
    struct Action;
private:
    bool debug;
    enum ActType { ACT_HANDLED, ACT_TRANS, ACT_SUPER, ACT_TOP, NUMACTS };
    typedef Action (T::*State)(HSMEvent const *);
    static const std::string ActTypeNames[NUMACTS];
    State currentState;
    struct StateTraceEntry {
        State state;
        char const * name;
        StateTraceEntry(State _state, char const * _name);
    };
    typedef std::vector<StateTraceEntry> StateTrace;
    StateTrace     trace1;
    StateTrace     trace2;
    StateTrace  *  currentTrace;
    StateTrace  *  oldTrace;
    char const * stateName(State state,
                           State *nextState = NULL,
                           bool *top = NULL);
    void backtrace(StateTrace *traceret, State state);
    std::string trace2str(StateTrace *trace);
    HSMEvent  exitEvt;
    HSMEvent  entryEvt;
    HSMEvent  probeEvt;
public:
    /** a state will receive this when it is entered */
    static const HSMEvent::Type HSM_ENTRY = HSMEvent::HSM_ENTRY;
    /** a state will receive this when it is exited */
    static const HSMEvent::Type HSM_EXIT = HSMEvent::HSM_EXIT;
    /** a state should return this to indicate what is supposed
     * to be done; it should be one of 
     * \ref HANDLED
     * \ref TRANS
     * \ref SUPER
     * \ref TOP */
    struct Action {
        ActType act;
        State state;
        char const * name;
        Action(void) { }
        Action(ActType _act, State _state, char const *_name);
    };
    /** return this to indicate event handling is complete with
     * no action to be taken. */
    struct HANDLED : Action { HANDLED(void ); };
    /** return this to indicate the state machine should transition
     * to another state. argument is &myClass::nextState. */
    struct   TRANS : Action {   TRANS(State s); };
    /** return this to indicate this state function will not handle
     * this event, but the super (parent) state may handle this state.
     * this is also used to determine a string name for the current state.
     * arguments are &myClass::superState, std::string thisStateName */
    struct   SUPER : Action {   SUPER(State s, char const * __name); };
    /** return this from the top state to indicate this is the top state
     * (there is no super). */
    struct     TOP : Action {     TOP(void ); };
    HSM( bool _debug = false );
    virtual ~HSM(void) { }
    /** if debug is set to true, printouts will occur as messages
     * are dispatched and states transition */
    void set_debug(bool _debug) { debug = _debug; }
    /** call this to initialize this state machine; if this is an
     * ActiveHSM, do NOT call this (it is done automatically). */
    void HSMInit(void);
    /** user must implement an initial method. if this is an ActiveHSM,
     * this method should perform all subscribes. if not, you may do
     * whatever you wish, except it must return TRANS to the first state. */
    virtual Action initial(void) = 0;
    /** invoke this method to pass an event into the state machine.
     * if this is an ActiveHSM, do not use this, it is done automatically
     * by the underlying ActiveHSM thread */
    void dispatch(HSMEvent const * evt);
};

class ActiveHSMBase;

typedef DLL3::List<ActiveHSMBase,1> ActiveHSMList_t;

class HSMScheduler;

struct HSMEventEnvelope : public ThreadSlinger::thread_slinger_message {
    HSMEvent * evt;
    const std::string msgName(void) const { return "HSMEventEnvelope"; }
};

/** base class for an Active HSM object */
class ActiveHSMBase : public HSMThread::Thread,
                      public ActiveHSMList_t::Links
{
protected:
    HSMScheduler * sched;
    /*virtual*/ void entry(void);
    /*virtual*/ void stopReq(void);
    ThreadSlinger::thread_slinger_queue<HSMEventEnvelope>  eventQueue;
    HSMEvent termEvent;
public:
    ActiveHSMBase(HSMScheduler * _sched, const std::string &name);
    virtual ~ActiveHSMBase(void);
    virtual void init(void) = 0;
    virtual void AHSMdispatch(HSMEvent const * evt) = 0;
    /** one way to enqueue an event to this HSM thread,
     * see also ActiveHSM::publish */
    void enqueue(HSMEvent * env);
};

/** base template for an HSM thread, see \ref ACTIVE_HSM_DECLARE */
template <class T>
class ActiveHSM : public ActiveHSMBase, public HSM<T>
{
public:
    /** constructor for an ActiveHSM
     * \param __sched   a scheduler which must be declared first
     * \param name     a string name for this object (for debug)
     * \param __debug  whether internal debug is enabled */
    ActiveHSM( HSMScheduler * __sched, const std::string &name,
               bool __debug = false );
    virtual ~ActiveHSM(void);
protected:
    void init(void);
    void AHSMdispatch(HSMEvent const * evt);
    /** a user's HSM::initial method should call this to subscribe
     * to HSMEvent::Type types of interest */
    void subscribe(int type);
    /** user code may call this to publish messages, whoever is 
     * subscribed will receive it. */
    void publish(HSMEvent * evt);
};

#define ACTIVE_HSM_BASE(__className)  ::HSM::ActiveHSM<__className>
/** a user should use this to declare an Active HSM thread.
 * \param __className  the name of the class to declare */
#define ACTIVE_HSM_DECLARE(__className) \
    class __className : public ACTIVE_HSM_BASE(__className)

struct HSMSubEntry;
class HSMSubEntryHash;
typedef DLL3::Hash<HSMSubEntry,HSMEvent::Type,HSMSubEntryHash,1> HSMSubHash_t;

struct HSMSubEntry : public HSMSubHash_t::Links
{
    HSMEvent::Type type;
    std::vector<ActiveHSMBase*> activeObjs;
    HSMSubEntry(HSMEvent::Type _t) : type(_t) { }
    ~HSMSubEntry(void) throw () { }
};
class HSMSubEntryHash {
public:
    static uint32_t obj2hash(const HSMSubEntry &item)
    { return (uint32_t) item.type; }
    static uint32_t key2hash(const HSMEvent::Type key)
    { return (uint32_t) key; }
    static bool hashMatch(const HSMSubEntry &item, const HSMEvent::Type key)
    { return (item.type == key); }
};

/** a scheduler object, all ActiveHSM objs should be attached to this */
class HSMScheduler {
    ActiveHSMList_t  active_hsms;
    HSMSubHash_t  subHash;
    ThreadSlinger::thread_slinger_pool<HSMEventEnvelope>  eventEnvelopePool;
public:
    HSMScheduler(void);
    ~HSMScheduler(void);
    void registerHSM(ActiveHSMBase *);
    void deregisterHSM(ActiveHSMBase *);
    void subscribe(ActiveHSMBase *, HSMEvent::Type type);
    /** a message may be injected into a set of ActiveHSMs via this method */
    void publish(HSMEvent * evt);
    /** start all ActiveHSM obj threads and call their HSM::initial methods */
    void start(void);
    /** stop all ActiveHSM threads and issue HSMEvent::HSM_EXIT events to all
     * current states */
    void stop(void);
    HSMEventEnvelope *allocEnv(void);
    void releaseEnv(HSMEventEnvelope *env);
};

#include "HSM.tcc"

}; //namespace HSM

/** \mainpage HSM user's API documentation

This is the manual for the Heirarchical State Machine API.

Interesting classes:

<ul>
<li> \ref HSM::HSMEvent::Type
<li> \ref HSM::HSMEventT
<li> \ref HSM_EVENT_DECLARE
<li> \ref HSM::HSMScheduler
<li> \ref HSM::HSMError
<li> \ref HSMThread::ThreadError
</ul>

First you should declare an enum for all your event types.
Start at HSM_USER_START because values below that are reserved.

\code

enum MyHSMEVENTTYPE {
    KICKOFF = HSM::HSMEvent::HSM_USER_START,
    CONFIG,
    CONNECT,
    DISCON,
    AUTH,
    DUMMY
};

\endcode

Next, declare structs and struct bodys for these events. There are
two ways to do it. Examples of both are presented below.

\code

struct myKickoffEvt : HSM::HSMEventT<myKickoffEvt,KICKOFF>
{
    const std::string evtName(void) const { return "myKickoffEvt"; }
    int value3;
    int value4;
};

HSM_EVENT_DECLARE(myConfigEvt,CONFIG,
                  int value1;
                  int value2;
    );

\endcode

Note that each class defined (in this case myConfigEvt and myKickoffEvt)
are given a memory pool (supplied by the ThreadSlinger library) and a
static HSM::HSMEventT::alloc method to alloc new instances.  Each event
is also given a ref and deref method (see \ref
ThreadSlinger::thread_slinger_message::ref).  On the last deref, the
event buffer is automatically released back to the pool.

Next, declare and implement the state machine.  There are two types
of state machines available.

A simple HSM::HSM state machine is an object with methods.  The
HSM::HSM::dispatch method must be invoked on the object to pass an
event into that state machine.  You manage the memory on the event
objects and there is no threading.  The dispatch method simply returns
once your state transition function has completed.

An example of declaring and using a simple HSM::HSM state machine follows:

\code

class myStateMachine1 : public HSM<myStateMachine1>
{
    myStateMachine1(void) : HSM(true) { // true enables debug
        // some init code
    }
    ~myStateMachine1(void) {
        // some cleanup code
    }
    Action initial(void) { // virtual from base class
        // some init code
        return TRANS(&myStateMachine1::state1);
    }
    Action  top(HSMEvent const * ev) {
        switch (ev->type)
        {
            case HSM_ENTRY:
                // do some state entry stuff
                return HANDLED();
            case HSM_EXIT:
                // do some state exit stuff
                return HANDLED();
            case CONFIG:
                doSomething(dynamic_cast<myConfigEvt*>(ev)->value1);
                return TRANS(&myStateMachine2::state2);
            // etc
        }
        return TOP();
    }
    Action  state1(HSMEvent const * ev) {
        switch (ev->type)
        {
            case HSM_ENTRY:
                // do some state entry stuff
                return HANDLED();
            case HSM_EXIT:
                // do some state exit stuff
                return HANDLED();
            // etc
        }
        return SUPER(&myStateMachine1::top, "state1");
    }
    Action  state2(HSMEvent const * ev) {
        // etc
        return SUPER(&myStateMachine1::top, "state2");
    }
};

void somefunc(void) {
    myStateMachine1  m;
    m.HSMInit();
    myConfigEvt  cfg;
    cfg.value1 = something;
    cfg.value2 = something;
    m.dispatch(&cfg);
    //etc
}

\endcode

Every HSM must implement the "initial" method.  The "initial" method
\em must return TRANS to indicate the initial state.

A state must return SUPER or TOP if it does not understand a message.
SUPER indicates the parent state may support the message.  TOP indicates
it is the top-most state in the heirarchy.

The HSM system uses SUPER also to determine state names. (It is assumed
the TOP state is named "top".)  That is why SUPER has a mandatory string
argument.

States receive an HSM_ENTRY event when a TRANS causes that state to be
entered.  When a heirarchical state is entered, all states which have just
been entered receive HSM_ENTRY, in order from top-most to bottom-most state.
When a TRANS causes a set of states in the heirarchy to be exited, they all
receive HSM_EXIT events, in reverse order (bottom-most to top-most).  If a
TRANS causes a subset of the states to be the same in the before-set and 
after-set, they receive no ENTRY or EXIT events.

In the above example, "top" is the top-most state, and its two
substates are state1 and state2.  (Note that the parent-relationship is
established via the SUPER return values.)  When HSMInit is called, the "initial"
virtual method is called.  "initial" puts it in state1.  This causes
an ENTRY event to "top" state, then and ENTRY to "state1".  We are now in the
"top.state1" heirarchical state.  When the "CONFIG" event is injected,
state1 function is invoked. In this case, state1 does not have a handler
for CONFIG, so it falls to the SUPER.  The config event is then passed to
the SUPER state, which is "top".  Top has a handler for that event which
causes a TRANS to state2.  Since state2 also has a SUPER to "top", this means
"top" is not exited.  So an EXIT is sent to state1, and ENTRY is sent to
state2.  We are now in the "top.state2" state.

An HSM::ActiveHSM is more complex.  It is a thread plus a subscription-
management system.  Multiple ActiveHSMs may be created, and each
may subscribe to a set of HSM::HSMEvent::Type types it is interested in
receiving.  An HSM::HSMScheduler object must be created, and the ActiveHSMs
are registered with it.  HSMScheduler performs subscription management and
reference-counting on the events.

The following example shows how to use an ActiveHSM, and also demonstrates
both ways to declare an ActiveHSM.

\code

class myStateMachine1 : public ActiveHSM<myStateMachine1>
{
    myStateMachine1(HSMScheduler * _sched, bool __debug = false)
        : ActiveHSM<myStateMachine1>(_sched, "machine1", __debug)
    {
        // init code
    }
    ~myStateMachine1(void)
    {
        // cleanup code
    }
    Action initial(void) // virtual in base
    {
        subscribe(CONFIG);
        subscribe(CONNECT);
        subscribe(DISCON);
        subscribe(AUTH);
        subscribe(DUMMY);
        return TRANS(&myStateMachine1::unconfigured);
    }
    // here is the state heirarchy:
    //   top
    //      unconfigured
    //      configured
    //         init
    //         connected
    //            auth
    Action top(HSMEvent const * ev)
    {
        switch (ev->type)
        {
        case HSM_ENTRY:
            printf("ENTER 1: top\n");
            return HANDLED();
        case HSM_EXIT:
            printf("EXIT 1: top\n");
            return HANDLED();
        }
        return TOP();
    }
    Action unconfigured(HSMEvent const * ev)
    {
        switch (ev->type)
        {
        case HSM_ENTRY:
            printf("ENTER 1: top.unconfigured\n");
            return HANDLED();
        case HSM_EXIT:
            printf("EXIT 1: top.unconfigured\n");
            return HANDLED();
        case CONFIG:
            printf("got CONFIG in top.unconfigured\n");
            return TRANS(&myStateMachine1::init);
        }
        return SUPER(&myStateMachine1::top, "unconfigured");
    }
    Action configured(HSMEvent const * ev)
    {
        switch (ev->type)
        {
        case HSM_ENTRY:
            printf("ENTER 1: top.configured\n");
            return HANDLED();
        case HSM_EXIT:
            printf("EXIT 1: top.configured\n");
            return HANDLED();
        case CONNECT:
            printf("got CONNECT in top.configured.init\n");
            return TRANS(&myStateMachine1::connected);
        case DISCON:
            printf("got DISCON in top.configured\n");
            return TRANS(&myStateMachine1::unconfigured);
        }
        return SUPER(&myStateMachine1::top, "configured");
    }
    Action init(HSMEvent const * ev)
    {
        switch (ev->type)
        {
        case HSM_ENTRY:
            printf("ENTER 1: top.configured.init\n");
            return HANDLED();
        case HSM_EXIT:
            printf("EXIT 1: top.configured.init\n");
            return HANDLED();
        }
        return SUPER(&myStateMachine1::configured, "init");
    }
    Action connected(HSMEvent const * ev)
    {
        switch (ev->type)
        {
        case HSM_ENTRY:
            printf("ENTER 1: top.configured.connected\n");
            return HANDLED();
        case HSM_EXIT:
            printf("EXIT 1: top.configured.connected\n");
            return HANDLED();
        case AUTH:
            printf("got AUTH in top.configured.connected\n");
            return TRANS(&myStateMachine1::auth);
        }
        return SUPER(&myStateMachine1::configured, "connected");
    }
    Action auth(HSMEvent const * ev)
    {
        switch (ev->type)
        {
        case HSM_ENTRY:
            printf("ENTER 1: top.configured.connected.auth\n");
            return HANDLED();
        case HSM_EXIT:
            printf("EXIT 1: top.configured.connected.auth\n");
            return HANDLED();
        }
        return SUPER(&myStateMachine1::connected, "auth");
    }
};

ACTIVE_HSM_DECLARE(myStateMachine2)
{
public:
    myStateMachine2(HSMScheduler * _sched, bool __debug = false)
        : ACTIVE_HSM_BASE(myStateMachine2)(_sched, "machine2", __debug)
    {
    }
    virtual ~myStateMachine2(void)
    {
    }
    Action initial(void) // virtual in base
    {
        subscribe(KICKOFF);
        return TRANS(&myStateMachine2::top);
    }
    Action top(HSMEvent const * ev)
    {
        switch (ev->type)
        {
        case HSM_ENTRY:
            printf("ENTER 2: top\n");
            return HANDLED();
        case HSM_EXIT:
            printf("EXIT 2: top\n");
            return HANDLED();

        case KICKOFF:
            printf("sm2 got KICKOFF in top\n");

            printf("sm2 sleeping\n");
            sleep(1);
            printf("sm2 sending config\n");
            publish(myConfigEvt::alloc());

            printf("sm2 sleeping\n");
            sleep(1);
            printf("sm2 sending connect\n");
            publish(myConnectEvt::alloc());

            printf("sm2 sleeping\n");
            sleep(1);
            printf("sm2 sending auth\n");
            publish(myAuthEvt::alloc());

            printf("sm2 sleeping\n");
            sleep(1);
            printf("sm2 sending discon\n");
            publish(myDisconEvt::alloc());

            printf("sm2 sleeping\n");
            sleep(1);
            printf("sm2 sending dummy\n");
            publish(myDummyEvt::alloc());

            printf("sm2 sleeping\n");
            sleep(1);
            done = true;

            return HANDLED();
        }
        return TOP();
    }
};

void somefunc(void)
{
        HSMScheduler  sched;

        myStateMachine1  myHsm1(&sched, true);
        myStateMachine2  myHsm2(&sched, true);

        sched.start();

        sched.publish(MyTestApp::myKickoffEvt::alloc());

        while (done == false)
            sleep(1);

        sched.stop();

        ThreadSlinger::poolReportList_t report;
        ThreadSlinger::thread_slinger_pools::report_pools(report);

        std::cout << "pool report:\n";
        for (size_t ind = 0; ind < report.size(); ind++)
        {
            const ThreadSlinger::poolReport &r = report[ind];
            std::cout << "    pool "
                      << r.name
                      << " : "
                      << r.usedCount
                      << " used "
                      << r.freeCount
                      << " free\n";
        }
}

\endcode

 */

#endif /* __HSM_H__ */
