// compiled with:  gcc ./hellopwntools.c -o hellopwntools
// Author: @eastXueLian
// Date:   2023-03-20

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void gift() {
    system("/bin/sh");
}

int main() {
    char buf[16];
    int a, b, c;

    setvbuf(stdout, 0, 2, 0);
    setvbuf(stdin, 0, 2, 0);

    // TASK 1
    puts("Welcome to Magical Mystery Tour! Give me your magic number: ");
    fgets(buf, 16, stdin);
    if ( *(long *)buf != 0xdeadbeef) {
        printf("[-] What a shame! Your input is: %lx\n", *(long *)buf);
        exit(0);
    }

    // TASK 2
    puts("I'll give you a gift if you answer 10 questions.");
    srand((unsigned)time(NULL));
    a = rand();
    b = rand();
    for (int i = 0; i < 10; i++) {
        printf("\n[Q%d] %d + %d = ", i+1, a, b);
        scanf("%d", &c);
        if (a + b != c) {
            puts("\n[-] WRONG!");
            exit(0);
        }
    }

    puts("\033[1;36;4m[+] Congratulations! Here's your gift: \033[0m");
    gift();

    return 0;
}
