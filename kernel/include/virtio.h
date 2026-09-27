#ifndef AEGIS_VIRTIO_H
#define AEGIS_VIRTIO_H

#include <stdint.h>
#include <stddef.h>
#include "types.h"

#define VIRTIO_MAGIC 0x74726976

#define VIRTIO_STATUS_ACKNOWLEDGE 1
#define VIRTIO_STATUS_DRIVER 2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEATURES_OK 8
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET 64
#define VIRTIO_STATUS_FAILED 128

#define VIRTIO_CONFIG_S_GUEST_PAGE_SIZE 4

#define VIRTIO_F_VERSION_1 32
#define VIRTIO_F_RING_INDIRECT_DESC 28
#define VIRTIO_F_RING_EVENT_IDX 29

#define VIRTIO_BLK_F_SIZE_MAX 1
#define VIRTIO_BLK_F_SEG_MAX 2
#define VIRTIO_BLK_F_GEOMETRY 4
#define VIRTIO_BLK_F_RO 5
#define VIRTIO_BLK_F_BLK_SIZE 6
#define VIRTIO_BLK_F_FLUSH 9
#define VIRTIO_BLK_F_TOPOLOGY 10
#define VIRTIO_BLK_F_CONFIG_WCE 11

#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1
#define VIRTIO_BLK_T_FLUSH 4
#define VIRTIO_BLK_T_GET_ID 8
#define VIRTIO_BLK_T_BARRIER 0x80000000

#define VIRTIO_BLK_S_OK 0
#define VIRTIO_BLK_S_IOERR 1
#define VIRTIO_BLK_S_UNSUPP 2

#define VIRTIO_MMIO_MAGIC_VALUE 0x000
#define VIRTIO_MMIO_VERSION 0x004
#define VIRTIO_MMIO_DEVICE_ID 0x008
#define VIRTIO_MMIO_VENDOR_ID 0x00c
#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DRIVER_FEATURES 0x014
#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x028
#define VIRTIO_MMIO_QUEUE_SEL 0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034
#define VIRTIO_MMIO_QUEUE_NUM 0x038
#define VIRTIO_MMIO_QUEUE_READY 0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060
#define VIRTIO_MMIO_INTERRUPT_ACK 0x064
#define VIRTIO_MMIO_STATUS 0x070

#define VIRTIO_QUEUE_SIZE 128

#define VIRTIO_DESC_F_NEXT 1
#define VIRTIO_DESC_F_WRITE 2
#define VIRTIO_DESC_F_INDIRECT 4

#define VIRTIO_DESC_F_NEXT 1
#define VIRTIO_DESC_F_WRITE 2
#define VIRTIO_DESC_F_INDIRECT 4

typedef struct {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} AEGIS_PACKED virtio_desc_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[VIRTIO_QUEUE_SIZE];
} AEGIS_PACKED virtio_avail_t;

typedef struct {
    uint32_t id;
    uint32_t len;
} AEGIS_PACKED virtio_used_elem_t;

typedef struct {
    uint16_t flags;
    uint16_t idx;
    virtio_used_elem_t ring[VIRTIO_QUEUE_SIZE];
} AEGIS_PACKED virtio_used_t;

typedef struct {
    virtio_desc_t desc[VIRTIO_QUEUE_SIZE];
    virtio_avail_t avail;
    uint8_t pad[6];
    virtio_used_t used;
} AEGIS_PACKED virtio_queue_t;

typedef struct {
    uint32_t type;
    uint32_t ioprio;
    uint64_t sector;
    uint8_t data[512];
    uint8_t status;
} AEGIS_PACKED virtio_blk_req_t;

typedef struct {
    uint64_t capacity;
    uint32_t size_max;
    uint32_t seg_max;
    struct {
        uint16_t cylinders;
        uint16_t heads;
        uint16_t sectors;
    } geometry;
    uint32_t blk_size;
    uint64_t topologies;
    uint8_t writeback;
} AEGIS_PACKED virtio_blk_config_t;

int virtio_init(uintptr_t base);
int virtio_blk_init(uintptr_t base);
int virtio_blk_read(uint64_t sector, void *buf, uint32_t count);
int virtio_blk_write(uint64_t sector, const void *buf, uint32_t count);
int virtio_blk_flush(void);
void virtio_handle_irq(void);

#endif