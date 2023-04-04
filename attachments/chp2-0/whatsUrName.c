// compiled with: gcc ./whatsUrName.c -o ./whatsUrName
// author: @eastXueLian
// date:   2023-03-26

#include <stdio.h>
#include <string.h>

void vuln() {
    char name[0x100];

    puts("What's your name?");
    fgets(name, 0x100, stdin);
    if ( strchr(name, '%') ) {
        printf(name);
        puts("? Why is your name so strange? I want your real name!!");
        fgets(name, 0x100, stdin);
    }
    puts("Well, hello ");
    printf(name);
}

int main() {
    setvbuf(stdout, 0, 2, 0);
    setvbuf(stdin, 0, 2, 0);
    vuln();
    return 0;
}
