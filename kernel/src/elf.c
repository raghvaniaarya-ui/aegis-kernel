#include "elf.h"
#include "console.h"
#include "mm.h"

static inline void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

static inline void *memset(void *s, int c, size_t n) {
    uint8_t *p = s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

int elf_load(const void *elf_data, uint64_t *entry_point) {
    const elf64_ehdr_t *ehdr = (const elf64_ehdr_t*)elf_data;

    if (ehdr->e_ident[0] != 0x7F ||
        ehdr->e_ident[1] != 'E' ||
        ehdr->e_ident[2] != 'L' ||
        ehdr->e_ident[3] != 'F') {
        console_write("[elf] Invalid ELF magic\n");
        return -1;
    }

    if (ehdr->e_ident[4] != ELF_CLASS_64) {
        console_write("[elf] Not 64-bit ELF\n");
        return -1;
    }

    if (ehdr->e_ident[5] != ELF_DATA_LSB) {
        console_write("[elf] Not little-endian\n");
        return -1;
    }

    if (ehdr->e_machine != ELF_MACHINE_X86_64) {
        console_write("[elf] Not x86_64\n");
        return -1;
    }

    if (ehdr->e_type != ELF_TYPE_EXEC) {
        console_write("[elf] Not executable type\n");
        return -1;
    }

    console_write("[elf] Loading ELF binary...\n");
    console_write("[elf] Entry point: 0x");
    console_write_hex(ehdr->e_entry);
    console_write("\n");

    *entry_point = ehdr->e_entry;

    const elf64_phdr_t *phdrs = (const elf64_phdr_t*)((uint8_t*)elf_data + ehdr->e_phoff);

    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        const elf64_phdr_t *phdr = &phdrs[i];

        if (phdr->p_type != ELF_PT_LOAD) continue;

        console_write("[elf] Loading segment ");
        console_write_hex(i);
        console_write(" at 0x");
        console_write_hex(phdr->p_vaddr);
        console_write(" size 0x");
        console_write_hex(phdr->p_memsz);
        console_write("\n");

        void *dst = (void*)phdr->p_vaddr;
        const void *src = (const uint8_t*)elf_data + phdr->p_offset;

        mm_alloc(phdr->p_memsz, 4096);

        memcpy(dst, src, phdr->p_filesz);

        if (phdr->p_memsz > phdr->p_filesz) {
            uint8_t *bss = (uint8_t*)dst + phdr->p_filesz;
            memset(bss, 0, phdr->p_memsz - phdr->p_filesz);
        }
    }

    console_write("[elf] Load complete\n");
    return 0;
}

void *elf_load_segment(const void *elf_data, const elf64_phdr_t *phdr, uint64_t *vaddr) {
    *vaddr = phdr->p_vaddr;
    return (void*)((uint8_t*)elf_data + phdr->p_offset);
}