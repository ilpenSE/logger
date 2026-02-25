#include <stdio.h>
#include "logger.h"

// Test for ODR shit
void do_something() {
  lg_info("hello world!");
}
