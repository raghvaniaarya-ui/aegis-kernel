#include "virtio.h"
#include "console.h"
#include "mm.h"
#include <string.h>

static uintptr_t virtio_net_base = 0;
static virtio_queue_t *virtio_net_tx_queue = NULL;
static virtio_queue_t *virtio_net_rx_queue = NULL;
static volatile uint32_t *virtio_net_mmio = NULL;
static virtio_net_config_t net_config = {0};
static uint8_t tx_buffer[2048];
static uint8_t rx_buffer[2048];
static volatile int tx_ready = 1;
static volatile int rx_ready = 0;
static volatile uint32_t rx_len = 0;

static inline void mmio_write(uint32_t offset, uint32_t value) {
    virtio_net_mmio[offset / 4] = value;
}

static inline uint32_t mmio_read(uint32_t offset) {
    return virtio_net_mmio[offset / 4];
}

static inline void mmio_write64(uint32_t offset, uint64_t value) {
    mmio_write(offset, (uint32_t)value);
    mmio_write(offset + 4, (uint32_t)(value >> 32));
}

static void virtio_net_set_status(uint32_t status) {
    mmio_write(VIRTIO_MMIO_STATUS, status);
}

static void virtio_net_ack_irq(uint32_t mask) {
    mmio_write(VIRTIO_MMIO_INTERRUPT_ACK, mask);
}

static void virtio_net_queue_setup(int queue_idx, virtio_queue_t **queue) {
    *queue = (virtio_queue_t *)mm_alloc(AEGIS_PAGE_SIZE, AEGIS_PAGE_SIZE);
    if (!*queue) {
        console_write("[virtio-net] Failed to allocate queue ");
        console_write_dec(queue_idx);
        console_write("\n");
        return;
    }
    
    for (int i = 0; i < VIRTIO_QUEUE_SIZE; i++) {
        (*queue)->desc[i].addr = 0;
        (*queue)->desc[i].len = 0;
        (*queue)->desc[i].flags = 0;
        (*queue)->desc[i].next = 0;
    }
    
    (*queue)->avail.flags = 0;
    (*queue)->avail.idx = 0;
    for (int i = 0; i < VIRTIO_QUEUE_SIZE; i++) {
        (*queue)->avail.ring[i] = 0;
    }
    
    (*queue)->used.flags = 0;
    (*queue)->used.idx = 0;
    for (int i = 0; i < VIRTIO_QUEUE_SIZE; i++) {
        (*queue)->used.ring[i].id = 0;
        (*queue)->used.ring[i].len = 0;
    }
    
    mmio_write(VIRTIO_MMIO_QUEUE_SEL, queue_idx);
    mmio_write(VIRTIO_MMIO_QUEUE_NUM, VIRTIO_QUEUE_SIZE);
    mmio_write(VIRTIO_MMIO_QUEUE_READY, 1);
    mmio_write64(VIRTIO_MMIO_QUEUE_READY, (uint64_t)*queue);
}

static void virtio_net_negotiate_features(void) {
    uint32_t device_features = mmio_read(VIRTIO_MMIO_DEVICE_FEATURES);
    console_write("[virtio-net] Device features: 0x");
    console_write_hex(device_features);
    console_write("\n");
    
    uint32_t driver_features = 0;
    driver_features |= (1u << VIRTIO_F_VERSION_1);
    driver_features |= (1u << VIRTIO_F_RING_INDIRECT_DESC);
    driver_features |= (1u << VIRTIO_F_RING_EVENT_IDX);
    driver_features |= (1u << VIRTIO_NET_F_MAC);
    driver_features |= (1u << VIRTIO_NET_F_STATUS);
    driver_features |= (1u << VIRTIO_NET_F_MRG_RXBUF);
    
    mmio_write(VIRTIO_MMIO_DRIVER_FEATURES, driver_features);
    console_write("[virtio-net] Driver features: 0x");
    console_write_hex(driver_features);
    console_write("\n");
}

static void virtio_net_read_config(void) {
    for (int i = 0; i < 6; i++) {
        net_config.mac[i] = mmio_read(VIRTIO_MMIO_DEVICE_FEATURES + 0x100 + i);
    }
    net_config.status = mmio_read(VIRTIO_MMIO_DEVICE_FEATURES + 0x106);
    net_config.max_virtqueue_pairs = mmio_read(VIRTIO_MMIO_DEVICE_FEATURES + 0x108);
    net_config.mtu = mmio_read(VIRTIO_MMIO_DEVICE_FEATURES + 0x10a);
    
    console_write("[virtio-net] MAC: ");
    for (int i = 0; i < 6; i++) {
        console_write_hex(net_config.mac[i]);
        if (i < 5) console_write(":");
    }
    console_write("\n");
    console_write("[virtio-net] Status: 0x");
    console_write_hex(net_config.status);
    console_write(", MTU: ");
    console_write_dec(net_config.mtu);
    console_write("\n");
}

static void virtio_net_setup_rx(void) {
    if (!virtio_net_rx_queue) return;
    
    virtio_desc_t *desc = &virtio_net_rx_queue->desc[0];
    virtio_desc_t *desc_data = &virtio_net_rx_queue->desc[1];
    
    desc->addr = (uint64_t)rx_buffer;
    desc->len = sizeof(virtio_net_hdr_t);
    desc->flags = VIRTIO_DESC_F_NEXT;
    desc->next = 1;
    
    desc_data->addr = (uint64_t)(rx_buffer + sizeof(virtio_net_hdr_t));
    desc_data->len = sizeof(rx_buffer) - sizeof(virtio_net_hdr_t);
    desc_data->flags = VIRTIO_DESC_F_WRITE | VIRTIO_DESC_F_NEXT;
    desc_data->next = 0;
    
    virtio_net_rx_queue->avail.ring[virtio_net_rx_queue->avail.idx % VIRTIO_QUEUE_SIZE] = 0;
    virtio_net_rx_queue->avail.idx++;
    
    mmio_write(VIRTIO_MMIO_QUEUE_SEL, 1);
    mmio_write(VIRTIO_MMIO_QUEUE_NOTIFY, 1);
}

int virtio_net_init(uintptr_t base) {
    virtio_net_base = base;
    virtio_net_mmio = (volatile uint32_t *)base;
    
    uint32_t magic = mmio_read(VIRTIO_MMIO_MAGIC_VALUE);
    if (magic != VIRTIO_MMIO_MAGIC_VALUE) {
        console_write("[virtio-net] Invalid magic value: 0x");
        console_write_hex(magic);
        console_write("\n");
        return -1;
    }
    
    uint32_t version = mmio_read(VIRTIO_MMIO_VERSION);
    if (version != 1 && version != 2) {
        console_write("[virtio-net] Unsupported version: ");
        console_write_dec(version);
        console_write("\n");
        return -1;
    }
    
    uint32_t device_id = mmio_read(VIRTIO_MMIO_DEVICE_ID);
    console_write("[virtio-net] Device ID: 0x");
    console_write_hex(device_id);
    console_write("\n");
    
    if (device_id != 1) {
        console_write("[virtio-net] Not a network device\n");
        return -1;
    }
    
    uint32_t vendor_id = mmio_read(VIRTIO_MMIO_VENDOR_ID);
    if (vendor_id != 0x1af4) {
        console_write("[virtio-net] Unknown vendor: 0x");
        console_write_hex(vendor_id);
        console_write("\n");
        return -1;
    }
    
    console_write("[virtio-net] Virtio network device found\n");
    
    virtio_net_set_status(VIRTIO_STATUS_ACKNOWLEDGE);
    virtio_net_set_status(VIRTIO_STATUS_DRIVER);
    
    virtio_net_negotiate_features();
    
    virtio_net_set_status(VIRTIO_STATUS_FEATURES_OK);
    
    virtio_net_queue_setup(0, &virtio_net_tx_queue);
    virtio_net_queue_setup(1, &virtio_net_rx_queue);
    
    virtio_net_read_config();
    
    virtio_net_set_status(VIRTIO_STATUS_DRIVER_OK);
    
    virtio_net_setup_rx();
    
    console_write("[virtio-net] Initialization complete\n");
    return 0;
}

int virtio_net_send(const void *data, uint32_t len) {
    if (!virtio_net_mmio || !virtio_net_tx_queue) return -1;
    if (len > sizeof(tx_buffer) - sizeof(virtio_net_hdr_t)) return -1;
    
    while (!tx_ready) {
        __asm__ volatile("pause");
    }
    tx_ready = 0;
    
    virtio_net_hdr_t *hdr = (virtio_net_hdr_t *)tx_buffer;
    hdr->flags = 0;
    hdr->gso_type = 0;
    hdr->hdr_len = 0;
    hdr->gso_size = 0;
    hdr->csum_start = 0;
    hdr->csum_offset = 0;
    hdr->num_buffers = 1;
    
    memcpy(tx_buffer + sizeof(virtio_net_hdr_t), data, len);
    
    virtio_desc_t *desc = &virtio_net_tx_queue->desc[0];
    virtio_desc_t *desc_data = &virtio_net_tx_queue->desc[1];
    
    desc->addr = (uint64_t)tx_buffer;
    desc->len = sizeof(virtio_net_hdr_t);
    desc->flags = VIRTIO_DESC_F_NEXT;
    desc->next = 1;
    
    desc_data->addr = (uint64_t)(tx_buffer + sizeof(virtio_net_hdr_t));
    desc_data->len = len;
    desc_data->flags = 0;
    desc_data->next = 0;
    
    virtio_net_tx_queue->avail.ring[virtio_net_tx_queue->avail.idx % VIRTIO_QUEUE_SIZE] = 0;
    virtio_net_tx_queue->avail.idx++;
    
    mmio_write(VIRTIO_MMIO_QUEUE_SEL, 0);
    mmio_write(VIRTIO_MMIO_QUEUE_NOTIFY, 0);
    
    return 0;
}

int virtio_net_recv(void *buf, uint32_t max_len) {
    if (!virtio_net_mmio || !virtio_net_rx_queue) return -1;
    if (!rx_ready) return 0;
    
    virtio_net_hdr_t *hdr = (virtio_net_hdr_t *)rx_buffer;
    uint32_t data_len = rx_len;
    
    if (data_len > max_len) data_len = max_len;
    memcpy(buf, rx_buffer + sizeof(virtio_net_hdr_t), data_len);
    
    rx_ready = 0;
    virtio_net_setup_rx();
    
    return data_len;
}

uint8_t *virtio_net_get_mac(void) {
    return net_config.mac;
}

void virtio_net_handle_irq(void) {
    if (!virtio_net_mmio) return;
    
    uint32_t status = mmio_read(VIRTIO_MMIO_INTERRUPT_STATUS);
    if (!status) return;
    
    virtio_net_ack_irq(status);
    
    mmio_write(VIRTIO_MMIO_QUEUE_SEL, 0);
    if (virtio_net_tx_queue && virtio_net_tx_queue->used.idx > 0) {
        tx_ready = 1;
    }
    
    mmio_write(VIRTIO_MMIO_QUEUE_SEL, 1);
    if (virtio_net_rx_queue && virtio_net_rx_queue->used.idx > 0) {
        uint32_t idx = (virtio_net_rx_queue->used.idx - 1) % VIRTIO_QUEUE_SIZE;
        uint32_t used_id = virtio_net_rx_queue->used.ring[idx].id;
        rx_len = virtio_net_rx_queue->used.ring[idx].len;
        if (rx_len > sizeof(virtio_net_hdr_t)) {
            rx_len -= sizeof(virtio_net_hdr_t);
        } else {
            rx_len = 0;
        }
        rx_ready = 1;
    }
}