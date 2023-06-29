// compiled with: gcc -static -no-pie -fno-stack-protector ./ret2text-rev-rev.c -o ./ret2text-rev-rev
// author: @eastXueLian
// date:   2023-03-25

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    setvbuf(stdout, 0, 2, 0);
    setvbuf(stdin, 0, 2, 0);
    char buf[0x20];
    puts("/bin/sh");
    puts("Give me your data: ");
    read(0, buf, 0x100);
    return 0;
}
