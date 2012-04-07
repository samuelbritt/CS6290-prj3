#ifndef THREAD_H
#define THREAD_H

#include "insn.h"
#include "settings.h"
#include "types.h"

#if (SESC_EMUL)
#include "ThreadContext.h"
#else
extern DEQUE<Insn *> *trace_queue;
#endif

using namespace std;

typedef enum {
    THREAD_UNDEF,
    THREAD_STALLED,
    THREAD_STALLED_NOT_RELEASED,
    THREAD_READY,
    THREAD_READY_NOT_RELEASED,
    THREAD_RUNNING,
    THREAD_EXITED
} thread_state_t;

class ThreadContext;

/**
 * A Thread may be either assigned to a processor, ready to run sitting on the 
 * ready_list, or stalled on the stalled list.
 */
class Thread {
public:
#if (SESC_EMUL)
	Thread(ThreadContext::pointer context);
#else
    Thread(int tid);
#endif

	~Thread();

    int tid;   /** Thread ID.  */
    int tgid;  /** Thread Group Leader.  */
    int pid;   /** Process ID.  */
    int pgid;  /** Process Group Leader.  */
    int ppid;  /** Process Parent ID.  */

    int cpu;
    timestamp_t sched_time;
    thread_state_t state;
    ThreadContext *context;    
    
    int get_tid (void) { return tid; }

    bool is_suspended (void);
    bool is_exited (void);

    Insn *fetch (void);
    Insn *peek (void);
    paddr_t peek_pc (void);

    void dump (void);
};

#endif   // THREAD_H
