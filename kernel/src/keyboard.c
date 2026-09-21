#include "keyboard.h"
#include "console.h"
#include "ipc.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

#define KEYBOARD_BUFFER_SIZE 256

static keycode_t key_buffer[KEYBOARD_BUFFER_SIZE];
static uint16_t buffer_head = 0;
static uint16_t buffer_tail = 0;
static bool shift_pressed = false;
static bool caps_lock = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;

static const char scancode_to_ascii[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+',
    '1', '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char scancode_to_ascii_shift[128] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0, '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+',
    '1', '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void keyboard_init(void) {
    outb(0x64, 0xAE);
    console_write("[keyboard] initialized\n");
}

void keyboard_handler(void) {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    if (!(status & 1)) return;

    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    bool released = scancode & 0x80;
    uint8_t code = scancode & 0x7F;

    switch (code) {
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            shift_pressed = !released;
            break;
        case KEY_CAPSLOCK:
            if (!released) caps_lock = !caps_lock;
            break;
        case KEY_LCTRL:
            ctrl_pressed = !released;
            break;
        case KEY_LALT:
            alt_pressed = !released;
            break;
    }

    if (!released && code < 128) {
        char c = shift_pressed ^ caps_lock ? scancode_to_ascii_shift[code] : scancode_to_ascii[code];
        if (c != 0) {
            uint16_t next = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
            if (next != buffer_head) {
                key_buffer[buffer_tail] = (keycode_t)c;
                buffer_tail = next;
            }
        }
    }

    outb(0x20, 0x20);
}

bool keyboard_has_key(void) {
    return buffer_head != buffer_tail;
}

keycode_t keyboard_get_key(void) {
    if (buffer_head == buffer_tail) return KEY_NONE;
    keycode_t key = key_buffer[buffer_head];
    buffer_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    return key;
}