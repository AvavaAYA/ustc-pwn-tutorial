// compiled with: gcc -no-pie -fno-stack-protector ./ret2text-rev.c -o ./ret2text-rev
// author: @eastXueLian
// date:   2023-03-25

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void gift() {
    system("/bin/sh");
}
int main() {
    char buf[0x20];
    puts("Give me your data: ");
    read(0, buf, 0x100);
    return 0;
}
