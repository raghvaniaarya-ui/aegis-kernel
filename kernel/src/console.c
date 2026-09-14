#include "console.h"

#define COM1 0x3F8

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static int serial_is_transmit_empty(void) {
    return inb(COM1 + 5) & 0x20;
}

static void serial_write_char(char c) {
    while (!serial_is_transmit_empty()) {
    }
    outb(COM1, (uint8_t)c);
}

void console_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

void console_write(const char *s) {
    for (; *s; s++) {
        if (*s == '\n') {
            serial_write_char('\r');
        }
        serial_write_char(*s);
    }
}

void console_write_hex(uint64_t value) {
    static const char *digits = "0123456789abcdef";
    char buf[19];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 15; i >= 0; i--) {
        buf[2 + (15 - i)] = digits[(value >> (i * 4)) & 0xF];
    }
    buf[18] = '\0';
    console_write(buf);
}
