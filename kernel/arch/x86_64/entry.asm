; Kernel entry helpers (stack, CPU features) — expand as the kernel grows.
section .text

global load_kernel_stack
load_kernel_stack:
    mov rsp, rdi
    ret
