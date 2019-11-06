/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/** \file HSM.tcc */
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

template <class T, int type>
T * HSMEventT<T,type>::alloc(void)
{
    static ThreadSlinger::thread_slinger_pool<T> pool;
    return pool.alloc(0,true);
}

template <class T>
const std::string HSM<T>::ActTypeNames[HSM<T>::NUMACTS] = 
{ "HANDLED", "TRANS", "SUPER", "TOP" };

template <class T>
HSM<T>::Action::Action(ActType _act, State _state, char const *_name)
    : act(_act), state(_state), name(_name) { }

template <class T>
HSM<T>::HANDLED::HANDLED(void)
    : Action(ACT_HANDLED,NULL,NULL) { }

template <class T>
HSM<T>::TRANS::TRANS(State s)
    : Action(ACT_TRANS, s, NULL) { }

template <class T>
HSM<T>::SUPER::SUPER(State s, char const * __name)
    : Action(ACT_SUPER, s, __name) { }

template <class T>
HSM<T>::TOP::TOP(void)
    : Action(ACT_TOP,NULL,"top") { }

template <class T>
HSM<T>::StateTraceEntry::StateTraceEntry(State _state, char const * _name)
    : state(_state), name(_name) { }

template <class T>
char const * HSM<T>::stateName(HSM<T>::State state,
                               HSM<T>::State *nextState /*= NULL*/,
                               bool *top /*= NULL*/)
{
    if (state == NULL)
        return "";
    T * derived = dynamic_cast<T*>(this);
    Action a = (derived->*state)(&probeEvt);
    switch (a.act)
    {
    case ACT_HANDLED:
    case ACT_TRANS:
        throw HSMError(HSMError::HSMErrorHandleProbe);
    case ACT_TOP:
        if (top != NULL)
            *top = true;
        // fallthru
    case ACT_SUPER:
        if (nextState != NULL)
            *nextState = a.state;
        return a.name;
    default:
    {
        HSMError  err(HSMError::HSMErrorBogusAct);
        std::ostringstream ostr;
        ostr << "bogus act " << (int) a.act;
        err.str = ostr.str();
        throw err;
    }
    }
}

template <class T>
void HSM<T>::backtrace(HSM<T>::StateTrace *traceret,
                       HSM<T>::State state)
{
    StateTrace trace;
    bool done = false;
    do {
        State nextState = NULL;
        char const * name = stateName(state,&nextState,&done);
        trace.push_back(StateTraceEntry(state, name));
        state = nextState;
    } while (done == false);
    traceret->clear();
    // reverse order of trace
    for (int ind = trace.size()-1; ind >= 0; ind--)
        traceret->push_back(trace[ind]);
}

template <class T>
std::string HSM<T>::trace2str(HSM<T>::StateTrace *trace)
{
    std::ostringstream ostr;
    for (size_t ind = 0; ind < trace->size(); ind++)
    {
        if (ind != 0)
            ostr << ".";
        ostr << (*trace)[ind].name;
    }
    return ostr.str();
}

template <class T>
HSM<T>::HSM( bool _debug /*= false*/ )
    : debug(_debug),
      exitEvt(HSMEvent::HSM_EXIT),
      entryEvt(HSMEvent::HSM_ENTRY),
      probeEvt(HSMEvent::__HSM_PROBE)
{
    currentState = NULL;
    currentTrace = NULL;
    oldTrace = NULL;
}

template <class T>
void HSM<T>::HSMInit(void)
{
    T * derived = dynamic_cast<T*>(this);
    Action a = initial();
    if (a.act != ACT_TRANS)
        throw HSMError(HSMError::HSMErrorInitialTrans);
    currentState = a.state;
    currentTrace = &trace1;
    oldTrace = &trace2;
    backtrace(currentTrace, currentState);
    for (size_t ind = 0; ind < currentTrace->size(); ind++)
        (derived->*((*currentTrace)[ind].state))(&entryEvt);
    if (debug)
        std::cout << "after init, current state: "
                  << trace2str(currentTrace)
                  << std::endl;
}

template <class T>
void HSM<T>::dispatch(HSMEvent const * evt)
{
    T * derived = dynamic_cast<T*>(this);
    State newState = NULL;
    State state = currentState;
    bool done = true;
    size_t ind;
    if (evt->type == HSMEvent::HSM_TERMINATE)
    {
        // issue exit to all states.
        for (ind = currentTrace->size()-1; ind >= 0; ind--)
        {
            Action a = (derived->*((*currentTrace)[ind].state))(&exitEvt);
            if (a.act != ACT_HANDLED)
                throw HSMError(HSMError::HSMErrorExitHandler);
        }
        return;
    }
    if (debug)
        std::cout << "dispatching event " 
                  << evt->evtName()
                  << " to state "
                  << stateName(currentState)
                  << std::endl;
    do {
        done = true;
        if (debug)
            std::cout << "--> passing event "
                      << evt->evtName()
                      << " to state "
                      << stateName(state)
                      << std::endl;
        Action  a = (derived->*state)(evt);
        if (debug)
            std::cout << "<-- returns "
                      << ActTypeNames[a.act]
                      << " "
                      << stateName(a.state)
                      << std::endl;
        switch (a.act)
        {
        case ACT_HANDLED:
            // do nothing
            break;
        case ACT_TRANS:
            newState = a.state;
            break;
        case ACT_SUPER:
            state = a.state;
            done = false;
            break;
        case ACT_TOP:
            if (debug)
                std::cout << "unhandled event "
                          << evt->type
                          << ":"
                          << evt->evtName()
                          << std::endl;
            break;
        default:
        {
            HSMError  err(HSMError::HSMErrorBogusAct);
            std::ostringstream ostr;
            ostr << "bogus act " << (int) a.act;
            err.str = ostr.str();
            throw err;
        }
        }
    } while(done == false);
    if (newState != NULL && newState != currentState)
    {
        StateTrace * newTrace = oldTrace;
        backtrace(newTrace, newState);
        if (debug)
            std::cout << "transition from "
                      << trace2str(currentTrace)
                      << " to "
                      << trace2str(newTrace)
                      << std::endl;
        for (ind = currentTrace->size()-1; ind >= 0; ind--)
        {
            if (ind < newTrace->size() &&
                (*currentTrace)[ind].state == (*newTrace)[ind].state)
                break;
            Action a = (derived->*((*currentTrace)[ind].state))(&exitEvt);
            if (a.act != ACT_HANDLED)
                throw HSMError(HSMError::HSMErrorExitHandler);
        }
        ind++;
        for (; ind < newTrace->size(); ind++)
        {
            Action a = (derived->*((*newTrace)[ind].state))(&entryEvt);
            if (a.act != ACT_HANDLED)
                throw HSMError(HSMError::HSMErrorEntryHandler);
        }
        currentState = newState;
        oldTrace = currentTrace;
        currentTrace = newTrace;
    }
}

template <class T>
ActiveHSM<T>::ActiveHSM( HSMScheduler * __sched, 
                         const std::string &name,
                         bool __debug /*=false*/ )
    : ActiveHSMBase(__sched, name),
      HSM<T>(__debug)
{
    // ?
}

//virtual
template <class T>
ActiveHSM<T>::~ActiveHSM(void)
{
    // ?
}

template <class T>
void ActiveHSM<T>::init(void)
{
    HSM<T>::HSMInit();
}

template <class T>
void ActiveHSM<T>::AHSMdispatch(HSMEvent const * evt)
{
    HSM<T>::dispatch(evt);
}

template <class T>
void ActiveHSM<T>::subscribe(int type)
{
    sched->subscribe(this,(HSMEvent::Type)type);
}

template <class T>
void ActiveHSM<T>::publish(HSMEvent * evt)
{
    sched->publish(evt);
}
