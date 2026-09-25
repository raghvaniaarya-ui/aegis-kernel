#ifndef AEGIS_MM_H
#define AEGIS_MM_H

#include "types.h"

#define AEGIS_PAGE_SIZE 4096

#define AEGIS_PTE_PRESENT    (1ULL << 0)
#define AEGIS_PTE_WRITABLE   (1ULL << 1)
#define AEGIS_PTE_USER       (1ULL << 2)
#define AEGIS_PTE_WRITE_THROUGH (1ULL << 3)
#define AEGIS_PTE_CACHE_DISABLE (1ULL << 4)
#define AEGIS_PTE_ACCESSED   (1ULL << 5)
#define AEGIS_PTE_DIRTY      (1ULL << 6)
#define AEGIS_PTE_HUGE       (1ULL << 7)
#define AEGIS_PTE_GLOBAL     (1ULL << 8)
#define AEGIS_PTE_NX         (1ULL << 63)

#define AEGIS_PTE_FLAGS_DEFAULT (AEGIS_PTE_PRESENT | AEGIS_PTE_WRITABLE)

typedef struct {
    phys_addr_t base;
    size_t      size;
} mem_region_t;

typedef uint64_t pte_t;

typedef struct {
    pte_t entries[512];
} AEGIS_PACKED page_table_t;

void mm_init(const void *multiboot_info);
void *mm_alloc(size_t size, size_t align);
phys_addr_t mm_phys_alloc(void);

/* Page table management */
page_table_t *mm_page_table_create(void);
void mm_page_table_destroy(page_table_t *pt);
int mm_page_map(page_table_t *pt, virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags);
void mm_page_unmap(page_table_t *pt, virt_addr_t vaddr);
pte_t *mm_page_walk(page_table_t *pt, virt_addr_t vaddr, int create);
void mm_switch_page_table(page_table_t *pt);
page_table_t *mm_kernel_page_table(void);

/* Memory protection */
int mm_page_protect(page_table_t *pt, virt_addr_t vaddr, uint64_t flags);

/* Virtual memory areas */
typedef struct vma {
    virt_addr_t start;
    virt_addr_t end;
    uint64_t flags;
    struct vma *next;
} vma_t;

int mm_vma_add(vma_t **list, virt_addr_t start, virt_addr_t end, uint64_t flags);
void mm_vma_remove(vma_t **list, virt_addr_t start);
vma_t *mm_vma_find(vma_t *list, virt_addr_t addr);

#endif