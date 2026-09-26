/*
 * Init server — first user-space task.
 * Spawns core servers (console, proc, vfs) via IPC.
 */
#include <stdint.h>
#include <stddef.h>
#include "../../kernel/include/types.h"

void *memset(void *s, int c, size_t n);

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
    uint8_t data[256];
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

void console_write(const char *s) {
    aegis_msg_t msg = {0};
    msg.type = 1;
    for (int i = 0; s[i] && i < 256; i++) msg.data[i] = s[i];
    ipc_send(1, &msg);
}

void console_write_hex(uint64_t val) {
    char buf[18];
    int i = 16;
    buf[0] = '0'; buf[1] = 'x';
    while (i--) {
        uint8_t nibble = val & 0xF;
        buf[18 - i] = nibble < 10 ? '0' + nibble : 'a' + (nibble - 10);
        val >>= 4;
    }
    console_write(buf);
}

void banner(void) {
    console_write("\n");
    console_write("  ___                    _            \n");
    console_write(" / _ \\                  (_)           \n");
    console_write("| __/_   _ ___  ___  ___ _  __ _ _ __ \n");
    console_write("| _|| | | / __|/ _ \\/ __| |/ _` | '__|\n");
    console_write("| | | |_| \\__ \\  __/\\__ \\ | (_| | |   \n");
    console_write("|_|  \\__, |___/\\___||___/_|\\__, |_|   \n");
    console_write("      __/ |                 __/ |      \n");
    console_write("     |___/                 |___/       \n");
    console_write("\n");
    console_write("Aegis init server v0.1\n\n");
}

void spawn_server(const char *name, uint64_t endpoint) {
    console_write("[init] spawning server: ");
    console_write(name);
    console_write("\n");
    (void)endpoint;
}

void _start(void) {
    banner();

    endpoint_id_t console_ep = 0;
    endpoint_id_t proc_ep = 0;
    endpoint_id_t vfs_ep = 0;

    ipc_endpoint_create(&console_ep);
    ipc_endpoint_create(&proc_ep);
    ipc_endpoint_create(&vfs_ep);

    console_write("[init] endpoints created\n");

    spawn_server("console", console_ep);
    spawn_server("proc", proc_ep);
    spawn_server("vfs", vfs_ep);

    console_write("[init] init server running\n");

    for (;;) {
        __asm__ volatile("hlt");
    }
}