/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <string.h>
#include <stdio.h>
#include <string>

template <class T>
class StateMachine {
protected:
    bool debug;
    struct StateRet;
    typedef StateRet (T::*State)(void);
    struct StateRet {
        enum retType { HANDLED, SUPER, TRANS, ERR } type;
        StateRet(void) { type = ERR; }
        StateRet(retType _type, State _ns = 0, const char *_name = NULL) {
            type = _type;
            if (type == SUPER)
                stateName = _name;
            if (type == SUPER || type == TRANS)
                newState = _ns;
        }
        const char * stateName; // if super
        State newState; // if TRANSITION
    };
    struct HANDLED : StateRet {
        HANDLED() : StateRet(StateRet::HANDLED) { }
    };
    struct SUPER   : StateRet {
        SUPER(State _ns, const char *_name)
            : StateRet(StateRet::SUPER,_ns,_name) { }
    };
    struct TRANS   : StateRet {
        TRANS(State _ns) : StateRet(StateRet::TRANS,_ns) { }
    };
    struct ERR : StateRet { ERR(void) : StateRet(StateRet::ERR) { } };
private:
    State state;
    StateRet callState(State s) {
        T * derived_class = dynamic_cast<T*>(this);
        if (derived_class == NULL)
        {
            printf("ERROR callstate dynamic cast returns NULL\n");
            return ERR();
        }
        return (derived_class->*s)();
    }
    enum InternalEvent {
        STATE_NORMAL, STATE_TRACE, STATE_ENTRY, STATE_EXIT
    };
    InternalEvent ievt;
    struct Trace {
        static const int MAX_TRACE = 10;
        int depth;
        State states[MAX_TRACE];
        const char *names[MAX_TRACE];
        void init(void) {
            memset(states, 0, sizeof(states));
            memset(names,  0, sizeof(names));
            depth = 0;
        }
        void save(State s, const char *name = NULL) {
            if (depth < MAX_TRACE)
            {
                states[depth] = s;
                names[depth] = name;
                depth++;
            }
        }
        void normalize(void) {
            if (depth == MAX_TRACE)
                return;
            memmove(&states[MAX_TRACE-depth], &states[0],
                    sizeof(State) * depth);
            memmove(&names[MAX_TRACE-depth], &names[0],
                    sizeof(char*) * depth);
            memset(&states[0], 0, sizeof(State) * (MAX_TRACE-depth));
            memset(&names[0],  0, sizeof(char*) * (MAX_TRACE-depth));
        }
    };
    Trace  trace_oldstate;
    Trace  trace_newstate;
    void __dispatch(void) {
        State s;
        StateRet res;
        for (s = state; s; s = res.newState)
        {
            res = callState(s);
            if (res.type == StateRet::HANDLED)
                return;
            if (res.type != StateRet::SUPER)
                break;
        }
        if (res.type == StateRet::TRANS)
        {
            State newState = res.newState;
            if (newState == state)
                return;
            // trace the current state up to the top state.
            ievt = STATE_TRACE;
            trace_oldstate.init();
            for (s = state; s; s = res.newState)
            {
                res = callState(s);
                if (res.type != StateRet::SUPER)
                {
                    printf("ERROR a state must return SUPER in "
                           "response to TRACE\n");
                    return;
                }
                trace_oldstate.save(s, res.stateName);
            }
            trace_oldstate.normalize();
            // trace new state up to top.
            trace_newstate.init();
            for (s = newState; s; s = res.newState)
            {
                res = callState(s);
                if (res.type != StateRet::SUPER)
                {
                    printf("ERROR a state must return SUPER in "
                           "response to TRACE\n");
                    return;
                }
                trace_newstate.save(s, res.stateName);
            }
            trace_newstate.normalize();
            // now compare the two traces, and
            // send an exit to every state that is
            // being exited -- important that we do it
            // innermost-to-outermost.
            ievt = STATE_EXIT;
            for (int ind = (Trace::MAX_TRACE-trace_oldstate.depth);
                 ind < Trace::MAX_TRACE; ind++)
            {
                State o = trace_oldstate.states[ind];
                if (o != trace_newstate.states[ind])
                {
                    if (debug)
                        printf("exit state %s\n",
                               trace_oldstate.names[ind]);
                    callState(o);
                }
            }
            // send an entry event to all the new states
            // being entered -- important that we do it
            // outermost-to-innermost.
            ievt = STATE_ENTRY;
            for (int ind = (Trace::MAX_TRACE-1); ind >= 0; ind--)
            {
                State n = trace_newstate.states[ind];
                if (n == NULL)
                    break;
                if (n != trace_oldstate.states[ind])
                {
                    if (debug)
                        printf("enter state %s\n",
                               trace_newstate.names[ind]);
                    callState(n);
                }
            }
            ievt = STATE_NORMAL;
            state = newState;
        }
    }
protected:
    void _dispatch(void) { ievt = STATE_NORMAL; __dispatch(); }
    bool ENTRY   (void) { return ievt == STATE_ENTRY ; }
    bool EXIT    (void) { return ievt == STATE_EXIT  ; }
    bool NORMAL  (void) { return ievt == STATE_NORMAL; }
public:
    StateMachine(State _initial) { state = _initial; debug = false; }
    void init(void) { ievt = STATE_ENTRY; __dispatch(); }
    void set_debug(bool _dbg) { debug = _dbg; }
    virtual ~StateMachine(void) { }
};


#ifdef __INCLUDE_DEMO_STATE_MACHINE__
// every state machine should follow this kind of structure.

// your state machine is a class derived from StateMachine<T>.
// note T must always be the name of the state machine class itself.
class TestStateMachine : public StateMachine<TestStateMachine>
{
    // every state may have if-statements testing "ENTRY", "EXIT", or
    // "NORMAL".  when a state is entered, ENTRY will be true.  when a
    // state is exited, EXIT will be true.  when a normal event is
    // being dispatched to the state, NORMAL will be true.
    //
    // every state function MUST return one of: HANDLED, TRANS, or SUPER.
    //
    // HANDLED means whatever event occurred has been handled by this
    // state and does not need to be passed up to a super-state.
    //
    // TRANS means we must transition to some other state. this means
    // we are exiting some states in the heirarchy and entering other
    // states, so all the states being exited will find their EXIT
    // clauses being invoked, and all the states being entered will
    // find their ENTRY clauses being invoked.
    //
    // your state machine needs a state called "initial".  that will
    // be called during the machine.init() call and must transition
    // to your first state.  the initial state must be referenced
    // by the chained constructor.
    StateRet initial( void ) {
        // your initial state must only have an ENTRY clause
        // which returns TRANS, and otherwise returns SUPER
        // with NULL state pointer.
        if (ENTRY())
        {
            //
            // put init stuff here
            //
            // initial state must transition during ENTRY to the first
            // real state of your state machine.
            return TRANS(&TestStateMachine::FirstState);
        }
        // include SUPER with NULL just so it returns something
        // when its not ENTRY.
        return SUPER(NULL, "initial");
    }

    // your state machine needs to handle events. the StateMachine
    // base class does not deal with events, that's up to you.
    // you should populate a class member with the event to be
    // processed and your state functions should check them and
    // handle them, performing transitions as needed.
    // an example of an incoming message.
    enum IncomingMessageType {
        MSG_TYPE_1,
        MSG_TYPE_2,
        MSG_TYPE_3, // only "top" handles this message
    };
    IncomingMessageType incomingMessage;
    
    StateRet top(void) {
        if (ENTRY())
        {
            // entry code here; note that your "top" state is
            // most likely only ever entered once, when the
            // state machine is initialized.
            return HANDLED();
        }
        if (EXIT())
        {
            // exit code here; note your "top" state is only
            // going to be exited once: during destructor of
            // this state machine (StateMachine tries to properly
            // exit all current states during cleanup).
            return HANDLED();
        }
        if (NORMAL())
        {
            // think of a heirarchical state machine as kind of like
            // a rain-gutter-- anything not caught by the current state
            // is passed (thru SUPER) to the super-state which might
            // know how to handle that event. until you get upto "top"
            // state.  if "top" doesn't know how to handle the event,
            // then it is not handled at all.
            switch (incomingMessage)
            {
            case MSG_TYPE_3:
                // do stuff
                return HANDLED();
            default:; // quiet compiler;
            }
        }
        // if we get here, the event is not handled, thus we
        // would pass it up a level to a super-state.  but "top"
        // is the top state, so the state pointer is NULL here.
        // note the function name here is useful for debug,
        // so we can see nice printfs.
        return SUPER(NULL, "top");
    }
    // in this test state machine design, FirstState is a substate
    // of "top", therefore when we get here, we enter both "top" and
    // "FirstState".
    StateRet FirstState( void ) {
        if (ENTRY())
        {
            // put your state entry code here
            return HANDLED();
        }
        if (EXIT())
        {
            // put your state exit code here
            return HANDLED();
        }
        if (NORMAL())
        {
            // put event handling stuff here
            switch (incomingMessage)
            {
            case MSG_TYPE_1:
                return TRANS(&TestStateMachine::SecondState);
            case MSG_TYPE_2:
                return TRANS(&TestStateMachine::ThirdState);
            default:; // quiet compiler
            }
        }
        // pass an unhandled message up one level to
        // our super-state.
        return SUPER(&TestStateMachine::top, "FirstState");
    }
    // in this test design, Second and Third states are substates
    // of FirstState (just to show what that looks like).
    StateRet SecondState(void) {
        // this state machine had no need for entry or exit
        // on SecondState so we did not populate the if-statements.
        // that is legal.
        if (NORMAL())
        {
            switch (incomingMessage)
            {
            case MSG_TYPE_1:
                // note in this example if we're in SecondState,
                // we are also in FirstState already because SecondState
                // is a sub-state of FirstState, so what does it mean
                // to transition to FirstState?  well, all it means is
                // SecondState is going to be exited.
                return TRANS(&TestStateMachine::FirstState);
            default:; // quiet compiler
            }
        }
        return SUPER(&TestStateMachine::FirstState, "SecondState");
    }
    StateRet ThirdState(void) {
        if (ENTRY())
        {
            // do something
            return HANDLED();
        }
        // this state didn't need an EXIT so we left it out.
        // it also handles no messages, so we skip NORMAL and go
        // straight to SUPER. that is also legal.
        return SUPER(&TestStateMachine::FirstState, "ThirdState");
    }
public:
    // chained constructor passes the initial state to the base class.
    // note you still need to invoke the "init" method to start
    // this state machine.
    TestStateMachine(void) : StateMachine(&TestStateMachine::initial) { }
    ~TestStateMachine(void) { }
    void dispatch( IncomingMessageType m ) {
        incomingMessage = m;
        _dispatch();
    }
};


TestStateMachine  __test_state_machine;

#endif /*__INCLUDE_TEST_STATE_MACHINE__*/
