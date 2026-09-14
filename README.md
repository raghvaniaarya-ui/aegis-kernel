# Aegis Kernel

A microkernel operating system built for learning and portfolio demonstration. Aegis keeps policy in user-space servers and leaves the kernel responsible only for scheduling, memory, IPC, and hardware abstraction.

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                   User space                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────┐ │
│  │   init   │  │ console  │  │  future servers  │ │
│  │  server  │  │  server  │  │ (fs, net, proc)  │ │
│  └────┬─────┘  └────┬─────┘  └────────┬─────────┘ │
│       │             │                  │            │
│       └─────────────┴──────────────────┘            │
│                     IPC (message passing)           │
├─────────────────────────────────────────────────────┤
│                   Aegis kernel                      │
│  scheduler │ virtual memory │ syscalls │ drivers   │
└─────────────────────────────────────────────────────┘
```

See [docs/architecture.md](docs/architecture.md) for design details and a roadmap.

## Prerequisites

| Tool | Purpose |
|------|---------|
| Cross-compiler | Bare-metal build (`clang` on Windows, `x86_64-elf-gcc` on Linux) |
| `nasm` | Boot and entry assembly |
| `qemu-system-x86_64` | Run the kernel in a VM |

### Windows (PowerShell) — no WSL

`sudo apt` does **not** work in PowerShell. Use the Windows setup script instead:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\setup-windows.ps1
# reopen PowerShell, then:
.\scripts\build.ps1
.\scripts\run-qemu.ps1
```

Full guide: [docs/SETUP-WINDOWS.md](docs/SETUP-WINDOWS.md)

### WSL2 / Ubuntu (optional, recommended long-term)

```bash
sudo apt update
sudo apt install -y build-essential nasm qemu-system-x86 gcc-x86-64-elf make
make && make run
```

## Build and run

**Windows:**

```powershell
.\scripts\build.ps1
.\scripts\run-qemu.ps1
```

**Linux / WSL:**

```bash
make
make run
```

## Project layout

```
boot/           Multiboot2 header and early boot
kernel/         Microkernel (IPC, memory, syscalls)
servers/        User-space services (init, console, ...)
scripts/        QEMU and helper scripts
docs/           Architecture and design notes
```

## Roadmap (portfolio milestones)

1. **Boot** — Multiboot2 entry, serial/console output
2. **IPC** — synchronous message passing between tasks
3. **Memory** — physical page allocator, virtual memory
4. **Processes** — spawn servers from init, preemptive scheduling
5. **Drivers** — keyboard, timer, basic block device
6. **Filesystem** — simple read-only FS server

## License

MIT — use freely for learning and portfolio work.
