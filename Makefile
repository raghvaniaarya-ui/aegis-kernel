# Aegis Kernel build
ARCH     := x86_64
TARGET   := $(ARCH)-elf
CC       := $(TARGET)-gcc
LD       := $(TARGET)-ld
OBJCOPY  := $(TARGET)-objcopy
NASM     := nasm

BUILD    := build
ISO      := $(BUILD)/aegis.iso

CFLAGS   := -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone \
            -mcmodel=kernel -Wall -Wextra -std=gnu11 \
            -Ikernel/include
ASFLAGS  := -f elf64
LDFLAGS  := -T linker.ld -nostdlib

KERNEL_C_SRCS := \
	kernel/src/main.c \
	kernel/src/console.c \
	kernel/src/ipc.c \
	kernel/src/mm.c \
	kernel/src/syscall.c

KERNEL_ASM_SRCS := \
	boot/boot.asm \
	kernel/arch/x86_64/entry.asm

KERNEL_C_OBJS   := $(patsubst %.c,$(BUILD)/%.o,$(KERNEL_C_SRCS))
KERNEL_ASM_OBJS := $(patsubst %.asm,$(BUILD)/%.o,$(KERNEL_ASM_SRCS))
KERNEL_OBJS     := $(KERNEL_C_OBJS) $(KERNEL_ASM_OBJS)

.PHONY: all clean run iso dirs

all: $(BUILD)/kernel.elf

dirs:
	@mkdir -p $(BUILD)/boot $(BUILD)/kernel/arch/x86_64 $(BUILD)/kernel/src

$(BUILD)/kernel.elf: dirs $(KERNEL_OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(NASM) $(ASFLAGS) $< -o $@

iso: all
	@mkdir -p $(BUILD)/iso/boot/grub
	cp $(BUILD)/kernel.elf $(BUILD)/iso/boot/
	cp grub.cfg $(BUILD)/iso/boot/grub/
	grub-mkrescue -o $(ISO) $(BUILD)/iso

run: all
	qemu-system-x86_64 -kernel $(BUILD)/kernel.elf -serial stdio -no-reboot -no-shutdown

clean:
	rm -rf $(BUILD)
