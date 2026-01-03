#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

#include "logger.h"
#include "loggerstream.hpp"

/**
This is NOT a JSON logger, its human-readable logger
Usage is provided below.

it no longer expects trailing slash, if its not provided, it should generate
if isLocalTime set to 1, it fetches your time but if you set it false it fetches UTC datetime
*/

void log_something(int i) {
  linfo << "Hello from stream, thread " << std::this_thread::get_id() << ", i = " << i;
  lg_info("Hello from API!");
}

void destruct_test() {
  linfo << "Log before destruct";
  if (!lg_destruct()) {
    std::cerr << "somehow logger destruct failed\n";
    return;
  }
  linfo << "Log after destruct";
}

void simple_test() {
  // you can use binary op in c++ with loggerstream.hpp
  linfo << "Hello" << "World!";
  lerror << "Some error occured";
  lwarning << "Some warning";

  // also you can use classic logger.h api (works on every other language)
  lg_info("info from api");
  lg_error("error from api");
  lg_warn("warning from api");
}

void multi_thread() {
  // multi-thread usage
  // since this is thread-safe logger, the logs wont break
  std::thread t1(log_something, -1);
  std::thread t2(log_something, -1);

  t1.join();
  t2.join();
}

void stress_test() {
  // stress test (1000 thread)
  constexpr int THREAD_COUNT = 1000;

  std::vector<std::thread> threads;
  threads.reserve(THREAD_COUNT);

  auto start = std::chrono::high_resolution_clock::now(); // START CLOCK
  for (int i = 0; i < THREAD_COUNT; ++i) {
    threads.emplace_back(log_something, i);
  }
  for (auto& t : threads) {
    t.join();
  }
  auto end = std::chrono::high_resolution_clock::now(); // END THE CLOCK

  // print elapsed time
  std::chrono::duration<double> diff = end - start;
  std::cout << "Elapsed time: " << diff.count() << " s\n";
}

int main(int argc, char** argv) {
  std::string path = "logs";
  int isLocalTime = 1;
  if (argc >= 3) {
    path = argv[1];
    isLocalTime = (std::string(argv[2]) == "1");
  }

  if (!lg_init(path.c_str(), isLocalTime, true)) {
    std::cerr << "[MAIN] Logger init failed\n";
    return -1;
  }

  std::cout << "====== LOGGER USAGE TEST ======\n";
  std::cout << "Choose a test:\n";
  std::cout << "[0] Simple Test\n";
  std::cout << "[1] Multi-thread Test\n";
  std::cout << "[2] Stress Test\n";
  std::cout << "[3] Use-after-destruct Test\n";
  std::cout << "[4] Exit\n";

  int op = 0;
  while (1) {
    if (!(std::cin >> op)) {
      std::cout << "Please enter a valid operation!\n";
      std::cin.clear();
      std::cin.ignore(10000, '\n');
      continue;
    }

    if (op < 0 || op > 4) {
      std::cout << "Please enter a valid operation!\n";
      continue;
    }

    switch (op) {
      case 0:
        simple_test();
        break;
      case 1:
        multi_thread();
        break;
      case 2:
        stress_test();
        break;
      case 3:
        destruct_test();
        break;
      default:
        goto exit;
        break;
    }
  }

exit:
  if (!lg_destruct()) {
    std::cerr << "[MAIN] Logger destruct failed\n";
    return -1;
  }

  return 0;
}
