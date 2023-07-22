#!/bin/sh

cargo build --release --target=x86_64-unknown-linux-musl
cp ./target/x86_64-unknown-linux-musl/release/exp ../core/exp
cd ../core
find . -print0 | cpio --null -ov --format=newc >../core.cpio
cd ..
./start.sh
