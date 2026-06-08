# Instalare tool-uri necesare:
# sudo apt install build-essential gcc-multilib nasm binutils
# sudo apt install qemu-system-x86
#
# Lansare Sistem: make run
#
# Ștergere fișiere "*.o" și "*.bin": make clean
#
# Pașii de urmat pentru adăugarea unui program extern
#​ 1.Compilezi programul user ca binar separat.
#​ 2.Îl convertești într-un obiect care conține datele binare (objcopy).
#​ 3.Linkezi acel obiect împreună cu codul Kernel-ului.
#​ 4.Accesezi datele folosind pointerii generați automat de Linker (_start și _end).

## Makefile Configuration (Linux vs. macOS)

This project uses a `Makefile` configured natively for Linux. Depending on your operating system, please follow the instructions below:

### 🐧 Linux Users
If you are running Linux, no changes are needed. The Makefile directly uses the native toolchain:
* `gcc`
* `ld`
* `objcopy`

---

### 🍏 macOS Users
Since macOS does not natively support the ELF toolchain, you will need to use a cross-compiler (e.g., installed via `brew install i686-elf-gcc`).

Modify your `Makefile` as follows:

1. **Add these lines at the very beginning of the file:**
   ```makefile
   CROSS = i686-elf

   CC      = $(CROSS)-gcc
   LD      = $(CROSS)-ld
   OBJCOPY = $(CROSS)-objcopy

2. **Replace the native commands throughout the file:**
Update the compilation rules by replacing the standard commands with the newly defined variables:
    Change `gcc` to `$(CC)`
    Change `ld` to `$(LD)`
    Change `objcopy` to `$(OBJCOPY)`
