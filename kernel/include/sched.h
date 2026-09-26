#ifndef AEGIS_SCHED_H
#define AEGIS_SCHED_H

#include <stdint.h>
#include <stdbool.h>
#include "types.h"

#define TASK_MAX 64
#define TASK_STACK_SIZE 4096
#define TIME_SLICE_MS 10

#define TASK_PRIORITY_LOW 0
#define TASK_PRIORITY_NORMAL 1
#define TASK_PRIORITY_HIGH 2
#define TASK_PRIORITY_MAX 2

typedef enum {
    TASK_STATE_EMPTY = 0,
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_TERMINATED
} task_state_t;

typedef struct AEGIS_PACKED {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t rip, rflags, cs, ss;
} task_regs_t;

typedef struct {
    task_id_t id;
    task_state_t state;
    task_regs_t regs;
    uint8_t stack[TASK_STACK_SIZE];
    uint64_t stack_top;
    task_id_t next;
    uint64_t wake_time;
    void (*entry)(void);
    uint8_t priority;
} task_t;

void sched_init(void);
task_id_t task_create(void (*entry)(void));
task_id_t task_create_priority(void (*entry)(void), uint8_t priority);
void task_yield(void);
void task_sleep(uint64_t ms);
void task_wake(task_id_t id);
void sched_tick(void);
task_id_t sched_current(void);
void sched_start(void);
void task_set_priority(task_id_t id, uint8_t priority);

#endif