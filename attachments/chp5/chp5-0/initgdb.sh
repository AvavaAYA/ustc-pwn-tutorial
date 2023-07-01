#!/usr/bin/sh

gdb-multiarch ./vmlinux.bin -ex "set architecture i386:x86_64" \
	-ex "target remote localhost:1234" \
	-ex "add-symbol-file ./core/core.ko $1" \
	-ex "c"
