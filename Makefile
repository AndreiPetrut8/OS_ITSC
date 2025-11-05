FILES = ./build/kernel.asm.o ./build/kernel.o
FLAGS = -g -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc

all:
	nasm -f bin ./src/boot.asm -o ./bin/boot.bin
	nasm -f elf64 -g ./src/kernel.asm -o ./build/kernel.asm.o
	gcc -I./src $(FLAGS) -std=gnu99 -c ./src/kernel.c -o ./build/kernel.o
	ld -g -relocatable $(FILES) -o ./build/completeKernel.o
	gcc $(FLAGS) -T ./src/linkerScript.ld -o ./bin/kernel.bin -ffreestanding -O0 -nostdlib ./build/completeKernel.o

	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=512 count=8 >> ./bin/os.bin

run:
	qemu-system-x86_64 -drive format=raw,file=./bin/os.bin

clean:
	rm -f ./bin/boot.bin
	rm -f ./bin/kernel.bin
	rm -f ./bin/os.bin
	rm -f ./build/kernel.asm.o
	rm -f ./build/kernel.o
	rm -f ./build/completeKernel.o
