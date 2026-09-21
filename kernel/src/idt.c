#include "idt.h"
#include "console.h"
#include "timer.h"
#include "keyboard.h"
#include "sched.h"

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

extern void *isr_stub_table[];
extern void *irq_stub_table[];

void idt_init(void) {
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;
    idt_ptr.base = (uint64_t)&idt;

    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, (uint64_t)isr_stub_table[i], 0x08, 0x8E);
    }

    idt_set_gate(IRQ_TIMER + 32, (uint64_t)irq_stub_table[IRQ_TIMER], 0x08, 0x8E);
    idt_set_gate(IRQ_KEYBOARD + 32, (uint64_t)irq_stub_table[IRQ_KEYBOARD], 0x08, 0x8E);

    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x00);
    outb(0xA1, 0x00);

    idt_load();

    timer_init(1000);
    keyboard_init();

    console_write("[idt] initialized\n");
}

void idt_load(void) {
    asm volatile("lidt %0" : : "m"(idt_ptr));
}

void idt_set_gate(uint8_t num, uint64_t handler, uint16_t selector, uint8_t flags) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].selector = selector;
    idt[num].ist = 0;
    idt[num].type_attr = flags;
    idt[num].offset_mid = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[num].zero = 0;
}

void isr_handler(void *regs) {
    console_write("[isr] interrupt ");
    console_write_hex(*(uint64_t*)regs);
    console_write("\n");
    for (;;) {
        __asm__ volatile("hlt");
    }
}

void irq_handler(void *regs) {
    uint64_t irq = *(uint64_t*)regs;
    switch (irq) {
        case IRQ_TIMER:
            timer_handler();
            break;
        case IRQ_KEYBOARD:
            keyboard_handler();
            break;
        default:
            break;
    }
    outb(0x20, 0x20);
    if (irq >= 8) outb(0xA0, 0x20);
}