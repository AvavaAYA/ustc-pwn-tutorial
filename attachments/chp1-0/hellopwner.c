// compiled with: gcc -no-pie ./hellopwner.c -o ./hellopwner
// author: @eastXueLian
// date:   2023-03-24

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sendfile.h>

char buf[256];
void (*cmdtb[10])();
char note_page[256];

int get_a_num(char *buffer) {
    fgets(buffer, 0x100, stdin);
    return atoi(buffer);
}
void readNote(int offset, char *buffer, int length) {
    write(1, buffer+offset, length);
}
void writeNote(int offset, char *buffer, int length) {
    read(0, buffer+offset, length);
}
void init() {
    setvbuf(stdout, 0, 2, 0);
    setvbuf(stdin, 0, 2, 0);
    cmdtb[0] = &readNote;
    cmdtb[1] = &writeNote;
}
void menu() {
    puts("==========pwner's=notebook============");
    puts("[0] read");
    puts("[1] write");
    puts("======================================");
    puts("Give me your choice:");
}
int main() {
    int choice, offs, leng;
    init();
    for (int i = 0; i < 16; i++) {
        menu();
        choice = get_a_num(buf);
        if (choice > 1) {
            puts("HACKER!!");
            exit(0);
        }
        puts("Where'd you love to start?");
        offs = get_a_num(buf);
        if (choice == 1) {
            puts("How much do you want to write?");
            leng = get_a_num(buf);
            if (leng+offs > 0x100) {
                puts("HACKER!!!!");
                exit(0);
            }
        }
        else {
            leng = strlen(note_page+offs);
        }
        cmdtb[choice](offs, note_page, leng);
    }
}

void pwners_gift() {
    int fd = open("./flag1", 0, 0);
    puts("You've gained my recognition, here's a gift for U: ");
    sendfile(1, fd, 0, 64);
}
