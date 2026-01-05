[BITS 32]

	global _start
	extern kernel_main

_start:
	call kernel_main
	jmp $

	global load_idt
	global isr_syscall_stub
	extern syscall_handler

load_idt:
	mov edx, [esp + 4]  
	mov cx, [esp + 8]  
	
	sub esp, 6
	mov [esp], cx       
	mov [esp+2], edx    
	lidt [esp]          
	add esp, 6
	ret

isr_syscall_stub:
	cli                 
	pusha              
				
	mov eax, esp       
	push eax            
	call syscall_handler 
	add esp, 4          
	popa               
	iretd

	extern timer_handler
	global isr_timer_stub

isr_timer_stub:
	pusha
	call timer_handler
	
	mov al, 0x20
	out 0x20, al
	
	popa
	iretd
	
times 512-($ - $$) db 0
