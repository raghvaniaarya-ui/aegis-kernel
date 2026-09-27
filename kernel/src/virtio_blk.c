#include "virtio.h"
#include "console.h"
#include "mm.h"

static uintptr_t virtio_base = 0;
static virtio_queue_t *virtio_queue = NULL;
static volatile uint32_t *virtio_mmio = NULL;
static virtio_blk_config_t blk_config = {0};

static inline void mmio_write(uint32_t offset, uint32_t value) {
    virtio_mmio[offset / 4] = value;
}

static inline uint32_t mmio_read(uint32_t offset) {
    return virtio_mmio[offset / 4];
}

static inline void mmio_write64(uint32_t offset, uint64_t value) {
    mmio_write(offset, (uint32_t)value);
    mmio_write(offset + 4, (uint32_t)(value >> 32));
}

static inline uint64_t mmio_read64(uint32_t offset) {
    return ((uint64_t)mmio_read(offset + 4) << 32) | mmio_read(offset);
}

static void virtio_set_status(uint32_t status) {
    mmio_write(VIRTIO_MMIO_STATUS, status);
}

static void virtio_ack_irq(uint32_t mask) {
    mmio_write(VIRTIO_MMIO_INTERRUPT_ACK, mask);
}

static void virtio_queue_setup(void) {
    virtio_queue = (virtio_queue_t *)mm_alloc(AEGIS_PAGE_SIZE, AEGIS_PAGE_SIZE);
    if (!virtio_queue) {
        console_write("[virtio] Failed to allocate queue\n");
        return;
    }
    
    for (int i = 0; i < VIRTIO_QUEUE_SIZE; i++) {
        virtio_queue->desc[i].addr = 0;
        virtio_queue->desc[i].len = 0;
        virtio_queue->desc[i].flags = 0;
        virtio_queue->desc[i].next = 0;
    }
    
    virtio_queue->avail.flags = 0;
    virtio_queue->avail.idx = 0;
    for (int i = 0; i < VIRTIO_QUEUE_SIZE; i++) {
        virtio_queue->avail.ring[i] = 0;
    }
    
    virtio_queue->used.flags = 0;
    virtio_queue->used.idx = 0;
    for (int i = 0; i < VIRTIO_QUEUE_SIZE; i++) {
        virtio_queue->used.ring[i].id = 0;
        virtio_queue->used.ring[i].len = 0;
    }
    
    mmio_write(VIRTIO_MMIO_QUEUE_SEL, 0);
    mmio_write(VIRTIO_MMIO_QUEUE_NUM, VIRTIO_QUEUE_SIZE);
    mmio_write(VIRTIO_MMIO_QUEUE_READY, 1);
    
    mmio_write64(VIRTIO_MMIO_QUEUE_READY, (uint64_t)virtio_queue);
}

static void virtio_negotiate_features(void) {
    uint32_t device_features = mmio_read(VIRTIO_MMIO_DEVICE_FEATURES);
    console_write("[virtio] Device features: 0x");
    console_write_hex(device_features);
    console_write("\n");
    
    uint32_t driver_features = 0;
    driver_features |= (1u << VIRTIO_F_VERSION_1);
    driver_features |= (1u << VIRTIO_F_RING_INDIRECT_DESC);
    driver_features |= (1u << VIRTIO_F_RING_EVENT_IDX);
    
    mmio_write(VIRTIO_MMIO_DRIVER_FEATURES, driver_features);
    console_write("[virtio] Driver features: 0x");
    console_write_hex(driver_features);
    console_write("\n");
}

static void virtio_read_config(void) {
    uint64_t capacity = mmio_read64(VIRTIO_MMIO_DEVICE_FEATURES + 0x100);
    blk_config.capacity = capacity;
    
    uint32_t blk_size = mmio_read(VIRTIO_MMIO_DEVICE_FEATURES + 0x108);
    if (blk_size) {
        blk_config.blk_size = blk_size;
    } else {
        blk_config.blk_size = 512;
    }
    
    blk_config.size_max = mmio_read(VIRTIO_MMIO_DEVICE_FEATURES + 0x10c);
    blk_config.seg_max = mmio_read(VIRTIO_MMIO_DEVICE_FEATURES + 0x110);
    
    uint16_t cylinders = mmio_read(VIRTIO_MMIO_DEVICE_FEATURES + 0x114) & 0xffff;
    uint16_t heads = (mmio_read(VIRTIO_MMIO_DEVICE_FEATURES + 0x114) >> 16) & 0xffff;
    uint16_t sectors = mmio_read(VIRTIO_MMIO_DEVICE_FEATURES + 0x118) & 0xffff;
    blk_config.geometry.cylinders = cylinders;
    blk_config.geometry.heads = heads;
    blk_config.geometry.sectors = sectors;
    
    blk_config.blk_size = mmio_read(VIRTIO_MMIO_DEVICE_FEATURES + 0x11c);
    blk_config.topologies = mmio_read(VIRTIO_MMIO_DEVICE_FEATURES + 0x120);
    blk_config.writeback = mmio_read(VIRTIO_MMIO_DEVICE_FEATURES + 0x124) & 0xff;
    
    console_write("[virtio-blk] Capacity: ");
    console_write_hex(blk_config.capacity);
    console_write(" sectors, block size: ");
    console_write_dec(blk_config.blk_size);
    console_write("\n");
}

int virtio_init(uintptr_t base) {
    virtio_base = base;
    virtio_mmio = (volatile uint32_t *)base;
    
    uint32_t magic = mmio_read(VIRTIO_MMIO_MAGIC_VALUE);
    if (magic != VIRTIO_MMIO_MAGIC_VALUE) {
        console_write("[virtio] Invalid magic value: 0x");
        console_write_hex(magic);
        console_write("\n");
        return -1;
    }
    
    uint32_t version = mmio_read(VIRTIO_MMIO_VERSION);
    if (version != 1 && version != 2) {
        console_write("[virtio] Unsupported version: ");
        console_write_dec(version);
        console_write("\n");
        return -1;
    }
    
    uint32_t device_id = mmio_read(VIRTIO_MMIO_DEVICE_ID);
    console_write("[virtio] Device ID: 0x");
    console_write_hex(device_id);
    console_write("\n");
    
    if (device_id != 2) {
        console_write("[virtio] Not a block device\n");
        return -1;
    }
    
    uint32_t vendor_id = mmio_read(VIRTIO_MMIO_VENDOR_ID);
    if (vendor_id != 0x1af4) {
        console_write("[virtio] Unknown vendor: 0x");
        console_write_hex(vendor_id);
        console_write("\n");
        return -1;
    }
    
    console_write("[virtio] Virtio block device found\n");
    
    virtio_set_status(VIRTIO_STATUS_ACKNOWLEDGE);
    virtio_set_status(VIRTIO_STATUS_DRIVER);
    
    virtio_negotiate_features();
    
    virtio_set_status(VIRTIO_STATUS_FEATURES_OK);
    
    virtio_queue_setup();
    
    virtio_read_config();
    
    virtio_set_status(VIRTIO_STATUS_DRIVER_OK);
    
    console_write("[virtio] Initialization complete\n");
    return 0;
}

static void virtio_submit_request(virtio_blk_req_t *req, uint32_t type, uint64_t sector) {
    req->type = type;
    req->ioprio = 0;
    req->sector = sector;
    
    virtio_desc_t *desc = &virtio_queue->desc[0];
    virtio_desc_t *desc_data = &virtio_queue->desc[1];
    virtio_desc_t *desc_status = &virtio_queue->desc[2];
    
    desc->addr = (uint64_t)req;
    desc->len = sizeof(virtio_blk_req_t);
    desc->flags = VIRTIO_DESC_F_NEXT;
    desc->next = 1;
    
    desc_data->addr = (uint64_t)((uint8_t *)virtio_queue + 512);
    desc_data->len = 512;
    desc_data->flags = (type == VIRTIO_BLK_T_OUT) ? VIRTIO_DESC_F_WRITE : 0;
    desc_data->flags |= VIRTIO_DESC_F_NEXT;
    desc_data->next = 2;
    
    virtio_queue->desc[2].addr = (uint64_t)&virtio_queue->used.ring[0].id;
    virtio_queue->desc[2].len = 1;
    virtio_queue->desc[2].flags = VIRTIO_DESC_F_WRITE;
    virtio_queue->desc[2].next = 0;
    
    virtio_queue->avail.ring[virtio_queue->avail.idx % VIRTIO_QUEUE_SIZE] = 0;
    virtio_queue->avail.idx++;
    
    mmio_write(VIRTIO_MMIO_QUEUE_NOTIFY, 0);
}

int virtio_blk_read(uint64_t sector, void *buf, uint32_t count) {
    if (!virtio_mmio) return -1;
    if (count == 0) return 0;
    
    virtio_blk_req_t *req = (virtio_blk_req_t *)mm_alloc(sizeof(virtio_blk_req_t), 16);
    if (!req) return -1;
    
    virtio_submit_request(req, VIRTIO_BLK_T_IN, sector);
    
    while (virtio_queue->used.idx == virtio_queue->avail.idx - 1) {
        __asm__ volatile("pause");
    }
    
    if (virtio_queue->used.ring[0].id != 0) {
        return -1;
    }
    
    return 0;
}

int virtio_blk_write(uint64_t sector, const void *buf, uint32_t count) {
    if (!virtio_mmio) return -1;
    if (count == 0) return 0;
    
    static virtio_blk_req_t req;
    virtio_submit_request(&req, VIRTIO_BLK_T_OUT, sector);
    
    while (virtio_queue->used.idx == virtio_queue->avail.idx - 1) {
        __asm__ volatile("pause");
    }
    
    if (virtio_queue->used.ring[0].id != 0) {
        return -1;
    }
    
    return 0;
}

int virtio_blk_flush(void) {
    if (!virtio_mmio) return -1;
    
    static virtio_blk_req_t req;
    virtio_submit_request(&req, VIRTIO_BLK_T_FLUSH, 0);
    
    while (virtio_queue->used.idx == virtio_queue->avail.idx - 1) {
        __asm__ volatile("pause");
    }
    
    if (virtio_queue->used.ring[0].id != 0) {
        return -1;
    }
    
    return 0;
}

void virtio_handle_irq(void) {
    uint32_t status = mmio_read(VIRTIO_MMIO_INTERRUPT_STATUS);
    if (!status) return;
    
    virtio_ack_irq(status);
    
    if (virtio_queue->used.idx > virtio_queue->avail.idx - 1) {
    }
}