#include "console.h"
#include "ipc.h"
#include "mm.h"
#include "syscall.h"
#include "sched.h"
#include "idt.h"
#include "elf.h"
#include "ramdisk.h"

static void banner(void) {
    console_write("\n");
    console_write("  ___                    _            \n");
    console_write(" / _ \\                  (_)           \n");
    console_write("| __/_   _ ___  ___  ___ _  __ _ _ __ \n");
    console_write("| _|| | | / __|/ _ \\/ __| |/ _` | '__|\n");
    console_write("| | | |_| \\__ \\  __/\\__ \\ | (_| | |   \n");
    console_write("|_|  \\__, |___/\\___||___/_|\\__, |_|   \n");
    console_write("      __/ |                 __/ |      \n");
    console_write("     |___/                 |___/       \n");
    console_write("\n");
    console_write("Aegis microkernel v0.1 — portfolio build\n\n");
}

static void ipc_self_test(void) {
    endpoint_id_t a = 0;
    endpoint_id_t b = 0;
    aegis_msg_t msg = {0};
    aegis_msg_t reply = {0};

    if (ipc_endpoint_create(&a) != 0 || ipc_endpoint_create(&b) != 0) {
        console_write("[ipc] endpoint creation failed\n");
        return;
    }

    msg.sender = a;
    msg.type = MSG_PING;
    msg.arg0 = 0xA6615;

    if (ipc_send(b, &msg) != 0) {
        console_write("[ipc] send failed\n");
        return;
    }

    if (ipc_recv(b, &reply) != 0 || reply.type != MSG_PING) {
        console_write("[ipc] recv failed\n");
        return;
    }

    console_write("[ipc] message delivered from endpoint ");
    console_write_hex(reply.sender);
    console_write(" type=");
    console_write_hex(reply.type);
    console_write(" arg0=");
    console_write_hex(reply.arg0);
    console_write("\n");
}

static void task_a(void) {
    for (;;) {
        console_write("[task_a] running\n");
        task_sleep(1000);
    }
}

static void task_b(void) {
    for (;;) {
        console_write("[task_b] running\n");
        task_sleep(2000);
    }
}

extern const uint8_t _ramdisk_start[];
extern const uint8_t _ramdisk_end[];

static void load_servers_from_ramdisk(void) {
    console_write("[boot] Initializing ramdisk...\n");
    ramdisk_init(_ramdisk_start);

    console_write("[boot] Loading servers from ramdisk...\n");

    uint64_t init_entry = 0;
    void *init_data = NULL;
    uint64_t init_size = 0;

    if (ramdisk_load_file("init.elf", &init_data, &init_size) == 0) {
        uint64_t entry_point = 0;
        if (elf_load(init_data, &entry_point) == 0) {
            console_write("[boot] Loaded init.elf at 0x");
            console_write_hex(entry_point);
            console_write("\n");
            // TODO: Actually spawn the init server
        }
    } else {
        console_write("[boot] init.elf not found in ramdisk\n");
    }

    void *vfs_data = NULL;
    uint64_t vfs_size = 0;
    if (ramdisk_load_file("vfs.elf", &vfs_data, &vfs_size) == 0) {
        uint64_t entry_point = 0;
        if (elf_load(vfs_data, &entry_point) == 0) {
            console_write("[boot] Loaded vfs.elf at 0x");
            console_write_hex(entry_point);
            console_write("\n");
        }
    }
}

void kmain(const void *multiboot_info) {
    (void)syscall_dispatch;

    console_init();
    banner();

    console_write("[boot] multiboot info @ ");
    console_write_hex((uint64_t)multiboot_info);
    console_write("\n");

    mm_init(multiboot_info);
    ipc_init();
    idt_init();
    sched_init();

    task_create(task_a);
    task_create(task_b);

    void *block = mm_alloc(64, 16);
    console_write("[mm] allocated test block @ ");
    console_write_hex((uint64_t)block);
    console_write("\n");

    ipc_self_test();

    load_servers_from_ramdisk();

    console_write("\n[ready] starting scheduler...\n");
    sched_start();
}
