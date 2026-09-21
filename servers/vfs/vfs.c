/*
 * VFS server — simple read-only filesystem server.
 */
#include <stdint.h>
#include <stddef.h>
#include "../../kernel/include/types.h"

#define IPC_SEND 0
#define IPC_RECV 1
#define IPC_REPLY 2

typedef struct {
    uint64_t type;
    uint64_t sender;
    uint64_t arg0;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint8_t data[512];
} aegis_msg_t;

static inline int syscall(uint64_t num, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    uint64_t ret;
    asm volatile("syscall" : "=a"(ret) : "a"(num), "D"(arg0), "S"(arg1), "d"(arg2), "r"(arg3) : "rcx", "r11", "memory");
    return (int)ret;
}

static inline int ipc_endpoint_create(uint64_t *endpoint) {
    return syscall(0, (uint64_t)endpoint, 0, 0, 0);
}

static inline int ipc_send(uint64_t endpoint, aegis_msg_t *msg) {
    return syscall(1, endpoint, (uint64_t)msg, 0, 0);
}

static inline int ipc_recv(uint64_t endpoint, aegis_msg_t *msg) {
    return syscall(2, endpoint, (uint64_t)msg, 0, 0);
}

static inline int ipc_reply(uint64_t endpoint, aegis_msg_t *msg) {
    return syscall(3, endpoint, (uint64_t)msg, 0, 0);
}

void _start(void) {
    endpoint_id_t ep = 0;
    ipc_endpoint_create(&ep);

    for (;;) {
        aegis_msg_t msg = {0};
        ipc_recv(0, &msg);
        
        if (msg.type == 100) { // READ
            // Simple read implementation
            msg.type = 101; // REPLY
            msg.arg0 = 0; // success
            ipc_reply(msg.sender, &msg);
        }
    }
}