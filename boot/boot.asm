; Multiboot2 header — see https://www.gnu.org/software/grub/manual/multiboot2/
section .multiboot_header
align 8
header_start:
    dd 0xe85250d6                ; magic
    dd 0                         ; architecture (i386)
    dd header_end - header_start ; header length
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; end tag
    dw 0
    dw 0
    dd 8
header_end:

section .text
extern kmain

global _start
_start:
    ; Multiboot2 passes:
    ;   rax = magic (0x36d76289)
    ;   rbx = multiboot info pointer
    mov rdi, rbx
    call kmain

.hang:
    cli
    hlt
    jmp .hang
