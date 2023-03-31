// compiled with: gcc -static -no-pie -fno-stack-protector ./ret2text-rev-rev-rev.c -o ./ret2text-rev-rev-rev -lseccomp
// author: @eastXueLian
// date:   2023-03-25

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <seccomp.h>
#include <linux/seccomp.h>

int main() {
    char buf[0x20];

	scmp_filter_ctx ctx;
	ctx = seccomp_init(SCMP_ACT_ALLOW);
	seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(execve), 0);
	seccomp_load(ctx);

    puts("/bin/sh");
    puts("flag is at: ");
    puts("/flag");
    puts("Give me your data: ");
    read(0, buf, 0x100);
    return 0;
}
