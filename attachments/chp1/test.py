from pwn import *

context.arch = "amd64"
a = open("test.asm").read()
s = asm(a)
print(s)
open("test.bin", "wb").write(s)
