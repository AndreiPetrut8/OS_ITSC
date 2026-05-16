[BITS 16]        
[ORG 0x7c00]     


CODE_OFFSET equ 0x8
DATA_OFFSET equ 0x10

KERNEL_LOAD_SEG equ 0x1000
KERNEL_START_ADDR equ 0x00100000



start:
	cli          
	xor ax, ax 
	mov ds, ax    
	mov es, ax    
	mov ss, ax    
	mov sp, 0x7C00
	sti           

	in   al, 0x92
	or   al, 0x02        ; setează bitul 1 -> activează A20
	and  al, 0xFE        
	out  0x92, al

;Load kernel
	mov ax, KERNEL_LOAD_SEG
	mov es, ax
	xor bx, bx
	
	mov dh, 0x00
	mov dl, 0x80
	mov ch, 0x00
	mov cl, 0x02
	mov ah, 0x02
	mov al, 128
	int 0x13

	jc disk_read_error


load_PM:
	cli
	lgdt [gdt_descriptor]
	
	mov eax, cr0
	or eax, 1
	mov cr0, eax
	
	jmp CODE_OFFSET:PModeMain


disk_read_error:
	hlt

;GDT Implemetation

gdt_start:
	dd 0x0
	dd 0x0

    ; Code segment descriptor
	dw 0xFFFF       
	dw 0x0000       
	db 0x00         
	db 10011010b    
	db 11001111b   
	db 0x00         

    ; Data segment descriptor
	dw 0xFFFF       
	dw 0x0000       
	db 0x00         
	db 10010010b    
	db 11001111b    
	db 0x00         

gdt_end:

gdt_descriptor:
	dw gdt_end - gdt_start - 1 
	dd gdt_start 


[BITS 32]
PModeMain:
	mov ax, DATA_OFFSET
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov ss, ax
	mov gs, ax
	
	mov ebp, 0x9C00
	mov esp, ebp

	cld
	
	mov esi, 0x00010000         
	mov edi, KERNEL_START_ADDR  
	mov ecx, (128 * 512) / 4  
	rep movsd
	
	jmp CODE_OFFSET:KERNEL_START_ADDR




times 510 - ($ - $$) db 0   

dw 0xAA55
