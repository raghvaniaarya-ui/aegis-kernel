#include "sched.h"
#include "console.h"
#include "mm.h"

static task_t tasks[TASK_MAX];
static task_id_t current_task = 0;
static task_id_t ready_queue_head[TASK_PRIORITY_MAX + 1] = {0};
static task_id_t ready_queue_tail[TASK_PRIORITY_MAX + 1] = {0};
static uint64_t task_counter = 1;
static bool scheduler_running = false;

void sched_init(void) {
    for (int i = 0; i < TASK_MAX; i++) {
        tasks[i].id = 0;
        tasks[i].state = TASK_STATE_EMPTY;
        tasks[i].priority = TASK_PRIORITY_NORMAL;
    }
    current_task = 0;
    for (int p = 0; p <= TASK_PRIORITY_MAX; p++) {
        ready_queue_head[p] = 0;
        ready_queue_tail[p] = 0;
    }
    task_counter = 1;
    scheduler_running = false;
    console_write("[sched] initialized with priority support\n");
}

task_id_t task_create(void (*entry)(void)) {
    return task_create_priority(entry, TASK_PRIORITY_NORMAL);
}

task_id_t task_create_priority(void (*entry)(void), uint8_t priority) {
    if (entry == NULL) return 0;
    if (priority > TASK_PRIORITY_MAX) priority = TASK_PRIORITY_MAX;

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
    tasks[slot].priority = priority;

    for (int i = 0; i < 16; i++) {
        ((uint64_t*)tasks[slot].stack)[i] = 0;
    }

    uint8_t prio = tasks[slot].priority;
    if (ready_queue_head[prio] == 0) {
        ready_queue_head[prio] = slot;
        ready_queue_tail[prio] = slot;
    } else {
        tasks[ready_queue_tail[prio]].next = slot;
        ready_queue_tail[prio] = slot;
    }

    console_write("[sched] created task ");
    console_write_hex(id);
    console_write(" with priority ");
    console_write_dec(priority);
    console_write("\n");
    return id;
}

void task_set_priority(task_id_t id, uint8_t priority) {
    if (priority > TASK_PRIORITY_MAX) priority = TASK_PRIORITY_MAX;
    
    for (int i = 1; i < TASK_MAX; i++) {
        if (tasks[i].id == id) {
            uint8_t old_prio = tasks[i].priority;
            if (old_prio == priority) return;
            
            // Remove from old priority queue
            if (ready_queue_head[old_prio] == i) {
                ready_queue_head[old_prio] = tasks[i].next;
                if (ready_queue_tail[old_prio] == i) {
                    ready_queue_tail[old_prio] = 0;
                }
            } else {
                for (int j = 1; j < TASK_MAX; j++) {
                    if (tasks[j].next == i) {
                        tasks[j].next = tasks[i].next;
                        if (ready_queue_tail[old_prio] == i) {
                            ready_queue_tail[old_prio] = j;
                        }
                        break;
                    }
                }
            }
            
            tasks[i].priority = priority;
            
            // Add to new priority queue
            if (ready_queue_head[priority] == 0) {
                ready_queue_head[priority] = i;
                ready_queue_tail[priority] = i;
                tasks[i].next = 0;
            } else {
                tasks[ready_queue_tail[priority]].next = i;
                ready_queue_tail[priority] = i;
                tasks[i].next = 0;
            }
            return;
        }
    }
}

void task_yield(void) {
    if (!scheduler_running || current_task == 0) return;

    uint8_t prio = tasks[current_task].priority;
    if (ready_queue_head[prio] == 0) return;

    int old = current_task;
    current_task = ready_queue_head[prio];
    ready_queue_head[prio] = tasks[ready_queue_head[prio]].next;

    if (ready_queue_head[prio] == 0) {
        ready_queue_tail[prio] = 0;
    }

    if (tasks[old].state == TASK_STATE_RUNNING) {
        tasks[old].state = TASK_STATE_READY;
        tasks[old].next = 0;
        if (ready_queue_head[prio] == 0) {
            ready_queue_head[prio] = old;
            ready_queue_tail[prio] = old;
        } else {
            tasks[ready_queue_tail[prio]].next = old;
            ready_queue_tail[prio] = old;
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

    if (ready_queue_head[tasks[old].priority] == 0) {
        return;
    }

    uint8_t prio = tasks[old].priority;
    current_task = ready_queue_head[prio];
    ready_queue_head[prio] = tasks[ready_queue_head[prio]].next;
    if (ready_queue_head[prio] == 0) {
        ready_queue_tail[prio] = 0;
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
            uint8_t prio = tasks[i].priority;
            if (ready_queue_head[prio] == 0) {
                ready_queue_head[prio] = i;
                ready_queue_tail[prio] = i;
            } else {
                tasks[ready_queue_tail[prio]].next = i;
                ready_queue_tail[prio] = i;
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
    // Find highest priority task
    int highest_prio = -1;
    for (int p = TASK_PRIORITY_MAX; p >= 0; p--) {
        if (ready_queue_head[p] != 0) {
            highest_prio = p;
            break;
        }
    }
    
    if (highest_prio == -1) {
        console_write("[sched] no tasks to run\n");
        return;
    }

    scheduler_running = true;
    current_task = ready_queue_head[highest_prio];
    ready_queue_head[highest_prio] = tasks[ready_queue_head[highest_prio]].next;
    if (ready_queue_head[highest_prio] == 0) {
        ready_queue_tail[highest_prio] = 0;
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