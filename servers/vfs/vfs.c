/*
 * VFS server — simple read-only filesystem server.
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
#define VFS_REPLY 200

#define IPC_SEND 0
#define IPC_RECV 1
#define IPC_REPLY 2

#define MAX_FILES 64
#define MAX_FILE_SIZE 4096

static inline int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
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
#define VFS_REPLY 200

#define IPC_SEND 0
#define IPC_RECV 1
#define IPC_REPLY 2

#define MAX_FILES 64
#define MAX_FILE_SIZE 4096

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
        memcpy(msg->data, file_table[idx].data + offset, size);
        reply.arg0 = size;
        reply.arg1 = 0;
    } else {
        reply.arg0 = -1;
        reply.arg1 = -1;
    }
    ipc_reply(0, &reply);
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
    ipc_reply(0, &reply);
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
    ipc_reply(0, &reply);
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
    ipc_reply(0, &reply);
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
    ipc_reply(0, &reply);
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
    ipc_reply(0, &reply);
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
    ipc_reply(0, &reply);
}

void handle_close(aegis_msg_t *msg) {
    (void)msg;
    aegis_msg_t reply = {0};
    reply.type = VFS_REPLY;
    reply.arg0 = 0;
    reply.arg1 = 0;
    ipc_reply(0, &reply);
}

void handle_seek(aegis_msg_t *msg) {
    (void)msg;
    aegis_msg_t reply = {0};
    reply.type = VFS_REPLY;
    reply.arg0 = -1;
    reply.arg1 = -1;
    ipc_reply(0, &reply);
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