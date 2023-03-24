// compiled with: gcc ./supertriangle.c -o ./supertriangle
// author: @eastXueLian
// date:   2023-03-24

#include <stdio.h>
#include <stdlib.h>

char buf[256];
int (*cmdtb[10])();
void readNote(int offset, int length);
void writeNote(int offset, char *data, int length);

void init() {

    setvbuf(stdout, 0, 2, 0);
    setvbuf(stdin, 0, 2, 0);

    cmdtb[0] = &readNote;
    cmdtb[1] = &writeNote;
}

void menu() {
    puts("==========supertriangle==============");
    puts("[0] read");
    puts("[1] write");
    puts("=====================================");
    puts("Give me your choice: ");
}

int main() {
    
    int choice;

    init();
    for (int i = 0; i < 16; i++) {
        menu();
    }
}
