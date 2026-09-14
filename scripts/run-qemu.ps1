#!/usr/bin/env pwsh
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Kernel = Join-Path $Root "build\kernel.elf"

if (-not (Test-Path $Kernel)) {
    Write-Host "Kernel not built. Run '.\scripts\build.ps1' first." -ForegroundColor Yellow
    exit 1
}

qemu-system-x86_64 -kernel $Kernel -serial stdio -no-reboot -no-shutdown
