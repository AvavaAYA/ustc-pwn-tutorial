#!/bin/sh

set -e

musl-gcc ./exp.c -static -masm=intel -o ./exp
cp ./exp ../core/exp
cd ../core
find . -print0 | cpio --null -ov --format=newc >../core.cpio
cd ..
./start.sh
