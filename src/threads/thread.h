#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/fixed_point.h"

/* estados possíveis de uma thread 
   RUNNING -> thread sendo executada na cpu
   READY -> pronto pra executar (a espera da cpu)
   BLOCKED -> bloqueada, esperando um recurso/evento pra poder ficar PRONTA
   DYING -> vai ser destruída (já foi executada)
   */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* identificador da thread  */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* definição de prioridades, normalmente, 
   o valor da prioridade de um programa determina 
   sua execução antes de outro  */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* cada thread ocua uma página de 4 kB, 
   a thread se estrutura no começo da página (offset 0), 
   enquanto a pilha da thread fica acima e vai crescendo seu tamanho 

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */

/* a estrutura da thread guarda todas as informações importantes de uma thread 
*/

struct thread
  {
    /* struct usada em thread.c */
    tid_t tid;                          /* id da thread é único dela */
    enum thread_status status;          /* status da thread (RUNNING, READY, BLOCKED ou DYING) */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* ponteiro pro topo da stack */
    int priority;                       /* prioridade de 0 a 63. */
    int nice;           // valor de "gentileza" da thread
    fixed_point recent_cpu;   //tempo da cpu usado recentemente pela thread
    struct list_elem allelem;           /* elemento pra lista de todas as threads */
    struct list_elem sleep_elem;    // elemento para a lista de threads dormindo (bloqueadas)
    int64_t wake_up_tick;   // tempo q a thread vai ficar dormindo
    
    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* para a ready_list ou a fila de semáforo. */

#ifdef USERPROG 
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_mlfqs_yield(void);

void thread_sleep (int64_t wake_up_time);
void thread_wake_up (int64_t ticks);

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

void mlfqs_increment_recent_cpu(void);
void mlfqs_recalc_priority(struct thread *t, void *aux UNUSED);
void mlfqs_recalc_load_avg(void);
void recalc_recent_cpu(struct thread *t, void *aux);
void mlfqs_recalc_all_recent_cpu(void);
void mlfqs_sort_ready_list(void);


#endif /* threads/thread.h */
