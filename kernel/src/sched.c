#include "sched.h"
#include "console.h"
#include "mm.h"

static task_t tasks[TASK_MAX];
static task_id_t current_task = 0;
static task_id_t ready_queue_head = 0;
static task_id_t ready_queue_tail = 0;
static uint64_t task_counter = 1;
static bool scheduler_running = false;

void sched_init(void) {
    for (int i = 0; i < TASK_MAX; i++) {
        tasks[i].id = 0;
        tasks[i].state = TASK_STATE_EMPTY;
    }
    current_task = 0;
    ready_queue_head = 0;
    ready_queue_tail = 0;
    task_counter = 1;
    scheduler_running = false;
    console_write("[sched] initialized\n");
}

task_id_t task_create(void (*entry)(void)) {
    if (entry == NULL) return 0;

    int slot = -1;
    for (int i = 1; i < TASK_MAX; i++) {
        if (tasks[i].state == TASK_STATE_EMPTY) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        console_write("[sched] no free task slots\n");
        return 0;
    }

    task_id_t id = task_counter++;
    tasks[slot].id = id;
    tasks[slot].state = TASK_STATE_READY;
    tasks[slot].entry = entry;
    tasks[slot].stack_top = (uint64_t)&tasks[slot].stack[TASK_STACK_SIZE - 16];
    tasks[slot].stack_top &= ~0xF;
    tasks[slot].next = 0;
    tasks[slot].wake_time = 0;

    for (int i = 0; i < 16; i++) {
        ((uint64_t*)tasks[slot].stack)[i] = 0;
    }

    if (ready_queue_head == 0) {
        ready_queue_head = slot;
        ready_queue_tail = slot;
    } else {
        tasks[ready_queue_tail].next = slot;
        ready_queue_tail = slot;
    }

    console_write("[sched] created task ");
    console_write_hex(id);
    console_write("\n");
    return id;
}

void task_yield(void) {
    if (!scheduler_running || current_task == 0) return;

    if (ready_queue_head == 0) return;

    int old = current_task;
    current_task = ready_queue_head;
    ready_queue_head = tasks[ready_queue_head].next;

    if (ready_queue_head == 0) {
        ready_queue_tail = 0;
    }

    if (tasks[old].state == TASK_STATE_RUNNING) {
        tasks[old].state = TASK_STATE_READY;
        tasks[old].next = 0;
        if (ready_queue_head == 0) {
            ready_queue_head = old;
            ready_queue_tail = old;
        } else {
            tasks[ready_queue_tail].next = old;
            ready_queue_tail = old;
        }
    }

    asm volatile(
        "mov %%rsp, %0\n"
        "mov %1, %%rsp\n"
        : "=m"(tasks[old].stack_top)
        : "r"(tasks[current_task].stack_top)
        : "memory"
    );
}

void task_sleep(uint64_t ms) {
    if (!scheduler_running || current_task == 0) return;

    int old = current_task;
    tasks[old].state = TASK_STATE_BLOCKED;
    tasks[old].wake_time = ms;

    if (ready_queue_head == 0) {
        return;
    }

    current_task = ready_queue_head;
    ready_queue_head = tasks[ready_queue_head].next;
    if (ready_queue_head == 0) {
        ready_queue_tail = 0;
    }

    asm volatile(
        "mov %%rsp, %0\n"
        "mov %1, %%rsp\n"
        : "=m"(tasks[old].stack_top)
        : "r"(tasks[current_task].stack_top)
        : "memory"
    );
}

void task_wake(task_id_t id) {
    for (int i = 1; i < TASK_MAX; i++) {
        if (tasks[i].id == id && tasks[i].state == TASK_STATE_BLOCKED) {
            tasks[i].state = TASK_STATE_READY;
            tasks[i].wake_time = 0;
            tasks[i].next = 0;
            if (ready_queue_head == 0) {
                ready_queue_head = i;
                ready_queue_tail = i;
            } else {
                tasks[ready_queue_tail].next = i;
                ready_queue_tail = i;
            }
            break;
        }
    }
}

void sched_tick(void) {
    for (int i = 1; i < TASK_MAX; i++) {
        if (tasks[i].state == TASK_STATE_BLOCKED && tasks[i].wake_time > 0) {
            if (tasks[i].wake_time <= 1) {
                task_wake(tasks[i].id);
            } else {
                tasks[i].wake_time--;
            }
        }
    }
}

task_id_t sched_current(void) {
    return current_task ? tasks[current_task].id : 0;
}

void sched_start(void) {
    if (ready_queue_head == 0) {
        console_write("[sched] no tasks to run\n");
        return;
    }

    scheduler_running = true;
    current_task = ready_queue_head;
    ready_queue_head = tasks[ready_queue_head].next;
    if (ready_queue_head == 0) {
        ready_queue_tail = 0;
    }

    tasks[current_task].state = TASK_STATE_RUNNING;

    console_write("[sched] starting scheduler\n");

    asm volatile(
        "mov %0, %%rsp\n"
        "call *%1\n"
        : : "r"(tasks[current_task].stack_top), "r"(tasks[current_task].entry)
        : "memory"
    );
}