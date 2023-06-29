#!/bin/sh
clang-10 -emit-llvm -S demo.c -o demo.ll
gdb opt-10 -ex "b *main" \
    -ex "set args -load ./test.so -bianYiYuanLiXiTiKe ./demo.ll" \
    -ex "r" \
    -ex "b *0x4bc8c8" \
    -ex "c" \
    -ex "set \$base=0x7ffff7fb5000" \
    -ex "b *(\$base+0x9625)" \
    -ex "set \$list=(\$base+0x13860)"
