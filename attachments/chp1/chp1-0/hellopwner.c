// compiled with: gcc -static -no-pie ./hellopwner.c -o ./hellopwner
// author: @eastXueLian
// date:   2023-03-24

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/mman.h>

char input[0x100];
void (*cmdtb[2])();
char *buf;


int get_a_num() {
    gets(input);
    return atoi(input);
}
void intro() {
    puts("=== Pwner's notebook ===");
    puts("");
    puts("Command:");
    puts("    0 - Load note");
    puts("    1 - Save note");
    puts("");
}
void cmd_load(char *buffer, int off, int len) {
    puts("data loaded😋");
}
void cmd_save(char *buffer, int off, int len) {
    puts("data saved😋");
}
void run() {
    int offset;
    int cmd;
    int len;
    
    printf("cmd> ");
    cmd = get_a_num();
    printf("offset: ");
    offset = get_a_num();
    if ( cmd == 1 )
    {
        printf("data: ");
        gets(buf);
        len = (unsigned int)strlen(buf);
    }
    else
    {
        len = 0;
    }
    cmdtb[cmd](buf, offset, len);
}

int main() {
    int i;

    intro();
    setvbuf(stdout, 0, 2, 0);
    setvbuf(stdin, 0, 2, 0);
    cmdtb[0] = &cmd_load;
    cmdtb[1] = &cmd_save;
    buf = (char *)mmap((void*)0x4c5000, 0x1000, 3, 34, 0, 0);
    for ( i = 0; i < 0x10; ++i )
      run();
    return 0;
}

void flag1() {
    int fd = open("./flag1", 0, 0);
    puts("You've gained my recognition, here's a gift for U: ");
    sendfile(1, fd, 0, 64);
}
void func_gifts() {
    mprotect(0, 0, 0);
}
