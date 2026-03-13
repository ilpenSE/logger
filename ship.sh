#!/bin/bash

proot=$(pwd)

echo "Project root = $proot"

# Generate headers zip
headers_zip="c-c++_headers"
tar -czf $headers_zip.tar.gz logger.h loggerstream.hpp
zip $headers_zip.zip logger.h loggerstream.hpp

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
tar -czf ../usages.tar.gz -C .. usages/
zip ../usages.zip -r ./

# Clear
rm -rf $proot/usages
