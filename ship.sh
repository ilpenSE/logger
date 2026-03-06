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
rm -rf logs app app.exe

# C++
cd $proot/usages/c++
rm -rf logs app app.exe time.txt test.py

# Python
cd $proot/usages/python
rm -rf logs __pycache__

# Rust
cd $proot/usages/rust
rm -rf logs Cargo.lock target tests

# Generate zips
cd $proot/usages
tar -czf ../usages.tar.gz -C .. usages/
zip ../usages.zip -r ./

# Clear
rm -rf $proot/usages
