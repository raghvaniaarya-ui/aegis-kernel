# Aegis Kernel API Reference

## Overview

This document describes the public APIs exposed by the Aegis microkernel and its core servers.

## Kernel Syscalls

All syscalls use the `syscall` instruction with arguments in registers:
- RAX = syscall number
- RDI, RSI, RDX, R10 = arguments 0-3
- Return value in RAX

### Syscall Numbers

| Number | Name | Description |
|--------|------|-------------|
| 0 | `ipc_endpoint_create` | Create new IPC endpoint |
| 1 | `ipc_send` | Send message to endpoint |
| 2 | `ipc_recv` | Receive message (blocking) |
| 3 | `ipc_reply` | Reply to message |

### Example Usage

```c
// Create endpoint
uint64_t ep;
syscall(0, (uint64_t)&ep, 0, 0, 0);

// Send message
aegis_msg_t msg = { .type = 42, .arg0 = 0xDEADBEEF };
syscall(1, endpoint, (uint64_t)&msg, 0, 0);
```

## Message Format

```c
typedef struct {
    uint64_t sender;      // Set by kernel on receive
    uint64_t type;        // Message type (user-defined)
    uint64_t arg0;        // Argument 0
    uint64_t arg1;        // Argument 1
    uint64_t arg2;        // Argument 2
    uint64_t arg3;        // Argument 3
    uint8_t  payload[256]; // Optional data payload
} aegis_msg_t;
```

## Kernel API

### Memory Management

```c
// Initialize physical memory manager
void mm_init(const void *multiboot_info);

// Allocate physically contiguous memory
void *mm_alloc(size_t size, size_t alignment);
```

### Scheduler

```c
void sched_init(void);
task_id_t task_create(void (*entry)(void));
void task_yield(void);
void task_sleep(uint64_t ms);
void task_wake(task_id_t id);
void sched_start(void);  // Never returns
```

### Interrupts & Timer

```c
void idt_init(void);
void timer_init(uint32_t frequency);
uint64_t timer_ticks(void);
```

### Keyboard

```c
void keyboard_init(void);
bool keyboard_has_key(void);
keycode_t keyboard_get_key(void);
```

### Console

```c
void console_init(void);
void console_write(const char *s);
void console_write_hex(uint64_t value);
```

### IPC

```c
int ipc_endpoint_create(uint64_t *endpoint);
int ipc_send(uint64_t endpoint, aegis_msg_t *msg);
int ipc_recv(uint64_t endpoint, aegis_msg_t *msg);
int ipc_reply(uint64_t endpoint, aegis_msg_t *msg);
```

### Task API

```c
task_id_t task_create(void (*entry)(void));
void task_yield(void);
void task_sleep(uint64_t ms);
void task_wake(task_id_t id);
task_id_t sched_current(void);
```

## Server APIs

### Console Server (Endpoint 2)

| Message Type | Description |
|--------------|-------------|
| 1 | Write string (payload = string) |
| 2 | Write hex (arg0 = value) |

### VFS Server (Endpoint 3)

| Message Type | Description |
|--------------|-------------|
| 100 | READ (arg0=path, arg1=offset, arg2=size) |
| 101 | REPLY (arg0=result, payload=data) |

### Init Server (Endpoint 1)

| Message Type | Description |
|--------------|-------------|
| 1 | SPAWN (arg0=path, arg1=args) |
| 2 | KILL (arg0=pid) |

## Keycodes

```c
typedef enum {
    KEY_NONE = 0,
    KEY_ESC = 1,
    KEY_1 = 2, KEY_2 = 3, ..., KEY_0 = 11,
    KEY_MINUS = 12, KEY_EQUAL = 13, KEY_BACKSPACE = 14,
    KEY_TAB = 15,
    KEY_Q = 16, KEY_W = 17, ... KEY_P = 25,
    KEY_LBRACKET = 26, KEY_RBRACKET = 27, KEY_ENTER = 28,
    KEY_LCTRL = 29,
    KEY_A = 30, KEY_S = 31, ... KEY_L = 38,
    KEY_SEMICOLON = 39, KEY_APOSTROPHE = 40, KEY_GRAVE = 41,
    KEY_LSHIFT = 42, KEY_BACKSLASH = 43,
    KEY_Z = 44, KEY_X = 45, ... KEY_M = 50,
    KEY_COMMA = 51, KEY_DOT = 52, KEY_SLASH = 53,
    KEY_RSHIFT = 54, KEY_KP_ASTERISK = 55,
    KEY_LALT = 56, KEY_SPACE = 57,
    KEY_CAPSLOCK = 58,
    KEY_F1 = 59, KEY_F2 = 60, ... KEY_F10 = 68,
    KEY_NUMLOCK = 69, KEY_SCROLLLOCK = 70,
    KEY_KP_7 = 71, ... KEY_KP_0 = 82, KEY_KP_DOT = 83
} keycode_t;
```

## Error Codes

| Code | Name | Description |
|------|------|-------------|
| 0 | OK | Success |
| -1 | EINVAL | Invalid argument |
| -2 | ENOENT | Not found |
| -3 | ENOMEM | Out of memory |
| -4 | EAGAIN | Try again |
| -5 | EPERM | Permission denied |
| -6 | EBUSY | Resource busy |

## Types

```c
typedef uint64_t phys_addr_t;
typedef uint64_t virt_addr_t;
typedef uint64_t task_id_t;
typedef uint64_t endpoint_id_t;

typedef struct {
    uint64_t sender;
    uint64_t type;
    uint64_t arg0, arg1, arg2, arg3;
    uint8_t payload[256];
} aegis_msg_t;
```