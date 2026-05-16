void _start() {
    char buf[64];
    int n;

    asm volatile("int $0x80" : "=a"(n) : "a"(6), "b"(0), "c"(buf), "d"(64));

    if (n > 0) {
        asm volatile("int $0x80" : : "a"(1), "b"(1), "c"(buf), "d"(n));
    }
    else if (n < 0) {
        asm volatile("int $0x80" : : "a"(5));
    }
}
