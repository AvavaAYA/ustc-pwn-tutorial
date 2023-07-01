#!/bin/sh

cargo build --release --target=x86_64-unknown-linux-musl
cp ./target/x86_64-unknown-linux-musl/release/exp ../rootfs/exp
cd ../rootfs
find . -print0 | cpio --null -ov --format=newc >../rootfs.cpio
cd ..
./start.sh
