#include "mm.h"

#include <stdint.h>

#define KERNEL_HEAP_SIZE (256 * 1024)

static uint8_t heap_area[KERNEL_HEAP_SIZE];
static size_t heap_offset = 0;
static phys_addr_t next_phys_page = 0x100000; /* 1 MiB */

static size_t align_up(size_t value, size_t align) {
    return (value + align - 1) & ~(align - 1);
}

void mm_init(const void *multiboot_info) {
    (void)multiboot_info;
    heap_offset = 0;
}

void *mm_alloc(size_t size, size_t align) {
    size_t start = align_up(heap_offset, align);
    if (start + size > KERNEL_HEAP_SIZE) {
        return NULL;
    }
    heap_offset = start + size;
    return &heap_area[start];
}

phys_addr_t mm_phys_alloc(void) {
    phys_addr_t page = next_phys_page;
    next_phys_page += AEGIS_PAGE_SIZE;
    return page;
}
