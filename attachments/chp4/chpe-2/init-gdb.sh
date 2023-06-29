#!/bin/sh
clang-8 -emit-llvm -S exp.c -o exp.ll
gdb ./opt -ex "b *main" \
    -ex "set args -load ./SAPass.so -SAPass ./exp.ll" \
    -ex "r" \
    -ex "b *0x4b8d65" \
    -ex "c" \
    -ex "vmmap" \
    -ex "set \$base=0x7ffff3b5c000" \
    -ex "b *(\$base+0x1CFD)"
