#ifndef AEGIS_RAMDISK_H
#define AEGIS_RAMDISK_H

#include <stdint.h>
#include "types.h"

#define RAMDISK_MAGIC 0x444B4D52

typedef struct {
    uint32_t magic;
    uint32_t file_count;
    uint64_t total_size;
} AEGIS_PACKED ramdisk_header_t;

typedef struct {
    char name[64];
    uint64_t offset;
    uint64_t size;
    uint32_t flags;
} AEGIS_PACKED ramdisk_entry_t;

void ramdisk_init(const void *ramdisk_data);
void *ramdisk_find_file(const char *name, uint64_t *size);
int ramdisk_load_file(const char *name, void **data, uint64_t *size);

#endif