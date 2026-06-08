CROSS = i686-elf

CC      = $(CROSS)-gcc
LD      = $(CROSS)-ld
OBJCOPY = $(CROSS)-objcopy

FILES = ./build/kernel.asm.o \
        ./build/kernel.o \
        ./build/heap.o \
        ./build/fs.o \
        ./build/ata.o \
        ./build/idt.o \
        ./build/uart.o \
        ./build/vga.o \
        ./build/pipe.o \
        ./build/pmm.o \
        ./build/vmm.o \
        ./build/gdt.o \
        ./build/tss.o \
        ./build/u1_bin.o \
        ./build/u2_bin.o

FLAGS = -g -ffreestanding -m32 -fno-pic -fno-pie \
         -fno-stack-protector -nostdlib -nostartfiles \
         -nodefaultlibs -Wall -O0 -Iinc

all: user_progs
	mkdir -p bin build
	nasm -f bin ./src/boot.asm -o ./bin/boot.bin
	nasm -f elf32 -g ./src/kernel.asm -o ./build/kernel.asm.o
	$(CC) -I./src $(FLAGS) -std=gnu99 -c ./src/kernel.c -o ./build/kernel.o
	$(CC) -I./src $(FLAGS) -std=gnu99 -c ./src/heap.c -o ./build/heap.o
	$(CC) -I./src $(FLAGS) -std=gnu99 -c ./src/fs.c -o ./build/fs.o
	$(CC) -I./src $(FLAGS) -std=gnu99 -c ./src/ata.c -o ./build/ata.o
	$(CC) -I./src $(FLAGS) -std=gnu99 -c ./src/idt.c -o ./build/idt.o
	$(CC) -I./src $(FLAGS) -std=gnu99 -c ./src/uart.c -o ./build/uart.o
	$(CC) -I./src $(FLAGS) -std=gnu99 -c ./src/vga.c -o ./build/vga.o
	$(CC) -I./src $(FLAGS) -std=gnu99 -c ./src/pipe.c -o ./build/pipe.o
	$(CC) -I./src $(FLAGS) -std=gnu99 -c ./src/pmm.c -o ./build/pmm.o
	$(CC) -I./src $(FLAGS) -std=gnu99 -c ./src/vmm.c -o ./build/vmm.o
	$(CC) -I./src $(FLAGS) -std=gnu99 -c ./src/gdt.c -o ./build/gdt.o
	$(CC) -I./src $(FLAGS) -std=gnu99 -c ./src/tss.c -o ./build/tss.o
	$(LD) -m elf_i386 -g -relocatable $(FILES) -o ./build/completeKernel.o
	$(CC) $(FLAGS) -T ./src/linkerScript.ld \
		-o ./bin/kernel.bin \
		-ffreestanding -O0 -nostdlib \
		./build/completeKernel.o
	rm -f ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=512 count=256 >> ./bin/os.bin

# User programs
user_progs:
	mkdir -p bin build
	$(CC) $(FLAGS) -c src/u1.c -o build/u1.o
	$(LD) -m elf_i386 -T src/user.ld -Ttext 0x400000 \
		--oformat binary build/u1.o -o bin/u1.bin
	$(OBJCOPY) -I binary -O elf32-i386 -B i386 \
		bin/u1.bin build/u1_bin.o
	$(CC) $(FLAGS) -c src/u2.c -o build/u2.o
	$(LD) -m elf_i386 -T src/user.ld -Ttext 0x500000 \
		--oformat binary build/u2.o -o bin/u2.bin
	$(OBJCOPY) -I binary -O elf32-i386 -B i386 \
		bin/u2.bin build/u2_bin.o

run: all
	qemu-system-x86_64 \
	-serial stdio \
	-device isa-debug-exit \
	-drive format=raw,file=./bin/os.bin

clean:
	rm -f ./bin/*.bin
	rm -f ./build/*.o
	rm -f ./build/*.elf
	rm -f ./build/*.out