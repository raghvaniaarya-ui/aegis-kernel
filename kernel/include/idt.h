#ifndef AEGIS_IDT_H
#define AEGIS_IDT_H

#include <stdint.h>
#include "types.h"

#define IDT_ENTRIES 256

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} AEGIS_PACKED idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} AEGIS_PACKED idt_ptr_t;

#define IDT_TYPE_INTERRUPT 0x8E
#define IDT_TYPE_TRAP 0x8F

void idt_init(void);
void idt_set_gate(uint8_t num, uint64_t handler, uint16_t selector, uint8_t flags);
void idt_load(void);

#define ISR_DIVIDE_BY_ZERO 0
#define ISR_DEBUG 1
#define ISR_NMI 2
#define ISR_BREAKPOINT 3
#define ISR_OVERFLOW 4
#define ISR_BOUND_RANGE 5
#define ISR_INVALID_OPCODE 6
#define ISR_DEVICE_NOT_AVAILABLE 7
#define ISR_DOUBLE_FAULT 8
#define ISR_INVALID_TSS 10
#define ISR_SEGMENT_NOT_PRESENT 11
#define ISR_STACK_FAULT 12
#define ISR_GENERAL_PROTECTION 13
#define ISR_PAGE_FAULT 14
#define ISR_FPU_ERROR 16
#define ISR_ALIGNMENT_CHECK 17
#define ISR_MACHINE_CHECK 18
#define ISR_SIMD_FLOATING_POINT 19

#define IRQ_TIMER 0
#define IRQ_KEYBOARD 1
#define IRQ_CASCADE 2
#define IRQ_SERIAL2 3
#define IRQ_SERIAL1 4
#define IRQ_PARALLEL2 5
#define IRQ_FLOPPY 6
#define IRQ_PARALLEL1 7
#define IRQ_RTC 8
#define IRQ_MOUSE 12
#define IRQ_COPROCESSOR 13
#define IRQ_PRIMARY_ATA 14
#define IRQ_SECONDARY_ATA 15

void isr_handler(void *regs);
void irq_handler(void *regs);

#endif