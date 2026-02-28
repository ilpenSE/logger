#!/bin/bash

clr_title="\e[1;36m"
clr_rst="\e[0m"

printf "${clr_title}Compiling library...$clr_rst\n"
cd ..
make
cd usage

# Test C
test_c() {
    printf "$clr_title==$clr_title======== Testing C ==========$clr_rst\n"
    cd c
    make && ./app
    cd ..
    printf "$clr_title========== C Test END ==========$clr_rst\n"
}

# Test JS
test_js() {
    printf "$clr_title========== Testing JavaScript (Bun) ==========$clr_rst\n"
    cd javascript
    bun main.js
    cd ..
    printf "$clr_title========== JS Test END ==========$clr_rst\n"
}

# Test Python
test_python() {
    printf "$clr_title========== Testing Python ==========$clr_rst\n"
    cd python
    python main.py # Activate an env with cffi installed
    cd ..
    printf "$clr_title========== Python Test END ==========$clr_rst\n"
}

# Test Java
test_java() {
    printf "$clr_title========== Testing Java ==========$clr_rst\n"
    cd java
    make run
    cd ..
    printf "$clr_title========== Java Test END ==========$clr_rst\n"
}

# Test Rust
test_rust() {
    printf "$clr_title========== Testing Rust ==========$clr_rst\n"
    cd rust
    ./start.sh
    cd ..
    printf "$clr_title========== Rust Test END ==========$clr_rst\n"
}

# Test C++
test_cpp() {
    printf "$clr_title========== Testing C++ ==========$clr_rst\n"
    cd c++
    make && ./app
    cd ..
    printf "$clr_title========== C++ Test END ==========$clr_rst\n"
}

# Test all cases
test_all() {
    test_c &&
    test_python &&
    test_java &&
    test_js && 
    test_cpp &&
    test_rust
}

case $1 in
    js | javascript)
       test_js ;;
    cpp | c++)
        test_cpp ;;
    rust)
        test_rust ;;
    c)
        test_c ;;
    py | python)
       test_python ;;
    java)
        test_java ;;
    *)
        test_all ;;
esac
