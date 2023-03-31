# 1.4.1 格式化字符串漏洞

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

对于 `%n` 而言，同样有 `%k$n` 的用法，指将在这个之前输出了的字符长度写入到第k个参数所对应的地址中。

回到上面的例子，第7个参数中的内容虽然是 12345678，但是同样可以被作为地址而解析，如果将其改为 `variable` 变量的地址，就可以修改 `variable` 变量了

首先可以找到 `variable` 的地址，由于没有pie，简化了获得地址的操作。在本次的例子中为 `0x804c024` 

```python
variable_addr = 0x804c024
p.sendline(p32(variable_addr) + b'a' * 4 + b'%7$n')
```

在图中，可以看到，esp指向的是格式化字符串的地址，而格式化字符串的地址 `0xffa5fa8c` 的前4个字节被作为了 `variable` 变量的地址，加上前面计算出，`0xffa5fa8c` 是printf的格式化字符串中第7个参数，因此 `%7$n` 会将在此之前打印出的8个字符（包括4个字节的地址与4个 `a` ）的个数，即8，写入到`0xffa5fa8c` 中。此时 `0x804c024` 还是 0xa，即10

在printf结束后，查看 `0x804c024` 的内容，变成了8

上面是一种利用方法，将addr放在了格式化字符串的最开头，但是这会有一些问题，在64位下，因为地址往往会包含 `0x00` 字节，如果地址放在最开头，会导致printf在打印时，打印到 `0x00` 时就认为是字符串结尾，停止打印。但是因为 `0x00` 往往出现在最高位上，而 `x86` 是小端序，因此如果将地址放在输入的最后，那么 `0x00` 将不会影响printf的打印

这里还是以32位来作为例子

先提供给程序这样的输入

```python
p.sendline('0123456789abcdef' * 4)
```

查看字符串的存储情况

例如说，如果我们希望能够把地址放在 `0xffb486ec` 的地方，也就是 `$eax+0x10` 处，那么对应的格式化字符串的地址的参数就是第 `0xffb486ec - 0xffb486c0 = 11` 个。`0xffb486ec`相对于字符串的偏移是 `0xffb486ec - 0xffb486dc = 16` ，也就是需要前面有16个字符。

这个时候可以构造这样的程序输入。在 `%11$n` 之前打印了4个字符，因此 `variable` 变量会被修改为4。这里使用了 `ljust` 来将字符串补齐为16的长度

```python
payload = b'aaaa%11$n'.ljust(16, b'a')
payload += p32(variable_addr)

p.sendline(payload)
```

查看被修改后的 `variable` 变量

练习：

如何在64位上进行如此的字符串格式化攻击，需要注意64位的前6个参数是通过寄存器传递的

如何写入任意大小的数字？如 `0x12345` 

- 需要注意的是，将一串很长很长输入传递给程序是不可行的。
- 可以通过 `%width c` 来实现（width和c之前没有空格，width指需要将输入的字符进行对其的宽度）

