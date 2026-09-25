/*
 * Process Management Server
 * Handles process creation, listing, killing, and process information
 */
#include <stdint.h>
#include <stddef.h>
#include "../../kernel/include/types.h"

static inline int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static inline size_t strlen(const char *s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

static inline char *strncpy(char *dest, const char *src, size_t n) {
    char *d = dest;
    const char *s = src;
    while (n && (*d++ = *s++)) n--;
    while (n--) *d++ = '\0';
    return dest;
}

#define IPC_SEND 0
#define IPC_RECV 1
#define IPC_REPLY 2

#define PROC_SPAWN 1
#define PROC_KILL 2
#define PROC_LIST 3
#define PROC_GET_INFO 4
#define PROC_SIGNAL 5

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

static inline int ipc_send(uint64_t endpoint, void *msg) {
    return syscall(1, endpoint, (uint64_t)msg, 0, 0);
}

static inline int ipc_recv(uint64_t endpoint, void *msg) {
    return syscall(2, endpoint, (uint64_t)msg, 0, 0);
}

static inline int ipc_reply(uint64_t endpoint, void *msg) {
    return syscall(3, endpoint, (uint64_t)msg, 0, 0);
}

#define MAX_PROCESSES 64

typedef struct {
    uint64_t pid;
    char name[32];
    uint64_t entry_point;
    uint64_t stack_ptr;
    uint8_t state;  // 0=empty, 1=ready, 2=running, 3=blocked, 4=terminated
    uint64_t parent_pid;
    int exit_code;
} process_info_t;

static process_info_t process_table[MAX_PROCESSES];
static uint64_t next_pid = 1;
static uint64_t proc_endpoint = 0;

static void proc_write(const char *s) {
    aegis_msg_t msg = {0};
    msg.type = 1;
    for (int i = 0; s[i] && i < 256; i++) msg.data[i] = s[i];
    ipc_send(2, &msg);  // console server endpoint
}

void proc_write_hex(uint64_t val) {
    char buf[19];
    int i = 16;
    buf[0] = '0'; buf[1] = 'x';
    while (i--) {
        uint8_t nibble = val & 0xF;
        buf[18 - i] = nibble < 10 ? '0' + nibble : 'a' + (nibble - 10);
        val >>= 4;
    }
    buf[18] = 0;
    proc_write(buf);
}

void proc_write_dec(uint64_t val) {
    char buf[32];
    int len = 0;
    if (val == 0) {
        proc_write("0");
        return;
    }
    char rev[32];
    while (val > 0) {
        rev[len++] = '0' + (val % 10);
        val /= 10;
    }
    for (int i = 0; i < len; i++) {
        buf[i] = rev[len - 1 - i];
    }
    buf[len] = 0;
    proc_write(buf);
}

void proc_list(void) {
    proc_write("\n=== Process List ===\n");
    proc_write("PID  PPID  STATE      NAME\n");
    proc_write("---  ----  -----      ----\n");
    
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid != 0) {
            const char *state_str;
            switch (process_table[i].state) {
                case 1: state_str = "READY  "; break;
                case 2: state_str = "RUNNING"; break;
                case 3: state_str = "BLOCKED"; break;
                case 4: state_str = "TERM   "; break;
                default: state_str = "UNKNOWN";
            }
            proc_write_dec(process_table[i].pid);
            proc_write("  ");
            proc_write_dec(process_table[i].parent_pid);
            proc_write("  ");
            proc_write(state_str);
            proc_write("  ");
            proc_write(process_table[i].name);
            proc_write("\n");
            count++;
        }
    }
    if (count == 0) {
        proc_write("(no processes)\n");
    }
    proc_write("====================\n");
}

void proc_spawn(const char *path, const char *name) {
    // For now, just add to process table
    // Actual ELF loading would be done by kernel
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == 0) {
            process_table[i].pid = next_pid++;
            strncpy(process_table[i].name, name, 31);
            process_table[i].name[31] = 0;
            process_table[i].state = 1; // READY
            process_table[i].parent_pid = 0; // init is parent
            process_table[i].exit_code = 0;
            proc_write("[proc] Spawned process ");
            proc_write(name);
            proc_write(" with PID ");
            proc_write_dec(process_table[i].pid);
            proc_write("\n");
            return;
        }
    }
    proc_write("[proc] No free process slots\n");
}

void proc_kill(uint64_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid) {
            process_table[i].state = 4; // TERMINATED
            process_table[i].exit_code = -1;
            proc_write("[proc] Killed process PID ");
            proc_write_dec(pid);
            proc_write("\n");
            return;
        }
    }
    proc_write("[proc] Process PID ");
    proc_write_dec(pid);
    proc_write(" not found\n");
}

void proc_get_info(uint64_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid) {
            proc_write("\n=== Process Info ===\n");
            proc_write("PID: "); proc_write_dec(process_table[i].pid); proc_write("\n");
            proc_write("PPID: "); proc_write_dec(process_table[i].parent_pid); proc_write("\n");
            proc_write("Name: "); proc_write(process_table[i].name); proc_write("\n");
            
            const char *state_str;
            switch (process_table[i].state) {
                case 1: state_str = "READY"; break;
                case 2: state_str = "RUNNING"; break;
                case 3: state_str = "BLOCKED"; break;
                case 4: state_str = "TERMINATED"; break;
                default: state_str = "UNKNOWN";
            }
            proc_write("State: "); proc_write(state_str); proc_write("\n");
            proc_write("Exit Code: "); proc_write_dec(process_table[i].exit_code); proc_write("\n");
            proc_write("====================\n");
            return;
        }
    }
    proc_write("[proc] Process PID ");
    proc_write_dec(pid);
    proc_write(" not found\n");
}

void _start(void) {
    // Create our endpoint
    ipc_endpoint_create(&proc_endpoint);
    
    proc_write("[proc] Process server starting...\n");
    
    // Register with init server
    // For now, just start listening
    
    // Add init process itself
    process_table[0].pid = 1;
    strncpy(process_table[0].name, "init", 31);
    process_table[0].state = 2; // RUNNING
    process_table[0].parent_pid = 0;
    next_pid = 2;
    
    proc_write("[proc] Process server ready on endpoint ");
    proc_write_hex(proc_endpoint);
    proc_write("\n");
    
    for (;;) {
        aegis_msg_t msg = {0};
        if (ipc_recv(proc_endpoint, &msg) == 0) {
            switch (msg.type) {
                case PROC_SPAWN:
                    proc_spawn((char*)msg.data, (char*)msg.data + strlen((char*)msg.data) + 1);
                    break;
                case PROC_KILL:
                    proc_kill(msg.arg0);
                    break;
                case PROC_LIST:
                    proc_list();
                    break;
                case PROC_GET_INFO:
                    proc_get_info(msg.arg0);
                    break;
                default:
                    break;
            }
            // Send reply
            ipc_reply(msg.sender, &msg);
        }
    }
}