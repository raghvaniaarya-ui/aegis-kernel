# Contributing to Aegis

Thank you for your interest in contributing to Aegis!

## Code Style

- **Language**: C11 (GNU extensions), x86-64 assembly (NASM syntax)
- **Formatting**: 4 spaces, no tabs
- **Naming**: snake_case for functions/variables, PascalCase for types
- **Comments**: Doxygen-style for public APIs

## Building

### Windows (PowerShell)

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\setup-windows.ps1
# Reopen PowerShell
.\scripts\build.ps1
```

### Linux / WSL

```bash
sudo apt update && sudo apt install -y \
  build-essential nasm qemu-system-x86 \
  gcc-x86-64-elf binutils-x86-64-elf
make
```

## Testing

```bash
# Build
./scripts/build.ps1  # Windows
make                  # Linux

# Run in QEMU (smoke test)
timeout 30 qemu-system-x86_64 \
  -kernel build/kernel.elf \
  -serial stdio \
  -no-reboot -no-shutdown \
  -display none
```

## Pull Request Process

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Write code with tests if applicable
4. Run the test suite
5. Submit a PR with a clear description

## Code Review Criteria

- [ ] Code compiles without warnings (`-Wall -Wextra`)
- [ ] Follows project coding style
- [ ] Includes appropriate tests
- [ ] Updates documentation if needed
- [ ] Commit messages are descriptive

## Architecture Guidelines

- Keep the kernel minimal - push policy to servers
- Use IPC for all cross-server communication
- Capability-based security for IPC endpoints
- No global mutable state in kernel

## Reporting Issues

Use GitHub Issues for:
- Bug reports (include steps to reproduce)
- Feature requests
- Documentation improvements

## Security

Report security vulnerabilities privately via GitHub Security Advisories.