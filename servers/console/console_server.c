/*
 * Console server — handles text output policy in user space (future).
 * Kernel keeps only the serial HAL for early boot.
 */
#include <stdint.h>

void _start(void) {
    for (;;) {
        __asm__ volatile("hlt");
    }
}
