#ifndef MCSOS_THREAD_H
#define MCSOS_THREAD_H

#include <stddef.h>
#include <stdint.h>

/* ── Konstanta ─────────────────────────────────────────── */
#define MCSOS_THREAD_MAGIC       UINT64_C(0x4d435448524541)
#define MCSOS_MIN_KERNEL_STACK   4096u
#define MCSOS_STACK_ALIGN        16u

/* ── Error codes ────────────────────────────────────────── */
#define MCSOS_SCHED_OK           0
#define MCSOS_SCHED_EINVAL      (-1)
#define MCSOS_SCHED_ESTACK      (-2)
#define MCSOS_SCHED_ESTATE      (-3)
#define MCSOS_SCHED_ECORRUPT    (-4)
#define MCSOS_SCHED_ENOMEM      (-5)

/* ── Thread state ───────────────────────────────────────── */
typedef enum {
    MCSOS_THREAD_NEW     = 0,
    MCSOS_THREAD_READY   = 1,
    MCSOS_THREAD_RUNNING = 2,
    MCSOS_THREAD_BLOCKED = 3,
    MCSOS_THREAD_ZOMBIE  = 4
} mcsos_thread_state_t;

/* ── Context (callee-saved registers + rip) ─────────────── */
typedef struct {
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
} mcsos_context_t;

/* ── Thread entry function ──────────────────────────────── */
typedef void (*mcsos_thread_entry_t)(void *arg);

/* ── Thread Control Block ───────────────────────────────── */
typedef struct mcsos_thread {
    uint64_t              magic;
    uint64_t              id;
    const char           *name;
    mcsos_thread_state_t  state;
    mcsos_context_t       context;
    mcsos_thread_entry_t  entry;
    void                 *arg;
    uint8_t              *stack_base;
    size_t                stack_size;
    struct mcsos_thread  *next;
    uint64_t              switches;
    uint64_t              ticks;
    int                   exit_code;
} mcsos_thread_t;

/* ── Scheduler ──────────────────────────────────────────── */
typedef struct {
    int              initialized;
    mcsos_thread_t  *current;
    mcsos_thread_t  *idle;
    mcsos_thread_t  *ready_head;
    mcsos_thread_t  *ready_tail;
    uint64_t         runnable_count;
    uint64_t         context_switches;
    uint64_t         ticks;
    uint64_t         next_id;
} mcsos_scheduler_t;

/* ── API ────────────────────────────────────────────────── */
int              mcsos_scheduler_init(mcsos_scheduler_t *sched, mcsos_thread_t *boot);
int              mcsos_thread_prepare(mcsos_thread_t *thread,
                                      const char *name,
                                      mcsos_thread_entry_t entry,
                                      void *arg,
                                      void *stack_base,
                                      size_t stack_size,
                                      uint64_t id);
int              mcsos_sched_enqueue(mcsos_scheduler_t *sched, mcsos_thread_t *thread);
mcsos_thread_t  *mcsos_sched_pick_next(mcsos_scheduler_t *sched);
int              mcsos_sched_yield(mcsos_scheduler_t *sched);
int              mcsos_sched_tick(mcsos_scheduler_t *sched);
int              mcsos_thread_block_current(mcsos_scheduler_t *sched);
int              mcsos_thread_mark_ready(mcsos_scheduler_t *sched, mcsos_thread_t *thread);
size_t           mcsos_sched_ready_count(const mcsos_scheduler_t *sched);
int              mcsos_sched_validate(const mcsos_scheduler_t *sched);

/* ── Context switch (assembly) ──────────────────────────── */
#if !defined(MCSOS_HOST_TEST)
void mcsos_context_switch(mcsos_context_t *old, mcsos_context_t *new_ctx);
#endif

#endif /* MCSOS_THREAD_H */
