#include "thread.h"
#include "processor.h"
#include "settings.h"
#include "sim.h"
#include "types.h"

extern Simulator *Sim;

using namespace std;

Thread::Thread (int tid)
{
    this->tid = tid;
    this->tgid = -1;
    this->pid = -1;
    this->pgid = -1;
    this->ppid = -1;

    this->cpu = -1;
    this->sched_time = Global_Clock;
    this->state = THREAD_UNDEF;
    this->context = NULL;
}

Thread::~Thread ()
{
}

bool Thread::is_suspended ()
{
    return false;
}


bool Thread::is_exited ()
{
    return trace_queue[tid].empty ();
}

Insn *Thread::fetch ()
{
    Insn *insn;

    if (!trace_queue[tid].empty ()) 
    {
        insn = trace_queue[tid].back ();
        trace_queue[tid].pop_back ();

        if (trace_queue[tid].empty ())
            this->state = THREAD_EXITED;

        return insn;
    }
    else
    {
        this->state = THREAD_EXITED;
        return new Insn (0x0, OpNop);
    }
}

void Thread::dump ()
{
    fprintf (stderr, "Tid: %d\n", tid);
    fprintf (stderr, "TGid: %d\n", tgid);
    fprintf (stderr, "Pid: %d\n", pid);
    fprintf (stderr, "PGid: %d\n", pgid);
    fprintf (stderr, "PPid: %d\n", ppid);
    
    switch (state) {
    case THREAD_UNDEF:
        fprintf (stderr, "State - Undefined\n");
        break;
    case THREAD_STALLED:
        fprintf (stderr, "State - Stalled\n");
        break;
    case THREAD_READY:
        fprintf (stderr, "State - Ready\n");
        break;
    case THREAD_RUNNING:
        fprintf (stderr, "State - Running\n");
        break;
    case THREAD_EXITED:
        fprintf (stderr, "State - Exited\n");
        break;
    default:
        fatal_error ("Thread State Unknown.\n");
    }
}
