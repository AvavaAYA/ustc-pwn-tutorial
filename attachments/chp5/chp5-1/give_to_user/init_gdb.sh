#!/usr/bin/sh

gdb-multiarch ./vmlinux.bin -ex "set architecture i386:x86_64" \
    -ex "target remote localhost:1234" \
    -ex "add-symbol-file ./rootfs/sudrv.ko $1" \
    -ex "b *(sudrv_write+0x4f)" \
    -ex "b *sudrv_write" \
    -ex "c"
