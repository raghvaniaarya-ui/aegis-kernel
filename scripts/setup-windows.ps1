# Install OS dev tools on Windows (no WSL required).
# Run in PowerShell:
#   Set-ExecutionPolicy -Scope Process Bypass
#   .\scripts\setup-windows.ps1

$ErrorActionPreference = "Stop"

Write-Host "Installing Aegis build tools via winget..." -ForegroundColor Cyan
Write-Host "Approve any UAC prompts that appear." -ForegroundColor Yellow
Write-Host ""

$packages = @(
    @{ Id = "Git.Git"; Name = "Git" },
    @{ Id = "LLVM.LLVM"; Name = "LLVM (clang + lld)" },
    @{ Id = "NASM.NASM"; Name = "NASM" },
    @{ Id = "SoftwareFreedomConservancy.QEMU"; Name = "QEMU" }
)

foreach ($pkg in $packages) {
    Write-Host "-> $($pkg.Name)" -ForegroundColor Green
    winget install --id $pkg.Id --accept-package-agreements --accept-source-agreements
}

Write-Host ""
Write-Host "Done. Close and reopen PowerShell, then:" -ForegroundColor Cyan
Write-Host "  cd $((Resolve-Path (Join-Path $PSScriptRoot '..')).Path)"
Write-Host "  .\scripts\build.ps1"
Write-Host "  .\scripts\run-qemu.ps1"
