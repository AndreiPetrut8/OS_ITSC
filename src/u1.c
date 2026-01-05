void _start() {
    asm volatile("int $0x80" : : "a"(1), "b"("Hello from Program 1!\n"), "c"(23));
}
