void _start1() {
    while(1) {
      asm volatile("int $0x80" : : "a"(1), "b"("Hello from Program 2!\n"), "c"(22));
        for(volatile int i = 0; i < 1000000; i++);
    }
}

