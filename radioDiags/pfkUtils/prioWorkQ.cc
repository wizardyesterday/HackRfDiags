#if 0
set -e -x
g++ prioWorkQ.cc -D__TEST__ -o pwqtst -lpthread
./pwqtst
exit 0
#endif
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

#include "prioWorkQ.h"
#include <iostream>

using namespace std;

prioWorkQ :: prioWorkQ(void)
{
    for (int p = 0; p < NUM_PRIOS; p++)
    {
        prioQueue_heads[p] = NULL;
        prioQueue_tails[p] = NULL;
    }
}

prioWorkQ :: ~prioWorkQ(void)
{
    for (int p = 0; p < NUM_PRIOS; p++)
        if (prioQueue_heads[p] != NULL)
            cerr << "prio queue " << p << " not empty!!" << endl;
}

void
prioWorkQ :: push(prioWorkQJob * job)
{
    int prio = job->prio;
    job->next = NULL;
    {
        WaitUtil::Lock lock(this);
        if (prioQueue_tails[prio] == NULL)
        {
            prioQueue_heads[prio] = prioQueue_tails[prio] = job;
        }
        else
        {
            prioQueue_tails[prio]->next = job;
            prioQueue_tails[prio] = job;
        }
    }
    waiterSignal();
}

bool
prioWorkQ :: runOne(bool wait)
{
    WaitUtil::Waiter waiter(this);
    while (1)
    {
        for (int prio = 0; prio < NUM_PRIOS; prio++)
        {
            if (prioQueue_heads[prio] != NULL)
            {
                prioWorkQJob * job = prioQueue_heads[prio];
                prioQueue_heads[prio] = job->next;
                if (job->next == NULL)
                    prioQueue_tails[prio] = NULL;
                waiter.unlock();
                job->job();
                delete job;
                return true;
            }
        }
        if (wait == false)
            return false;
        waiter.wait();
    }
}

#ifdef __TEST__

#include <unistd.h>

class myWorkJob : public prioWorkQJob {
    int a;
public:
    myWorkJob(int _prio, int _a) : prioWorkQJob(_prio) {
        a = _a;
    }
    /*virtual*/ void job(void) {
        cout << "executing job a=" << a << endl;
    }
};

void *
run_work_queue(void *arg)
{
    prioWorkQ * workq = (prioWorkQ *) arg;

    while (1)
        workq->runOne(true);

    return NULL;
}

int
main()
{
    prioWorkQ  workq;

    pthread_t id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&id, &attr, 
                   run_work_queue, &workq);
    pthread_attr_destroy(&attr);

    workq.push(new myWorkJob(15,1));
    workq.push(new myWorkJob(15,2));
    workq.push(new myWorkJob(25,8));
    workq.push(new myWorkJob(15,3));
    workq.push(new myWorkJob(15,4));
    workq.push(new myWorkJob(5,5));
    workq.push(new myWorkJob(25,9));
    workq.push(new myWorkJob(5,6));
    workq.push(new myWorkJob(5,7));

    sleep(1);

    return 0;
}

#endif /* __TEST__ */
