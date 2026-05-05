# Definim toate obiectele necesare
FILES = ./build/kernel.asm.o \
        ./build/kernel.o \
        ./build/heap.o \
        ./build/fs.o \
        ./build/ata.o \
        ./build/idt.o \
        ./build/uart.o \
        ./build/u1_bin.o \
        ./build/u2_bin.o

FLAGS = -g -ffreestanding -m32 -fno-pic -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc

all: user_progs ./bin/os.bin

# Regula principala pentru imaginea OS
./bin/os.bin: ./bin/boot.bin ./bin/kernel.bin
	dd if=./bin/boot.bin > ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=512 count=256 >> ./bin/os.bin

# Bootloader
./bin/boot.bin: ./src/boot.asm
	nasm -f bin ./src/boot.asm -o ./bin/boot.bin

# Kernel ASM
./build/kernel.asm.o: ./src/kernel.asm
	nasm -f elf32 -g ./src/kernel.asm -o ./build/kernel.asm.o

# Compilare fisiere C (Regula generica pentru a evita repetitia)
./build/%.o: ./src/%.c
	gcc -I./src $(FLAGS) -std=gnu99 -c $< -o $@

# Link-uire Kernel
./bin/kernel.bin: $(FILES)
	ld -m elf_i386 -g -relocatable $(FILES) -o ./build/completeKernel.o
	gcc $(FLAGS) -T ./src/linkerScript.ld -o ./bin/kernel.bin -ffreestanding -O0 -nostdlib ./build/completeKernel.o

user_progs:
	# User program 1
	gcc $(FLAGS) -c src/u1.c -o build/u1.o
	ld -m elf_i386 -T src/user.ld -Ttext 0x400000 --oformat binary build/u1.o -o bin/u1.bin
	objcopy -I binary -O elf32-i386 -B i386 bin/u1.bin build/u1_bin.o
	# User program 2
	gcc $(FLAGS) -c src/u2.c -o build/u2.o
	ld -m elf_i386 -T src/user.ld -Ttext 0x500000 --oformat binary build/u2.o -o bin/u2.bin
	objcopy -I binary -O elf32-i386 -B i386 bin/u2.bin build/u2_bin.o

run: all
	qemu-system-x86_64 -serial stdio -drive format=raw,file=./bin/os.bin

clean:
	rm -f ./bin/*.bin
	rm -f ./build/*.o
