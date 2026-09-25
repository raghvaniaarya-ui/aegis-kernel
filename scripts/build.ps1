# Native Windows build (clang + lld + nasm). No WSL or make required.
$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Build = Join-Path $Root "build"

$CC = "clang"
$LD = "ld.exe"
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
    "kernel\src\syscall.c",
    "kernel\src\sched.c",
    "kernel\src\timer.c",
    "kernel\src\keyboard.c",
    "kernel\src\idt.c",
    "kernel\src\elf.c",
    "kernel\src\ramdisk.c",
    "kernel\src\string.c"
)

$ServerCSources = @(
    "servers\init\init.c",
    "servers\vfs\vfs.c",
    "servers\proc\proc.c"
)

$AsmSources = @(
    "boot\boot.asm",
    "kernel\arch\x86_64\entry.asm",
    "kernel\arch\x86_64\isr.asm"
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

# Compile servers separately (not linked into kernel)
$ServerObjects = @()
foreach ($src in $ServerCSources) {
    $obj = Join-Path $Build ((Split-Path $src -Leaf) -replace '\.c$', '.o')
    $args = $CFlags + @("-c", (Join-Path $Root $src), "-o", $obj)
    & $CC @args
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $src" }
    $ServerObjects += $obj
}

foreach ($src in $AsmSources) {
    $obj = Join-Path $Build ((Split-Path $src -Leaf) -replace '\.asm$', '.o')
    & $NASM -f elf64 (Join-Path $Root $src) -o $obj
    if ($LASTEXITCODE -ne 0) { throw "Assemble failed: $src" }
    $Objects += $obj
}

$Kernel = Join-Path $Build "kernel.elf"
$LinkerScript = Join-Path $Root "linker.ld"

# First, build server binaries
Write-Host "Building server binaries..."
$ServerObjects = @()
foreach ($src in $ServerCSources) {
    $obj = Join-Path $Build ((Split-Path $src -Leaf) -replace '\.c$', '.o')
    $args = $CFlags + @("-c", (Join-Path $Root $src), "-o", $obj)
    & $CC @args
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $src" }
}

# Link server binaries
$InitBin = Join-Path $Build "init.elf"
$VfsBin = Join-Path $Build "vfs.elf"
$ProcBin = Join-Path $Build "proc.elf"

& $LD "-T" $LinkerScript "-o" $InitBin "build\init.o", "build\string.o"
if ($LASTEXITCODE -ne 0) { throw "Link init failed" }

& $LD "-T" $LinkerScript "-o" $VfsBin "build\vfs.o", "build\string.o"
if ($LASTEXITCODE -ne 0) { throw "Link vfs failed" }

& $LD "-T" $LinkerScript "-o" $ProcBin "build\proc.o", "build\string.o"
if ($LASTEXITCODE -ne 0) { throw "Link proc failed" }

# Create ramdisk with server binaries
Write-Host "Creating ramdisk..."
$RamdiskDir = Join-Path $Build "ramdisk"
New-Item -ItemType Directory -Force -Path $RamdiskDir | Out-Null
Copy-Item "build\init.elf" "$RamdiskDir\init.elf" -Force
Copy-Item "build\vfs.elf" "$RamdiskDir\vfs.elf" -Force
Copy-Item "build\proc.elf" "$RamdiskDir\proc.elf" -Force

python "$Root\scripts\mkramdisk.py" "$RamdiskDir" "$Build\ramdisk.img"
if ($LASTEXITCODE -ne 0) { throw "Ramdisk creation failed" }

# Embed ramdisk into kernel (create assembly with embedded binary)
$RamdiskAsm = Join-Path $Build "ramdisk.asm"
$RamdiskBin = Join-Path $Build "ramdisk.img"

$RamdiskAsmContent = @"
BITS 64
SECTION .rodata
GLOBAL _ramdisk_start
GLOBAL _ramdisk_end
GLOBAL _ramdisk_size

_ramdisk_start:
    incbin "ramdisk.img"
_ramdisk_end:
_ramdisk_size equ $$ - _ramdisk_start
"@

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($RamdiskAsm, $RamdiskAsmContent, (New-Object System.Text.UTF8Encoding($false)))

# Run NASM from the build directory to resolve relative paths
$OldLocation = Get-Location
Set-Location $Build
& $NASM -f elf64 "ramdisk.asm" -o "ramdisk_embed.o"
Set-Location $OldLocation
if ($LASTEXITCODE -ne 0) { throw "Assemble ramdisk failed" }

# Link kernel with ramdisk (ramdisk_embed.o provides embedded binary)
$Kernel = Join-Path $Build "kernel.elf"
$LinkerScript = Join-Path $Root "linker.ld"

& $LD "-T" $LinkerScript "-o" $Kernel $Objects "build\ramdisk_embed.o"
if ($LASTEXITCODE -ne 0) { throw "Link kernel with ramdisk failed" }

# Link server binaries
$InitBin = Join-Path $Build "init.elf"
$VfsBin = Join-Path $Build "vfs.elf"

& $LD "-T" $LinkerScript "-o" $InitBin "build\init.o", "build\string.o"
if ($LASTEXITCODE -ne 0) { throw "Link init failed" }

& $LD "-T" $LinkerScript "-o" $VfsBin "build\vfs.o", "build\string.o"
if ($LASTEXITCODE -ne 0) { throw "Link vfs failed" }

Write-Host "Built $Kernel with embedded ramdisk" -ForegroundColor Green
Write-Host "Built $InitBin" -ForegroundColor Green
Write-Host "Built $VfsBin" -ForegroundColor Green