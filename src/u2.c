void _start() {
  asm volatile("int $0x80" : : "a"(1), "b"(2), "c"("Hello from Program 2!\n"), "d"(23));
  return;
}
