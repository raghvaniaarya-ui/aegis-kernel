# Windows setup (no WSL)

You ran `sudo apt update` in **PowerShell**. That command only works on **Linux** (Ubuntu, WSL, etc.). On Windows you install tools differently.

## Option A — Native Windows (fastest to start)

### 1. Install tools

Open PowerShell in the project folder and run:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\setup-windows.ps1
```

This installs via `winget`:

| Tool | Purpose |
|------|---------|
| Git | Version control |
| LLVM | `clang` + `ld.lld` cross-compiler |
| NASM | Assembly |
| QEMU | Run the kernel |

**Close and reopen PowerShell** after install so PATH updates.

### 2. Build

```powershell
cd C:\Users\raghv\Projects\aegis-kernel
.\scripts\build.ps1
```

### 3. Run in QEMU

```powershell
.\scripts\run-qemu.ps1
```

You should see the Aegis banner and IPC test output in the terminal.

### Manual install (if the script fails)

```powershell
winget install Git.Git
winget install LLVM.LLVM
winget install NASM.NASM
winget install SoftwareFreedomConservancy.QEMU
```

---

## Option B — WSL2 (best long-term for OS dev)

WSL gives you a real Linux environment where `apt` works.

### 1. Install WSL (one-time, needs admin)

Open **PowerShell as Administrator** and run:

```powershell
wsl --install
```

Restart if prompted, then open **Ubuntu** from the Start menu.

### 2. Inside Ubuntu (WSL terminal)

```bash
sudo apt update
sudo apt install -y build-essential nasm qemu-system-x86 gcc-x86-64-elf make
cd /mnt/c/Users/raghv/Projects/aegis-kernel
make
make run
```

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `clang` / `nasm` / `qemu` not found | Reopen terminal after install, or reboot |
| `sudo` disabled in PowerShell | Normal on Windows — use Option A or WSL |
| UAC / admin required | Approve the installer prompt, or use an admin PowerShell for WSL install |
| Build errors after tool install | Run `.\scripts\build.ps1` and paste the full error |
