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