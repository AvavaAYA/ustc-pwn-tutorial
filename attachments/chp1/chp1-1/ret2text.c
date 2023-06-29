// compiled with: gcc -no-pie -fno-stack-protector ./ret2text.c -o ./ret2text
// author: @eastXueLian
// date:   2023-03-25

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void gift() {
    asm("pop %rax");
    system("/bin/sh");
}
int main() {
    setvbuf(stdout, 0, 2, 0);
    setvbuf(stdin, 0, 2, 0);
    char buf[0x20];
    puts("Give me your data: ");
    read(0, buf, 0x100);
    return 0;
}
