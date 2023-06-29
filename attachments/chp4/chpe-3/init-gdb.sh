#!/bin/sh
clang-8 -emit-llvm -S exp.c -o exp.ll
gdb ./opt-8 -ex "b *main" \
    -ex "set args -load ./VMPass.so -VMPass ./exp.ll" \
    -ex "r" \
    -ex "b *0x4b8d65" \
    -ex "c" \
    -ex "vmmap"
