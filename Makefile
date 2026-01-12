FILES = ./build/kernel.asm.o ./build/kernel.o ./build/heap.o ./build/idt.o ./build/uart.o ./build/u1_bin.o ./build/u2_bin.o
FLAGS = -g -ffreestanding -m32 -fno-pic -fno-pie -fno-stack-protector -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc


all: user_progs
	nasm -f bin ./src/boot.asm -o ./bin/boot.bin
	nasm -f elf32 -g ./src/kernel.asm -o ./build/kernel.asm.o
	gcc -I./src $(FLAGS) -std=gnu99 -c ./src/kernel.c -o ./build/kernel.o
	gcc -I./src $(FLAGS) -std=gnu99 -c ./src/heap.c -o ./build/heap.o
	gcc -I./src $(FLAGS) -std=gnu99 -c ./src/idt.c -o ./build/idt.o
	gcc -I./src $(FLAGS) -std=gnu99 -c ./src/uart.c -o ./build/uart.o
	ld -m elf_i386 -g -relocatable $(FILES) -o ./build/completeKernel.o
	gcc $(FLAGS) -T ./src/linkerScript.ld -o ./bin/kernel.bin -ffreestanding -O0 -nostdlib ./build/completeKernel.o

	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=512 count=100 >> ./bin/os.bin

run: all
	qemu-system-x86_64 -serial stdio -drive format=raw,file=./bin/os.bin

user_progs:
	gcc $(FLAGS) -c src/u1.c -o build/u1.o
	ld -m elf_i386 -T src/user.ld -Ttext 0x400000 --oformat binary build/u1.o -o bin/u1.bin
	objcopy -I binary -O elf32-i386 -B i386 bin/u1.bin build/u1_bin.o

	gcc $(FLAGS) -c src/u2.c -o build/u2.o
	ld -m elf_i386 -T src/user.ld -Ttext 0x500000 --oformat binary build/u2.o -o bin/u2.bin
	objcopy -I binary -O elf32-i386 -B i386 bin/u2.bin build/u2_bin.o


clean:
	rm -f -r ./bin/*.bin
	rm -f -r ./build/*.o
