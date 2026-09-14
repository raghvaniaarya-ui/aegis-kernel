/*
 * Init server — first user-space task (future).
 * Will start core servers (console, proc, vfs) via IPC.
 */
#include <stdint.h>

void _start(void) {
    for (;;) {
        __asm__ volatile("hlt");
    }
}
