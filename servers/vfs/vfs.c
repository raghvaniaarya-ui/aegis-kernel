/*
 * VFS server — simple read-write filesystem server with ramfs.
 */
#include <stdint.h>
#include <stddef.h>
#include "../../kernel/include/types.h"

#define IPC_SEND 0
#define IPC_RECV 1
#define IPC_REPLY 2

#define VFS_READ 100
#define VFS_WRITE 101
#define VFS_CREATE 102
#define VFS_DELETE 103
#define VFS_TRUNCATE 104
#define VFS_OPEN 105
#define VFS_CLOSE 106
#define VFS_SEEK 107
#define VFS_STAT 108
#define VFS_READDIR 109
#define VFS_REPLY 200

#define MAX_FILES 64
#define MAX_FILE_SIZE 4096

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

static inline void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

static inline void *memset(void *s, int c, size_t n) {
    uint8_t *p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

static inline char *strncpy(char *dest, const char *src, size_t n) {
    char *d = dest;
    const char *s = src;
    while (n && (*d++ = *s++)) n--;
    while (n--) *d++ = '\0';
    return dest;
}

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

typedef struct {
    char name[64];
    uint8_t data[MAX_FILE_SIZE];
    uint64_t size;
    uint32_t flags;
    uint8_t used;
} file_entry_t;

static file_entry_t file_table[MAX_FILES];
static endpoint_id_t vfs_endpoint = 0;

int file_find(const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used && strcmp(file_table[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int file_find_free(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!file_table[i].used) return i;
    }
    return -1;
}

void handle_read(aegis_msg_t *msg) {
    const char *path = (const char *)msg->data;
    int idx = file_find(path);
    
    aegis_msg_t reply = {0};
    reply.type = VFS_REPLY;
    
    if (idx >= 0) {
        uint64_t offset = msg->arg0;
        uint64_t size = msg->arg1;
        if (offset + size > file_table[idx].size) {
            size = file_table[idx].size - offset;
        }
        memcpy(reply.data, file_table[idx].data + offset, size);
        reply.arg0 = size;
        reply.arg1 = 0;
    } else {
        reply.arg0 = -1;
        reply.arg1 = -1;
    }
    ipc_reply(msg->sender, &reply);
}

void handle_write(aegis_msg_t *msg) {
    const char *path = (const char *)msg->data;
    int idx = file_find(path);
    
    aegis_msg_t reply = {0};
    reply.type = VFS_REPLY;
    
    if (idx >= 0) {
        uint64_t offset = msg->arg0;
        uint64_t size = msg->arg1;
        if (offset + size > MAX_FILE_SIZE) {
            size = MAX_FILE_SIZE - offset;
        }
        memcpy(file_table[idx].data + offset, msg->data, size);
        if (offset + size > file_table[idx].size) {
            file_table[idx].size = offset + size;
        }
        reply.arg0 = size;
        reply.arg1 = 0;
    } else {
        reply.arg0 = -1;
        reply.arg1 = -1;
    }
    ipc_reply(msg->sender, &reply);
}

void handle_create(aegis_msg_t *msg) {
    const char *path = (const char *)msg->data;
    uint32_t flags = msg->arg0;
    
    aegis_msg_t reply = {0};
    reply.type = VFS_REPLY;
    
    if (file_find(path) >= 0) {
        reply.arg0 = -1;
        reply.arg1 = -1;
    } else {
        int idx = file_find_free();
        if (idx < 0) {
            reply.arg0 = -1;
            reply.arg1 = -1;
        } else {
            strncpy(file_table[idx].name, path, 63);
            file_table[idx].name[63] = 0;
            file_table[idx].size = 0;
            file_table[idx].flags = flags;
            file_table[idx].used = 1;
            reply.arg0 = 0;
            reply.arg1 = 0;
        }
    }
    ipc_reply(msg->sender, &reply);
}

void handle_delete(aegis_msg_t *msg) {
    const char *path = (const char *)msg->data;
    int idx = file_find(path);
    
    aegis_msg_t reply = {0};
    reply.type = VFS_REPLY;
    
    if (idx >= 0) {
        file_table[idx].used = 0;
        file_table[idx].size = 0;
        reply.arg0 = 0;
        reply.arg1 = 0;
    } else {
        reply.arg0 = -1;
        reply.arg1 = -1;
    }
    ipc_reply(msg->sender, &reply);
}

void handle_truncate(aegis_msg_t *msg) {
    const char *path = (const char *)msg->data;
    uint64_t new_size = msg->arg0;
    int idx = file_find(path);
    
    aegis_msg_t reply = {0};
    reply.type = VFS_REPLY;
    
    if (idx >= 0) {
        if (new_size > MAX_FILE_SIZE) {
            new_size = MAX_FILE_SIZE;
        }
        if (new_size < file_table[idx].size) {
            memset(file_table[idx].data + new_size, 0, file_table[idx].size - new_size);
        }
        file_table[idx].size = new_size;
        reply.arg0 = 0;
        reply.arg1 = 0;
    } else {
        reply.arg0 = -1;
        reply.arg1 = -1;
    }
    ipc_reply(msg->sender, &reply);
}

void handle_stat(aegis_msg_t *msg) {
    const char *path = (const char *)msg->data;
    int idx = file_find(path);
    
    aegis_msg_t reply = {0};
    reply.type = VFS_REPLY;
    
    if (idx >= 0) {
        reply.arg0 = 0;
        reply.arg1 = file_table[idx].size;
        reply.arg2 = file_table[idx].flags;
        memcpy(reply.data, file_table[idx].name, 64);
    } else {
        reply.arg0 = -1;
        reply.arg1 = -1;
    }
    ipc_reply(msg->sender, &reply);
}

void handle_readdir(aegis_msg_t *msg) {
    (void)msg;
    
    aegis_msg_t reply = {0};
    reply.type = VFS_REPLY;
    
    char *out = (char *)reply.data;
    int written = 0;
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used) {
            int len = strlen(file_table[i].name);
            if (written + len + 2 >= 512) break;
            memcpy(out + written, file_table[i].name, len);
            written += len;
            out[written++] = '\n';
        }
    }
    out[written] = '\0';
    
    reply.arg0 = written;
    reply.arg1 = 0;
    ipc_reply(msg->sender, &reply);
}

void handle_open(aegis_msg_t *msg) {
    const char *path = (const char *)msg->data;
    int idx = file_find(path);
    
    aegis_msg_t reply = {0};
    reply.type = VFS_REPLY;
    
    if (idx >= 0) {
        reply.arg0 = idx;
        reply.arg1 = 0;
    } else {
        reply.arg0 = -1;
        reply.arg1 = -1;
    }
    ipc_reply(msg->sender, &reply);
}

void handle_close(aegis_msg_t *msg) {
    (void)msg;
    aegis_msg_t reply = {0};
    reply.type = VFS_REPLY;
    reply.arg0 = 0;
    reply.arg1 = 0;
    ipc_reply(msg->sender, &reply);
}

void handle_seek(aegis_msg_t *msg) {
    (void)msg;
    aegis_msg_t reply = {0};
    reply.type = VFS_REPLY;
    reply.arg0 = -1;
    reply.arg1 = -1;
    ipc_reply(msg->sender, &reply);
}

void vfs_init_ramfs(void) {
    // Create some default files
    int idx;
    
    idx = file_find_free();
    if (idx >= 0) {
        strncpy(file_table[idx].name, "/hello.txt", 63);
        const char *content = "Hello from Aegis RAM filesystem!\n";
        size_t len = strlen(content);
        memcpy(file_table[idx].data, content, len);
        file_table[idx].size = len;
        file_table[idx].flags = 0;
        file_table[idx].used = 1;
    }
    
    idx = file_find_free();
    if (idx >= 0) {
        strncpy(file_table[idx].name, "/readme.md", 63);
        const char *content = "# Aegis OS\n\nA microkernel-based operating system.\n";
        size_t len = strlen(content);
        memcpy(file_table[idx].data, content, len);
        file_table[idx].size = len;
        file_table[idx].flags = 0;
        file_table[idx].used = 1;
    }
    
    idx = file_find_free();
    if (idx >= 0) {
        strncpy(file_table[idx].name, "/version", 63);
        const char *content = "Aegis OS 0.1.0\n";
        size_t len = strlen(content);
        memcpy(file_table[idx].data, content, len);
        file_table[idx].size = len;
        file_table[idx].flags = 0;
        file_table[idx].used = 1;
    }
}

void _start(void) {
    endpoint_id_t ep = 0;
    ipc_endpoint_create(&ep);
    vfs_endpoint = ep;
    
    vfs_init_ramfs();
    
    for (;;) {
        aegis_msg_t msg = {0};
        ipc_recv(ep, &msg);
        
        switch (msg.type) {
            case VFS_READ:
                handle_read(&msg);
                break;
            case VFS_WRITE:
                handle_write(&msg);
                break;
            case VFS_CREATE:
                handle_create(&msg);
                break;
            case VFS_DELETE:
                handle_delete(&msg);
                break;
            case VFS_TRUNCATE:
                handle_truncate(&msg);
                break;
            case VFS_STAT:
                handle_stat(&msg);
                break;
            case VFS_READDIR:
                handle_readdir(&msg);
                break;
            case VFS_OPEN:
                handle_open(&msg);
                break;
            case VFS_CLOSE:
                handle_close(&msg);
                break;
            case VFS_SEEK:
                handle_seek(&msg);
                break;
            default:
                {
                    aegis_msg_t reply = {0};
                    reply.type = VFS_REPLY;
                    reply.arg0 = -1;
                    reply.arg1 = -1;
                    ipc_reply(msg.sender, &reply);
                }
                break;
        }
    }
}