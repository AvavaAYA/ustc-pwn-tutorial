// compiled with: gcc -no-pie -fno-stack-protector ./libcHijack.c -o ./libcHijack
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
    puts("INPUT: ");
    gets(buf);
}
int main() {   
    init();
    vuln();

    return 0;
}
