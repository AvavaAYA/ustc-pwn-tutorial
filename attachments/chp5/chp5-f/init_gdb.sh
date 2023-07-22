#!/usr/bin/sh

gdb-multiarch ./vmlinux.bin -ex "set architecture i386:x86_64" \
    -ex "target remote localhost:1234" \
    -ex "add-symbol-file ./d3kheap.ko $1" \
    -ex "b *($1+0x18)" \
    -ex "b *($1+0x6d)" \
    -ex "c"
