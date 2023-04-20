// compiled with: gcc -no-pie -fno-stack-protector ./libcHijack-rev.c -o ./libcHijack-rev -lseccomp
// author: @eastXueLian
// date:   2023-03-25

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <seccomp.h>
#include <linux/seccomp.h>

void init() {
    setvbuf(stdout, 0, 2, 0);
    setvbuf(stdin, 0, 2, 0);

	scmp_filter_ctx ctx;
	ctx = seccomp_init(SCMP_ACT_ALLOW);
	seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(execve), 0);
	seccomp_load(ctx);
}
void vuln() {
    char buf[64];
    puts("flag is at:");
    puts("/flag");
    puts("INPUT: ");
    gets(buf);
}
int main() {   
    init();
    vuln();

    return 0;
}
