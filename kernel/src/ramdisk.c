#include "ramdisk.h"
#include "console.h"

static inline int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static ramdisk_header_t *ramdisk_header = NULL;
static ramdisk_entry_t *ramdisk_entries = NULL;

void ramdisk_init(const void *ramdisk_data) {
    ramdisk_header = (ramdisk_header_t*)ramdisk_data;

    if (ramdisk_header->magic != RAMDISK_MAGIC) {
        console_write("[ramdisk] Invalid ramdisk magic\n");
        return;
    }

    ramdisk_entries = (ramdisk_entry_t*)((uint8_t*)ramdisk_data + sizeof(ramdisk_header_t));

    console_write("[ramdisk] Initialized: ");
    console_write_hex(ramdisk_header->file_count);
    console_write(" files, ");
    console_write_hex(ramdisk_header->total_size);
    console_write(" bytes\n");
}

void *ramdisk_find_file(const char *name, uint64_t *size) {
    if (!ramdisk_header || !ramdisk_entries) return NULL;

    for (uint32_t i = 0; i < ramdisk_header->file_count; i++) {
        ramdisk_entry_t *entry = &ramdisk_entries[i];
        if (strcmp(entry->name, name) == 0) {
            if (size) *size = entry->size;
            return (uint8_t*)ramdisk_header + entry->offset;
        }
    }
    return NULL;
}

int ramdisk_load_file(const char *name, void **data, uint64_t *size) {
    void *file_data = ramdisk_find_file(name, size);
    if (!file_data) return -1;
    *data = file_data;
    return 0;
}