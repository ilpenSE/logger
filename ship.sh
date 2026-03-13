#!/bin/bash

proot=$(pwd)

echo "Project root = $proot"

mkdir artifacts

# Generate headers zip
headers_zip="c-c++_headers"
tar -czf artifacts/$headers_zip.tar.gz logger.h loggerstream.hpp
zip artifacts/$headers_zip.zip logger.h loggerstream.hpp

WINDOWS_BINARIES="logger.h logger.lib logger.exp logger.dll logger.obj"
LINUX_BINARIES="liblogger.so liblogger.a logger.o logger.h"

# Generate x86_64 binaries
make clean
make -B
cd $proot/build
tar -czf ../artifacts/x86_64-gnu-linux.tar.gz $LINUX_BINARIES
cd $proot
nmake /a /f Makefile.win
cd $proot/build
zip ../artifacts/x86_64-windows.zip $WINDOWS_BINARIES
cd $proot

# Generate AARCH64 Binaries
make clean
make -B arch=aarch64
cd $proot/build
tar -czf ../artifacts/aarch64-gnu-linux.tar.gz $LINUX_BINARIES
cd $proot
# Fuck windows aarch64

# Generate usages folder
cp -r usage usages

# Remove deprecated
rm -rf usages/deprecated

# C
cd $proot/usages/c
rm -rf build logs app app.exe *.log

# C89
cd $proot/usages/c89
rm -rf build logs app app.exe *.log

# C++
cd $proot/usages/c++
rm -rf logs build app app.exe time.txt test.py *.log

# Python
cd $proot/usages/python
rm -rf logs __pycache__ *.log

# Rust
cd $proot/usages/rust
rm -rf logs Cargo.lock target tests *.log

# Generate zips
cd $proot/usages
tar -czf ../artifacts/usages.tar.gz -C .. usages/
zip ../artifacts/usages.zip -r ./

# Clear
rm -rf $proot/usages

cd $proot

hash=$(git rev-parse HEAD)
commit=$(git --no-pager log -1 --pretty=%s)

echo "Commit message:"
echo "========================"
echo "$commit"
echo ""
echo "See this commit: https://github.com/ilpenSE/logger/commit/$hash"
echo "========================"
