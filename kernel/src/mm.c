#include "mm.h"
#include "console.h"

#include <stdint.h>

#define KERNEL_HEAP_SIZE (256 * 1024)

static uint8_t heap_area[KERNEL_HEAP_SIZE];
static size_t heap_offset = 0;
static phys_addr_t next_phys_page = 0x100000; /* 1 MiB */

static page_table_t *kernel_page_table = NULL;
static page_table_t *current_page_table = NULL;

static size_t align_up(size_t value, size_t align) {
    return (value + align - 1) & ~(align - 1);
}

static page_table_t *page_table_alloc(void) {
    void *page = mm_alloc(AEGIS_PAGE_SIZE, AEGIS_PAGE_SIZE);
    if (!page) return NULL;
    return (page_table_t *)page;
}

void mm_init(const void *multiboot_info) {
    (void)multiboot_info;
    heap_offset = 0;

    /* Create kernel page table */
    kernel_page_table = page_table_alloc();
    if (!kernel_page_table) {
        console_write("[mm] Failed to create kernel page table\n");
        return;
    }

    /* Identity map first 4 MiB for kernel */
    for (phys_addr_t addr = 0; addr < 4 * 1024 * 1024; addr += AEGIS_PAGE_SIZE) {
        mm_page_map(kernel_page_table, addr, addr, AEGIS_PTE_FLAGS_DEFAULT | AEGIS_PTE_GLOBAL);
    }

    current_page_table = kernel_page_table;
    console_write("[mm] Initialized with page tables\n");
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

static page_table_t *page_table_get_entry(page_table_t *pt, uint16_t index, int create) {
    pte_t entry = pt->entries[index];
    if (entry & AEGIS_PTE_PRESENT) {
        return (page_table_t *)(entry & ~0xFFF);
    }
    if (!create) return NULL;

    page_table_t *new_pt = page_table_alloc();
    if (!new_pt) return NULL;

    pt->entries[index] = (uint64_t)new_pt | AEGIS_PTE_PRESENT | AEGIS_PTE_WRITABLE | AEGIS_PTE_USER;
    return new_pt;
}

page_table_t *mm_page_table_create(void) {
    return page_table_alloc();
}

void mm_page_table_destroy(page_table_t *pt) {
    /* In a real implementation, we'd free all sub-tables and pages */
    (void)pt;
}

int mm_page_map(page_table_t *pt, virt_addr_t vaddr, phys_addr_t paddr, uint64_t flags) {
    uint16_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint16_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    uint16_t pd_idx = (vaddr >> 21) & 0x1FF;
    uint16_t pt_idx = (vaddr >> 12) & 0x1FF;

    page_table_t *pml4 = pt;
    page_table_t *pdpt = page_table_get_entry(pml4, pml4_idx, 1);
    if (!pdpt) return -1;

    page_table_t *pd = page_table_get_entry(pdpt, pdpt_idx, 1);
    if (!pd) return -1;

    page_table_t *pt_page = page_table_get_entry(pd, pd_idx, 1);
    if (!pt_page) return -1;

    pte_t entry = (paddr & ~0xFFF) | flags | AEGIS_PTE_PRESENT;
    pt->entries[pt_idx] = entry;
    return 0;
}

void mm_page_unmap(page_table_t *pt, virt_addr_t vaddr) {
    uint16_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint16_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    uint16_t pd_idx = (vaddr >> 21) & 0x1FF;
    uint16_t pt_idx = (vaddr >> 12) & 0x1FF;

    page_table_t *pml4 = pt;
    page_table_t *pdpt = page_table_get_entry(pml4, pml4_idx, 0);
    if (!pdpt) return;

    page_table_t *pd = page_table_get_entry(pdpt, pdpt_idx, 0);
    if (!pd) return;

    page_table_t *pt_page = page_table_get_entry(pd, pd_idx, 0);
    if (!pt_page) return;

    pt->entries[pt_idx] = 0;
}

pte_t *mm_page_walk(page_table_t *pt, virt_addr_t vaddr, int create) {
    uint16_t pml4_idx = (vaddr >> 39) & 0x1FF;
    uint16_t pdpt_idx = (vaddr >> 30) & 0x1FF;
    uint16_t pd_idx = (vaddr >> 21) & 0x1FF;
    uint16_t pt_idx = (vaddr >> 12) & 0x1FF;

    page_table_t *pml4 = pt;
    page_table_t *pdpt = page_table_get_entry(pml4, pml4_idx, create);
    if (!pdpt) return NULL;

    page_table_t *pd = page_table_get_entry(pdpt, pdpt_idx, create);
    if (!pd) return NULL;

    page_table_t *pt_page = page_table_get_entry(pd, pd_idx, create);
    if (!pt_page) return NULL;

    return &pt->entries[pt_idx];
}

void mm_switch_page_table(page_table_t *pt) {
    current_page_table = pt;
    asm volatile("mov %0, %%cr3" :: "r"(pt) : "memory");
}

page_table_t *mm_kernel_page_table(void) {
    return kernel_page_table;
}

int mm_page_protect(page_table_t *pt, virt_addr_t vaddr, uint64_t flags) {
    pte_t *entry = mm_page_walk(pt, vaddr, 0);
    if (!entry || !(*entry & AEGIS_PTE_PRESENT)) return -1;
    *entry = (*entry & ~0xFFF) | flags;
    return 0;
}

int mm_vma_add(vma_t **list, virt_addr_t start, virt_addr_t end, uint64_t flags) {
    if (start >= end || start % AEGIS_PAGE_SIZE != 0 || end % AEGIS_PAGE_SIZE != 0) {
        return -1;
    }

    vma_t *new_vma = mm_alloc(sizeof(vma_t), 8);
    if (!new_vma) return -1;

    new_vma->start = start;
    new_vma->end = end;
    new_vma->flags = flags;
    new_vma->next = *list;
    *list = new_vma;
    return 0;
}

void mm_vma_remove(vma_t **list, virt_addr_t start) {
    vma_t **curr = list;
    while (*curr) {
        if ((*curr)->start == start) {
            *curr = (*curr)->next;
            return;
        }
        curr = &(*curr)->next;
    }
}

vma_t *mm_vma_find(vma_t *list, virt_addr_t addr) {
    for (vma_t *vma = list; vma; vma = vma->next) {
        if (addr >= vma->start && addr < vma->end) {
            return vma;
        }
    }
    return NULL;
}