#include "syscall.h"

long syscall_dispatch(uint64_t nr, uint64_t a0, uint64_t a1, uint64_t a2) {
    (void)a0;
    (void)a1;
    (void)a2;

    switch (nr) {
    case SYS_IPC_SEND:
    case SYS_IPC_RECV:
    case SYS_TASK_YIELD:
        return -1; /* wired up when user tasks exist */
    default:
        return -1;
    }
}
