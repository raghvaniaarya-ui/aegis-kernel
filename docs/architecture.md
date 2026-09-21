# Aegis Architecture

## Design principles

Aegis follows a **microkernel** model: the kernel stays small and most OS services run as isolated user-space servers that communicate over IPC.

| Layer | Responsibility |
|-------|----------------|
| Kernel | Scheduling, virtual memory, IPC, interrupt handling, minimal HAL |
| Servers | Device policy, filesystem, networking, process management |
| Init | Boots the system and starts core servers |

## IPC model

Servers exchange fixed-size messages through kernel-mediated endpoints:

```c
struct aegis_msg {
    uint64_t sender;
    uint64_t type;
    uint64_t arg0;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint8_t  payload[256];
};
```

Syscalls (implemented):

| Syscall | Number | Purpose |
|---------|--------|---------|
| `ipc_endpoint_create` | 0 | Create a new IPC endpoint |
| `ipc_send` | 1 | Send message to endpoint |
| `ipc_recv` | 2 | Block until message arrives |
| `ipc_reply` | 3 | Reply to a received message |

## Scheduler

Preemptive round-robin scheduler with time-slice quantum of 10ms.

Task states: EMPTY, READY, RUNNING, BLOCKED, TERMINATED

## Interrupts & IDT

- ISR handlers for CPU exceptions (0-31)
- IRQ handlers for hardware interrupts (0-15, remapped to 32-47)
- PIC remapped to vectors 32-47
- Current IRQs: Timer (0), Keyboard (1)

## Memory

Phase 1: Bump allocator over known physical memory region
Phase 2 (planned):
- Page frame allocator
- Per-task page tables (higher-half kernel)
- Copy-on-write (stretch goal)

## Drivers

- **Timer (PIT)**: Channel 0, mode 3, configurable frequency (default 1000Hz)
- **Keyboard (PS/2)**: Scancode set 1, supports shift, caps lock, ctrl, alt
- **Serial (UART 16550)**: COM1 at 0x3F8, 115200 baud

## Boot flow

1. GRUB loads the Multiboot2 kernel image
2. Assembly entry sets up a stack and calls `kmain`
3. Kernel initializes serial, memory, IPC, IDT, timer, keyboard
4. Kernel spawns initial tasks (task_a, task_b)
5. Scheduler starts (preemptive round-robin)

## Server Architecture

| Server | Endpoint | Purpose |
|--------|----------|---------|
| init | 1 | First user task, spawns other servers |
| console | 2 | Serial output, logging |
| vfs | 3 | Read-only filesystem operations |
| proc | 4 | Process management (planned) |

## Portfolio narrative

When presenting Aegis on a resume or in interviews, emphasize:

- **Why microkernel**: fault isolation, security boundaries, modularity
- **What you built**: IPC design, boot chain, memory model, server lifecycle, scheduler, drivers
- **Trade-offs**: IPC overhead vs monolithic simplicity; what you deferred and why

## Next implementation steps

1. Implement init server ELF loading and capability-based access control
2. Add ELF loader for user-space servers
3. Implement process management server (proc)
3. Add capability-based access control to IPC endpoints
4. Implement virtual filesystem with ramdisk support
5. Add block device driver for virtio-blk
6. Implement network stack (virtio-net)
6. Add capability-based access control to IPC endpoints

## API Reference

### Kernel Syscalls

```c
// Create a new IPC endpoint
int ipc_endpoint_create(uint64_t *endpoint);

// Send a message to an endpoint
int ipc_send(uint64_t endpoint, aegis_msg_t *msg);

// Receive a message from an endpoint (blocking)
int ipc_recv(uint64_t endpoint, aegis_msg_t *msg);

// Reply to a received message
int ipc_reply(uint64_t endpoint, aegis_msg_t *msg);
```

### Message Structure

```c
typedef struct {
    uint64_t sender;
    uint64_t type;
    uint64_t arg0;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint8_t  payload[256];
} aegis_msg_t;
```

### Scheduler API

```c
// Initialize the scheduler
void sched_init(void);

// Create a new task
task_id_t task_create(void (*entry)(void));

// Yield the current time slice
void task_yield(void);

// Sleep for specified milliseconds
void task_sleep(uint64_t ms);

// Wake a blocked task
void task_wake(task_id_t id);

// Start the scheduler (never returns)
void sched_start(void);
```

### Timer API

```c
// Initialize timer at specified frequency (Hz)
void timer_init(uint32_t frequency);

// Get current tick count
uint64_t timer_ticks(void);
```

### Keyboard API

```c
// Initialize keyboard
void keyboard_init(void);

// Check if key available
bool keyboard_has_key(void);

// Get next key
keycode_t keyboard_get_key(void);
```

### Memory Management

```c
// Initialize memory manager with Multiboot info
void mm_init(const void *multiboot_info);

// Allocate aligned physical memory
void *mm_alloc(size_t size, size_t alignment);
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines.