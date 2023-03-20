#!/usr/bin/python3
#-*- coding: utf-8 -*-
#  author:  @eastXueLian
#  date:    2023-03-20

from pwn import *
context.log_level = "debug"

p = process("./hellopwntools")

input()
p.recvuntil(b'Welcome to Magical Mystery Tour! Give me your magic number: \n')
p.sendline(p64(0xdeadbeef))

for i in range(10):
    p.recvuntil(b"] ")
    num1 = int(p.recvuntil(b" + ", drop=True))
    num2 = int(p.recvuntil(b" = ", drop=True))
    p.sendline(str(num1+num2).encode())

p.interactive()
