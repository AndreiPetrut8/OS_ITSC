void _start() {
    char *msg = "Hello from prog2\n";
    int len = 18;

    asm volatile("int $0x80" : : "a"(1), "b"(1), "c"(msg), "d"(len));

    unsigned int start, now;
    asm volatile("int $0x80" : "=a"(start) : "a"(3));

    do {
        asm volatile("int $0x80" : "=a"(now) : "a"(3));
    } while ((now - start) < 5000);

    asm volatile("int $0x80" : : "a"(5));
}
