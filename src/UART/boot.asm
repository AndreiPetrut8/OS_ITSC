BITS 32
GLOBAL _start
EXTERN kernel_main

section .multiboot
align 4
    dd 0x1BADB002        ; magic
    dd 0x00000000        ; flags
    dd -(0x1BADB002)     ; checksum

section .text
_start:
    cli
    call kernel_main
.hang:
    hlt
    jmp .hang

section .note.GNU-stack noalloc noexec nowrite
