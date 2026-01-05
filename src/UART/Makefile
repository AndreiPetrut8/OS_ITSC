CC=gcc
LD=ld
ASM=nasm

CFLAGS=-m32 -ffreestanding -O2 -Wall -Wextra
LDFLAGS=-m elf_i386 -T linker.ld -nostdlib

all: kernel.bin

kernel.bin: boot.o kernel.o
	$(LD) $(LDFLAGS) -o kernel.elf $^
	objcopy -O binary kernel.elf kernel.bin

boot.o: boot.asm
	$(ASM) -f elf32 boot.asm -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

run: kernel.bin
	qemu-system-i386 -kernel kernel.elf -nographic

clean:
	rm -f *.o *.elf *.bin
