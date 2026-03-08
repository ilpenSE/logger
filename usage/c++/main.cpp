#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

#define LOGGER_IMPLEMENTATION
#define LOGGER_DEBUG
#include "logger.h"
#include "loggerstream.hpp"

void hello() {
  std::cout << "====== LOGGER USAGE TEST ======\n";
  std::cout << "Choose a test:\n";
  std::cout << "[0] Simple Test\n";
  std::cout << "[1] Multi-thread Test\n";
  std::cout << "[2] Stress Test\n";
  std::cout << "[3] Use-after-destruct Test\n";
  std::cout << "[4] Print this message\n";
  std::cout << "[5] Exit\n";
}

void log_something(int i) {
  auto tid = std::this_thread::get_id(); 
  size_t id = std::hash<std::thread::id>{}(tid);
  sinfo << "Hello from stream, thread" << id << ", i =" << i;
  lg_info("Hello from API! thread %zu , i = %d", id, i);
}

void destruct_test() {
  sinfo << "Log before destruct";
  if (!lg_destroy(nullptr)) {
    std::cerr << "somehow logger destruct failed\n";
    return;
  }
  sinfo << "Log after destruct"; // UB (crash on x86_64-windows64)
}

void simple_test() {
  // you can use binary op in c++ with loggerstream.hpp
  sinfo << "Hello" << "World!";
  serr << "Some error occured";
  swarn << "Some %20 warning";

  // also you can use classic logger.h api (works on every other language)
  lg_info("info from %s", "api");
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

#include <fstream>
static std::ofstream testFile;

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
  if (!testFile) return;
  testFile << diff.count() << '\n';
  testFile.flush();
}

int myFormatter(int isLocalTime, LgLogLevel level,
                const char* msg, uint32_t needed, LgMsgPack pack) {
  char time_str[LOGGER_TIME_STR_SIZE];
  if (!lg_get_time_str(time_str, isLocalTime)) return false;

  LgString* stdout_str = &pack[LG_OUT_TTY];
  LgString* file_str   = &pack[LG_OUT_FILE];

  const char* lvlstr = lg_lvl_to_str(level);
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_TTY)) {
    lg_str_format_into(
      stdout_str,
      "%s {%s} %s\n",
      time_str, lvlstr, msg);
  }
  
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_FILE)) {
    lg_str_format_into(
      file_str,
      "%s {%s} %s\n",
      time_str, lvlstr, msg);
  }
  return true;
}

int main() {
  Logger* lg = lg_alloc();

  LoggerConfig conf;
  conf.localTime = true;
  conf.printStdout = true;
  conf.maxFiles = 10;
  conf.logPolicy = LG_DROP;
  conf.logFormatter = myFormatter;

  if (!lg_init(lg, "logs", conf)) {
    std::cerr << "[MAIN] Logger init failed\n";
    return -1;
  }

  hello();

  int op = 0;
  while (1) {
    if (!(std::cin >> op)) {
      std::cout << "Please enter a valid operation!\n";
      std::cin.clear();
      std::cin.ignore(10000, '\n');
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
      if (!testFile.is_open())
        testFile = std::ofstream("time.txt", std::ios::app);
      stress_test();
      break;
    case 3:
      destruct_test();
      break;
    case 4:
      hello();
      break;
    case 5:
      goto exit;
    default:
      std::cout << "Please enter a valid operation!\n";;
      break;
    }
  }

exit:
  testFile.close();
  
  if (!lg_destroy(nullptr)) {
    std::cerr << "[MAIN] Logger destruct failed\n";
    return -1;
  }

  lg_free(lg);
  return 0;
}
