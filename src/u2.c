void _start() {
    asm volatile("int $0x80" : : "a"(1), "b"("Hello from Program 2!\n"), "c"(23));
}
