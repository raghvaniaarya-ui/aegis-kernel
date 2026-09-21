#include "timer.h"
#include "console.h"
#include "sched.h"

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static uint64_t tick_count = 0;

void timer_init(uint32_t frequency) {
    if (frequency == 0) frequency = 1000;

    uint32_t divisor = PIT_FREQUENCY / frequency;

    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    console_write("[timer] initialized at ");
    console_write_hex(frequency);
    console_write(" Hz\n");
}

void timer_handler(void) {
    tick_count++;
    sched_tick();
    task_yield();
}

uint64_t timer_ticks(void) {
    return tick_count;
}