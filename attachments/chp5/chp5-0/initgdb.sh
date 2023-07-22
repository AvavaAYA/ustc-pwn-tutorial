#!/usr/bin/sh

gdb-multiarch ./vmlinux -ex "set architecture i386:x86_64" \
	-ex "target remote localhost:1234" \
	-ex "add-symbol-file ./core/core.ko $1" \
	-ex "b *(core_release+0x12A)" \
	-ex "c"
