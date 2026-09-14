#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
KERNEL="$ROOT/build/kernel.elf"

if [[ ! -f "$KERNEL" ]]; then
  echo "Kernel not built. Run 'make' first."
  exit 1
fi

exec qemu-system-x86_64 -kernel "$KERNEL" -serial stdio -no-reboot -no-shutdown
