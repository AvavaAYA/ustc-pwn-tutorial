# 1.2.3 libc 和 patchelf 以及 glibc-all-in-one

对于动态链接的可执行文件，在执行时会调用动态链接库，在 windows 下是 `.dll`，而在 linux 下就是 `.so` (即 shared object) 文件。

## ldd

而 glibc 就是 GUN 发布的 libc 库，是 linux 系统中最底层的动态链接库，读者可以通过 `ldd` 命令来看自己的 glibc 版本：

```sh
❯ ldd --version
ldd (Ubuntu GLIBC 2.31-0ubuntu9.9) 2.31
Copyright (C) 2020 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
Written by Roland McGrath and Ulrich Drepper.
```

ldd 也可以查看动态链接的 elf 程序对于 glibc 的调用情况：

```sh
❯ ldd ./hellopwntools
        linux-vdso.so.1 (0x00007ffc1a0bc000)
        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f903ae2f000)
        /lib64/ld-linux-x86-64.so.2 (0x00007f903b046000)
```

---

## glibc-all-in-one

`glibc-all-in-one` 提供了一系列不同版本且附带调试信息的 glibc 库，直接从 github 上 clone 下来即可，仓库地址为：[https://github.com/matrix1001/glibc-all-in-one](https://github.com/matrix1001/glibc-all-in-one) .

---

## patchelf

在 ubuntu 中 `patchelf` 可以直接通过 `apt install patchelf` 进行安装。

通常使用下面两条命令修改程序的 libc 库，使其与远程环境一致：

```sh
patchelf --replace-needed libc.so.6 <target libc.so path> <elf name>
patchelf --set-interpreter <target ld path> <elf name>
```

