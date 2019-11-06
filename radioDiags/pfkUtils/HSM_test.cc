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

namespace MyTestApp {

enum MyHSMEVENTTYPE {
    KICKOFF = HSMEvent::HSM_USER_START,
    CONFIG,
    CONNECT,
    DISCON,
    AUTH,
    DUMMY
};

HSM_EVENT_DECLARE(myKickoffEvt,KICKOFF,);
HSM_EVENT_DECLARE(myConfigEvt,CONFIG,
                  int value1;
                  int value2;
    );
HSM_EVENT_DECLARE(myConnectEvt,CONNECT,);
HSM_EVENT_DECLARE(myDisconEvt,DISCON,);
HSM_EVENT_DECLARE(myAuthEvt,AUTH,);
HSM_EVENT_DECLARE(myDummyEvt,DUMMY,);

ACTIVE_HSM_DECLARE(myStateMachine1)
{
public:
    myStateMachine1(HSMScheduler * _sched, bool __debug = false)
        : ACTIVE_HSM_BASE(myStateMachine1)(_sched, "machine1", __debug)
    {
    }
    virtual ~myStateMachine1(void)
    {
    }
    /*virtual*/ Action initial(void)
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

bool done = false;

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
    /*virtual*/ Action initial(void)
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

}; // namespace MyTestApp

using namespace MyTestApp;

#define DO_TRY 1

int
main()
{
#if DO_TRY
    try {
#endif
        HSMScheduler  sched;

        MyTestApp::myStateMachine1  myHsm1(&sched, true);
        MyTestApp::myStateMachine2  myHsm2(&sched, true);

        sched.start();

        sched.publish(MyTestApp::myKickoffEvt::alloc());

        while (MyTestApp::done == false)
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

#if DO_TRY
    }
    catch (HSMError  err)
    {
        std::cout << "caught HSM error:\n"
                  << err.Format();
    }
    catch (DLL3::ListError le)
    {
        std::cout << "caught DLL3 error:\n"
                  << le.Format();
    }
    catch (ThreadSlinger::ThreadSlingerError tse)
    {
        std::cout << "caught slinger error:\n"
                  << tse.Format();
    }
#endif

    return 0;
}
