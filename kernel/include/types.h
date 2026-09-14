#ifndef AEGIS_TYPES_H
#define AEGIS_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define AEGIS_PACKED __attribute__((packed))

typedef uint64_t phys_addr_t;
typedef uint64_t virt_addr_t;
typedef uint64_t task_id_t;
typedef uint64_t endpoint_id_t;

#endif
