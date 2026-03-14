#!/bin/bash

clr_aqua="\e[1;36m"
clr_rst="\e[0m"
clr_red="\e[1;31m"
clr_title="\e[1;35m"

printf "${clr_aqua}Compiling library...$clr_rst\n"
cd ..
make
cd usage

test_lang() {
  lang=$1
  printf "$clr_title========== Testing ${lang^} ==========$clr_rst\n"
  cd $lang
  case $lang in
    c | c89 | c++ | cpp)
      make && ./app ;;
    python | py)
      python main.py ;;
    go)
      make test &&
        make run ;;
    rust)
      cargo run ;;
    *)
      echo -e "$clr_red[ERROR]$clr_rst Unkown usage." ;;
  esac
  cd ..
  printf "$clr_title========== Tested ${lang^} ==========$clr_rst\n"
}

# Test all cases
test_all() {
  test_lang c &&
    test_lang go &&
    test_lang c89 &&
    test_lang rust &&
    test_lang python &&
    test_lang cpp
}

if [ "$1" ==  "all" ]; then
  test_all
else
  test_lang $1
fi
