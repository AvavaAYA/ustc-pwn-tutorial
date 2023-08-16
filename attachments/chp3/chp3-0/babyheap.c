// compiled with: gcc ./babyheap.c -o ./babyheap -lseccomp
// author: @eastXueLian
// date:   2023-03-26

#include <elf.h>
#include <linux/seccomp.h>
#include <seccomp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PAGECOUNT 0x10

char *notebook[PAGECOUNT];
int sizeList[PAGECOUNT];

void add_note() {
  int i, size;
  for (i = 0; i < PAGECOUNT; i++) {
    if (!notebook[i]) {
      puts("[*] Note size: ");
      scanf("%d", &size);
      if (size > 0x1000) {
        exit(0);
      }
      notebook[i] = (char *)malloc((char)size);
      sizeList[i] = size;
      puts("[*] Note content: ");
      read(0, notebook[i], size);
      return;
    }
  }
  puts("[-] Notebook is full");
}
void delete_note() {
  int idx;
  puts("[*] Note index: ");
  scanf("%d", &idx);
  if (idx < 0 || idx > PAGECOUNT || !notebook[idx]) {
    exit(0);
  }
  free(notebook[idx]);
  notebook[idx] = 0;
}
void edit_note() {
  int idx;
  puts("[*] Note index: ");
  scanf("%d", &idx);
  if (idx < 0 || idx > PAGECOUNT || !notebook[idx]) {
    exit(0);
  }
  puts("[*] Note content: ");
  read(0, notebook[idx], sizeList[idx]);
}
void show_note() {
  int idx;
  puts("[*] Note index: ");
  scanf("%d", &idx);
  if (idx < 0 || idx > PAGECOUNT || !notebook[idx]) {
    exit(0);
  }
  write(1, notebook[idx], sizeList[idx]);
}
void inits() {
  setvbuf(stdout, 0, 2, 0);
  setvbuf(stdin, 0, 2, 0);
  setvbuf(stderr, 0, 2, 0);

  // scmp_filter_ctx ctx;
  // ctx = seccomp_init(SCMP_ACT_ALLOW);
  // seccomp_rule_add(ctx, SCMP_ACT_KILL, SCMP_SYS(execve), 0);
  // seccomp_load(ctx);
}
void banner() {
  puts(" _           _           _                      ");
  puts("| |__   __ _| |__  _   _| |__   ___  __ _ _ __  ");
  puts("| '_ \\ / _` | '_ \\| | | | '_ \\ / _ \\/ _` | '_ \\ ");
  puts("| |_) | (_| | |_) | |_| | | | |  __/ (_| | |_) |");
  puts("|_.__/ \\__,_|_.__/ \\__, |_| |_|\\___|\\__,_| .__/ ");
  puts("                   |___/                 |_|    ");
}
void menu() {
  puts("\t1 : add note");
  puts("\t2 : delete note");
  puts("\t3 : edit note");
  puts("\t4 : show note");
  puts("Your choice: ");
}

int main() {
  unsigned int choice;
  inits();
  banner();
  printf("printf: %p\n", (void *)printf);
  while (1) {
    menu();
    scanf("%d", &choice);
    switch (choice) {
    case 1:
      add_note();
      break;
    case 2:
      delete_note();
      break;
    case 3:
      edit_note();
      break;
    case 4:
      show_note();
      break;
    default:
      exit(0);
    }
  }
  return 0;
}
