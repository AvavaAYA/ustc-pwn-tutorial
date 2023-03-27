// compiled with: gcc -no-pie ./hellopwner.c -o ./hellopwner
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
    puts("=== Trusted Keystore ===");
    puts("");
    puts("Command:");
    puts("    0 - Load key");
    puts("    1 - Save key");
    puts("");
}
void cmd_load(char *buffer, int off, int len) {
    write(1, buffer+off, len);
}
void cmd_save(char *buffer, int off, int len) {
    puts("data saved😋");
}
void run() {
    int idx;
    int cmd;
    int len;
    
    printf("cmd> ");
    cmd = get_a_num();
    printf("index: ");
    idx = get_a_num();
    if ( cmd == 1 )
    {
        printf("key: ");
        scanf("%s", buf);
        len = (unsigned int)strlen(buf);
    }
    else
    {
        len = 0;
    }
    cmdtb[cmd](buf, idx, len);
}

int main() {
    int i;
    
    intro();
    setvbuf(stdout, 0, 2, 0);
    setvbuf(stdin, 0, 2, 0);
    cmdtb[0] = &cmd_load;
    cmdtb[1] = &cmd_save;
    buf = (char *)mmap(0, 0x1000, 3, 34, 0, 0);
    for ( i = 0; i < 0x10; ++i )
      run();
    return 0;
}
