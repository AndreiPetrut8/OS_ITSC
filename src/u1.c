void _start() {
    char *msg = "Hello from u1 through the pipeline!\n";
    int len = 38;

    asm volatile("int $0x80" : : "a"(1), "b"(1), "c"(msg), "d"(len));

    asm volatile("int $0x80" : : "a"(5));

}
