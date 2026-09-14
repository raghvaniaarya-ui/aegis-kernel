#ifndef AEGIS_SYSCALL_H
#define AEGIS_SYSCALL_H

#include "ipc.h"

typedef enum {
    SYS_IPC_SEND = 1,
    SYS_IPC_RECV = 2,
    SYS_TASK_YIELD = 3,
} aegis_syscall_t;

long syscall_dispatch(uint64_t nr, uint64_t a0, uint64_t a1, uint64_t a2);

#endif
