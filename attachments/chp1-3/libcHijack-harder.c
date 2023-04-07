// compiled with: gcc -no-pie -fno-stack-protector ./libcHijack-harder.c -o ./libcHijack-harder
// author: @eastXueLian
// date:   2023-03-25

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void init() {
    setvbuf(stdout, 0, 2, 0);
    setvbuf(stdin, 0, 2, 0);
}
void vuln() {
    char buf[64];
    printf("gift: %p\n", buf);
    puts("INPUT: ");
    read(0, buf, 0x50);
}
int main() {   
    init();
    vuln();

    return 0;
}
