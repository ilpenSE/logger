#!/bin/bash

proot=$(pwd)
clr_green="\e[1;32m"
clr_red="\e[1;31m"
clr_gold="\e[1;38;2;255;165;0m"
clr_rst="\e[0m"
mkdir -p artifacts

generate_headers() {
  headers_zip="c-c++_headers"
  tar -czf artifacts/$headers_zip.tar.gz logger.h loggerstream.hpp > /dev/null 2>&1
  zip artifacts/$headers_zip.zip logger.h loggerstream.hpp > /dev/null 2>&1
}

verify_binary() {
  local file=$1
  local expected=$2
  local result=$(file "$file")
  if echo "$result" | grep -qi "$expected"; then
    echo -e "$file: $clr_green[OK]$clr_rst ($expected)"
  else
    echo -e "$file: $clr_red[FAILED]$clr_rst Expected: $expected"
    echo "  Real: $result"
    exit 1
  fi
}

build() {
  local os=$1
  local arch=$2
  cd $proot

  if [ "$os" = "msvc" ]; then
    echo "==> Building MSVC x86_64"
    nmake /a /f Makefile.win > /dev/null 2>&1
    cd $proot/build
    verify_binary logger.dll "x86-64"
    zip ../artifacts/x86_64-windows.zip logger.h logger.lib logger.exp logger.dll logger.obj > /dev/null 2>&1
    return
  fi

  echo "==> Building $os $arch"
  make -B os=$os arch=$arch > /dev/null 2>&1
  cd $proot/build

  local artifact_name="${arch}-${os}"
  case "$os" in
    linux)
      verify_binary liblogger.so "$( [ "$arch" = "aarch64" ] && echo "aarch64" || echo "x86-64" )"
      tar -czf ../artifacts/${artifact_name}.tar.gz liblogger.so liblogger.a logger.o logger.h > /dev/null 2>&1
      ;;
    mingw)
      verify_binary liblogger.dll "$( [ "$arch" = "aarch64" ] && echo "aarch64" || echo "x86-64" )"
      zip ../artifacts/${artifact_name}.zip liblogger.dll liblogger.a logger.o logger.h > /dev/null 2>&1
      ;;
    osx)
      verify_binary liblogger.dylib "$( [ "$arch" = "aarch64" ] && echo "arm64" || echo "x86_64" )"
      tar -czf ../artifacts/${artifact_name}.tar.gz liblogger.dylib liblogger.a logger.o logger.h > /dev/null 2>&1
      ;;
  esac
}

generate_binaries() {
  # x86_64
  build linux  x86_64
  build msvc   x86_64
  build mingw  x86_64
  build osx    x86_64

  # aarch64
  build linux  aarch64
  build osx    aarch64
  build mingw  aarch64

  cd $proot
  make clean > /dev/null 2>&1
}

generate_usages() {
  cp -r usage usages
  rm -rf usages/deprecated
  rm -rf usages/go/build   usages/go/logs   usages/go/app  usages/go/app.exe  usages/go/*.log
  rm -rf usages/c/build    usages/c/logs    usages/c/app   usages/c/app.exe   usages/c/*.log
  rm -rf usages/c89/build  usages/c89/logs  usages/c89/app usages/c89/app.exe usages/c89/*.log
  rm -rf usages/c++/logs   usages/c++/build usages/c++/app usages/c++/app.exe usages/c++/time.txt usages/c++/test.py usages/c++/*.log
  rm -rf usages/python/logs  usages/python/__pycache__  usages/python/*.log
  rm -rf usages/rust/logs    usages/rust/Cargo.lock     usages/rust/target usages/rust/tests usages/rust/*.log
  cd $proot/usages
  tar -czf ../artifacts/usages.tar.gz -C ../usages . > /dev/null 2>&1
  zip ../artifacts/usages.zip -r ./ > /dev/null 2>&1
  cd $proot
  rm -rf usages
}

generate_release_message() {
  is_verbose=$1

  hash=$(git rev-parse HEAD)
  full_msg=$(git --no-pager log -1 --pretty=%B)
  subject=$(git --no-pager log -1 --pretty=%s | sed 's/^v[0-9][0-9.]*: //')
  breaking=$(echo "$full_msg" | awk '/^BREAKING/{found=1; next} found && /^[A-Z]/{exit} found{print}')
  tests=$(echo "$full_msg" | awk '/^TESTS/{found=1; next} found{print}')

  if [[ "$is_verbose" -eq 1 ]]; then
    echo "Generated release message:"
    echo "========================"
  fi
  echo "$subject"
  echo ""
  if [ -n "$breaking" ]; then
    echo "# BREAKING"
    echo "$breaking" | while IFS= read -r line; do echo "$line"; done
    echo ""
  fi
  if [ -n "$tests" ]; then
    echo "# Tests"
    echo "$tests" | while IFS= read -r line; do echo "$line"; done
    echo ""
  fi
  echo "See this commit: https://github.com/ilpenSE/logger/commit/$hash"
  if [[ "$is_verbose" -eq 1 ]]; then
    echo "========================"
  fi
}

op=$1

case $op in
  binary | binaries)
    generate_binaries ;;
  release | release_message | message)
    generate_release_message 0 ;;
  headers | header)
    generate_headers ;;
  usages | usage)
    generate_usages ;;
  all)
    generate_binaries &&
      generate_headers &&
      generate_usages &&
      generate_release_message 1 ;;
  *)
    echo -e "$clr_red[ERROR]$clr_rst Unkown option.";
    echo -e "Usage:";
    echo -e "  ${clr_gold}all$clr_rst: Operate all of the operations";
    echo -e "  ${clr_gold}binaries$clr_rst: Generate binaries for all platforms";
    echo -e "  ${clr_gold}release$clr_rst: Generate release message from last commit";
    echo -e "  ${clr_gold}headers$clr_rst: Pack all stb-style headers and no implementation header";
    echo -e "  ${clr_gold}usages$clr_rst: Pack all usages from usage folder";
esac
