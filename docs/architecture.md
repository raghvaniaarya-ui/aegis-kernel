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
    char     payload[48];
};
```

Syscalls (planned):

| Syscall | Purpose |
|---------|---------|
| `ipc_send` | Send message to endpoint |
| `ipc_recv` | Block until message arrives |
| `ipc_reply` | Reply to a received message |
| `mem_map` | Map physical pages into address space |
| `task_spawn` | Create a new server task |

## Memory

Phase 1 uses a bump allocator over a known physical memory region. Phase 2 adds:

- Page frame allocator
- Per-task page tables (higher-half kernel)
- Copy-on-write (stretch goal)

## Boot flow

1. GRUB loads the Multiboot2 kernel image
2. Assembly entry sets up a stack and calls `kmain`
3. Kernel initializes serial, memory, and IPC tables
4. Kernel spawns the `init` server
5. `init` starts `console` and other core servers

## Portfolio narrative

When presenting Aegis on a resume or in interviews, emphasize:

- **Why microkernel**: fault isolation, security boundaries, modularity
- **What you built**: IPC design, boot chain, memory model, server lifecycle
- **Trade-offs**: IPC overhead vs monolithic simplicity; what you deferred and why

## Next implementation steps

1. Wire up GDT/IDT and interrupt stubs
2. Implement a round-robin scheduler with timer preemption
3. Load ELF server binaries from a ramdisk
4. Add capability-based access control to IPC endpoints
