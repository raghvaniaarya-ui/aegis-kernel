#ifndef AEGIS_MM_H
#define AEGIS_MM_H

#include "types.h"

#define AEGIS_PAGE_SIZE 4096

typedef struct {
    phys_addr_t base;
    size_t      size;
} mem_region_t;

void mm_init(const void *multiboot_info);
void *mm_alloc(size_t size, size_t align);
phys_addr_t mm_phys_alloc(void);

#endif
