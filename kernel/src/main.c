#include "console.h"
#include "ipc.h"
#include "mm.h"
#include "syscall.h"

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

void kmain(const void *multiboot_info) {
    (void)syscall_dispatch;

    console_init();
    banner();

    console_write("[boot] multiboot info @ ");
    console_write_hex((uint64_t)multiboot_info);
    console_write("\n");

    mm_init(multiboot_info);
    ipc_init();

    void *block = mm_alloc(64, 16);
    console_write("[mm] allocated test block @ ");
    console_write_hex((uint64_t)block);
    console_write("\n");

    ipc_self_test();

    console_write("\n[ready] kernel idle — next: scheduler + init server\n");

    for (;;) {
        __asm__ volatile("hlt");
    }
}
