# Native Windows build (clang + lld + nasm). No WSL or make required.
$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Build = Join-Path $Root "build"

$CC = "clang"
$LD = "ld.lld"
$NASM = "nasm"

function Require-Tool([string]$Name) {
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Missing '$Name'. Run .\scripts\setup-windows.ps1 and reopen PowerShell."
    }
}

Require-Tool $CC
Require-Tool $LD
Require-Tool $NASM

$Includes = @("-I$Root\kernel\include")
$CFlags = @(
    "--target=x86_64-unknown-none-elf",
    "-ffreestanding",
    "-fno-stack-protector",
    "-fno-pic",
    "-mno-red-zone",
    "-mcmodel=kernel",
    "-Wall",
    "-Wextra",
    "-std=gnu11"
) + $Includes

$CSources = @(
    "kernel\src\main.c",
    "kernel\src\console.c",
    "kernel\src\ipc.c",
    "kernel\src\mm.c",
    "kernel\src\syscall.c"
)

$AsmSources = @(
    "boot\boot.asm",
    "kernel\arch\x86_64\entry.asm"
)

New-Item -ItemType Directory -Force -Path $Build | Out-Null

$Objects = @()

foreach ($src in $CSources) {
    $obj = Join-Path $Build ((Split-Path $src -Leaf) -replace '\.c$', '.o')
    $args = $CFlags + @("-c", (Join-Path $Root $src), "-o", $obj)
    & $CC @args
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $src" }
    $Objects += $obj
}

foreach ($src in $AsmSources) {
    $obj = Join-Path $Build ((Split-Path $src -Leaf) -replace '\.asm$', '.o')
    & $NASM -f elf64 (Join-Path $Root $src) -o $obj
    if ($LASTEXITCODE -ne 0) { throw "Assemble failed: $src" }
    $Objects += $obj
}

$Kernel = Join-Path $Build "kernel.elf"
$LinkerScript = Join-Path $Root "linker.ld"

& $LD "-T" $LinkerScript "-o" $Kernel @Objects
if ($LASTEXITCODE -ne 0) { throw "Link failed" }

Write-Host "Built $Kernel" -ForegroundColor Green
