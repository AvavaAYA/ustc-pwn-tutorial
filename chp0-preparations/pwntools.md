# 1.2.1 pwntools 和 pwncli

>   对于环境配置，网上已经有大量的资料，不过这里还是再简单介绍一下软件的安装及使用方法， 同时也会给出笔者的一些[板子](https://github.com/AvavaAYA/pwn-scripts)，给读者后续做题带来一些便利。

这里也给出一些相关链接：

-   官方文档：[http://docs.pwntools.com/](http://docs.pwntools.com/)
-   github 仓库：[https://github.com/Gallopsled/pwntools](https://github.com/Gallopsled/pwntools#readme)
-   官方教学：[https://github.com/Gallopsled/pwntools-tutorial#readme](https://github.com/Gallopsled/pwntools-tutorial#readme)
-   pwncli：[https://github.com/RoderickChan/pwncli](https://github.com/RoderickChan/pwncli)

## pwntools 安装

而 pwntools 的安装也相当简单，直接 pip install 即可：

```sh
apt-get update
apt-get install python3 python3-pip python3-dev git libssl-dev libffi-dev build-essential
python3 -m pip install --upgrade pip
python3 -m pip install --upgrade pwntools
```

---

## pwntools 使用

pwntools 是对 socket 交互和一些辅助函数的封装，下面借助例题介绍 pwntools 一些常见的用法：

### 例题 chp0-0-hellopwntools

由于这是 pwn 部分的第一道例题，所以直接给出例题源码（这些内容在附件中也已提供，读者也可以将 elf 文件拖进 ida 中进行分析以熟悉 ida 的用法），读者可以关注一下这里额外加上去的注释便于理解：

```c
// compiled with:  gcc ./hellopwntools.c -o hellopwntools
// Author: @eastXueLian
// Date:   2023-03-20

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void gift() {
    // 这里是题目中直接提供的后门函数，效果是获得一个交互式的 shell
    // 随着后面深入，读者会了解到程序没提供后门函数时怎么完成利用
    system("/bin/sh");
}

int main() {
    char buf[16];
    int a, b, c;

    // 程序需要输出输入时，加上这两段代码，不然部署在 docker 中运行时要回车才有输出
    setvbuf(stdout, 0, 2, 0);
    setvbuf(stdin, 0, 2, 0);

    // TASK 1
    puts("Welcome to Magical Mystery Tour! Give me your magic number: ");
    // 利用 fgets 从标准输入中读 16 个字符，用 \n 截断（\x00 不会截断输入）
    fgets(buf, 16, stdin);
    if ( *(long *)buf != 0xdeadbeef) {
    // buf 被强转为 (long*) 类型，那读者可以考虑一下我们该用什么样的输入来通过这里
        printf("[-] What a shame! Your input is: %lx\n", *(long *)buf);
        exit(0);
    }

    // TASK 2
    // 这里主要还是用于熟悉 pwntools 的使用
    puts("I'll give you a gift if you answer 10 questions.");
    srand((unsigned)time(NULL));
    a = rand();
    b = rand();
    for (int i = 0; i < 10; i++) {
        printf("\n[Q%d] %d + %d = ", i+1, a, b);
        scanf("%d", &c);
        if (a + b != c) {
            puts("\n[-] WRONG!");
            exit(0);
        }
    }

    puts("[+] Congratulations! Here's your gift: ");
    gift();

    return 0;
}
```

这道题实际上并没有什么明显的漏洞，放在这里是想让读者熟悉一下 pwntools 的使用，下面给出 exp.py（即利用脚本），每一句后面都有解释，读者可以借此了解 pwntools 的基础使用：

```python
#!/usr/bin/python3
#-*- coding: utf-8 -*-
#  author:  @eastXueLian
#  date:    2023-03-20

#  使用前先导入 pwn 的库
from pwn import *
#  调整 log_level，如果嫌输出太多太乱可以注释掉或者把 debug 改成 error
context.log_level = "debug"

#  本地调试，若远程则改为 remote("xx.xx.xx.xx", xxxx)
p = process("./hellopwntools")

#  断下来便于调试，这里可以将 gdb attach 上去
input()
#  控制 io 输入输出
p.recvuntil(b'Welcome to Magical Mystery Tour! Give me your magic number: \n')
#  这是第一处的答案，p64 用于产生符合字节序的输入，这里可以结合调试看一下内存中的数据到底长什么样
p.sendline(p64(0xdeadbeef))

for i in range(10):
    p.recvuntil(b"] ")
    #  drop=True，即丢弃截断的内容，进入 int 函数
    num1 = int(p.recvuntil(b" + ", drop=True))
    num2 = int(p.recvuntil(b" = ", drop=True))
    #  可以注意到这里要加上 .encode()
    p.sendline(str(num1+num2).encode())

#  io 控制权交还给用户
p.interactive()
```

---

### 模板

经过上面的简单示例，读者不难发现很多东西是可以简化的，下面是笔者接触 pwncli 前常用的做题模板，给程序交互提供了便利。

```python
#!/usr/bin/python3
#-*- coding: utf-8 -*-
#  author: @eastXueLian

from pwn import *

context.log_level = 'debug'
context.arch='amd64'
context.terminal = ['tmux','sp','-h','-l','120']

LOCAL = 1

filename = "./pwn"
if LOCAL:
    p = process(filename)
else:
    remote_service = ""
    remote_service = remote_service.strip().split(":")
    p = remote(remote_service[0], int(remote_service[1]))
e = ELF(filename, checksec=False)
l = ELF(e.libc.path, checksec=False)


rl = lambda a=False : p.recvline(a)
ru = lambda a,b=True : p.recvuntil(a,b)
rn = lambda x : p.recvn(x)
sn = lambda x : p.send(x)
sl = lambda x : p.sendline(x)
sa = lambda a,b : p.sendafter(a,b)
sla = lambda a,b : p.sendlineafter(a,b)
irt = lambda : p.interactive()
dbg = lambda text=None : gdb.attach(p, text)
lg = lambda s : log.info('\033[1;31;40m %s --> 0x%x \033[0m' % (s, eval(s)))
i2b = lambda c : str(c).encode()
uu32 = lambda data : u32(data.ljust(4, b'\x00'))
uu64 = lambda data : u64(data.ljust(8, b'\x00'))
def debugPID():
    if LOCAL:
        lg("p.pid")
        input()
    pass

debugPID()


irt()
```

---

## pwncli

笔者认为这里有必要提一下 pwncli，具体可以参考 [https://github.com/RoderickChan/pwncli](https://github.com/RoderickChan/pwncli)，是一个基于 click 和 pwntools 的一款简单、易用的 pwn 题调试与攻击工具，相比 pwntools 提供了更多函数、结构体与命令行参数。

