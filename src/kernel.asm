[BITS 32]

global _start
extern kernel_main
extern gpf_handler_c
extern df_handler_c

_start:
    call kernel_main
    jmp $

; ------------------------------------------------------------
; GDT
; ------------------------------------------------------------
global gdt_flush
extern gp
gdt_flush:
    lgdt [gp]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush
.flush:
    ret

global tss_load
tss_load:
    mov ax, 0x28
    ltr ax
    ret

; ------------------------------------------------------------
; IDT
; ------------------------------------------------------------
global load_idt
load_idt:
    mov edx, [esp + 4]
    mov cx,  [esp + 8]
    sub esp, 6
    mov [esp], cx
    mov [esp+2], edx
    lidt [esp]
    add esp, 6
    ret

; ------------------------------------------------------------
; Syscall stub (ring 0)
; ------------------------------------------------------------
global isr_syscall_stub
extern syscall_handler
isr_syscall_stub:
    cli
    pusha
    mov eax, esp
    push eax
    call syscall_handler
    add esp, 4
    popa
    iretd

; ------------------------------------------------------------
; Timer stub with full context switch
; ------------------------------------------------------------
global isr_timer_stub
extern timer_handler_c

isr_timer_stub:
    pusha
    push esp
    call timer_handler_c
    add esp, 4
    mov esp, eax 
    mov al, 0x20
    out 0x20, al
    popa 
    iretd 

global isr_gpf_stub
global isr_df_stub

isr_gpf_stub:
    pusha
    push esp
    call gpf_handler_c
    add esp, 4
    popa
    add esp, 4
    iretd

isr_df_stub:
    cli
    pusha
    push esp
    call df_handler_c
    add esp, 4
    popa
    cli
    hlt
    jmp isr_df_stub

; ------------------------------------------------------------
; Built-in process wrappers (never return)
; ------------------------------------------------------------
extern process1
extern process2
extern process3

global proc1_wrapper
proc1_wrapper:
    call process1
    mov eax, 2
    int 0x80
    jmp proc1_wrapper

global proc2_wrapper
proc2_wrapper:
    call process2
    mov eax, 2
    int 0x80
    jmp proc2_wrapper

global proc3_wrapper
proc3_wrapper:
    call process3
    mov eax, 2
    int 0x80
    jmp proc3_wrapper

; ------------------------------------------------------------
; Old labels (kept for heap.c tests)
; ------------------------------------------------------------
global _proc1_start
global _proc1_end
global _proc2_start
global _proc2_end
global _proc3_start
global _proc3_end

_proc1_start:
    call process1
_proc1_end:

_proc2_start:
    call process2
_proc2_end:

_proc3_start:
    call process3
_proc3_end:

