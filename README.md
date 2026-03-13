# The Logger Library

- ![Language](https://img.shields.io/badge/language-C-blue)
![Language](https://img.shields.io/badge/safe%20in-C++-blue)
- ![Platform](https://img.shields.io/badge/platform-posix%20windows%20apple-brightgreen)
- ![Asynchronous](https://img.shields.io/badge/asynchronous-purple?style=for-the-badge)
![Thread Safe](https://img.shields.io/badge/thread%20safe-brightgreen?style=for-the-badge)
![Flexible](https://img.shields.io/badge/flexible-blue?style=for-the-badge)
![STB-Style](https://img.shields.io/badge/stb-single%20header-yellow?style=for-the-badge)

- This Logger supports apple silicon, windows and unix or unix-like systems.
- It requires minimum C11 or C++17 to compile.
- You can build and generate `.so`, `.dll` and `.dylib` on your machine.

- It does only have header files with implementation,
but if you define `LOGGER_IMPLEMENTATION` during compilation of header, you can get shared object.
- It is written in purely C. Contains C++ stuff for extra options (not included in logger.h, logger.h is pure C)
- Since it is written in C, you can port this library every language supports C FFI (most of them).
You can compile this single header and turn it into shared object file.
- If you don't want to use C11, generate library and use no-implementation header that is automatically generated in Makefile.
(Windows one doesn't generate because yeah)

## Quick Links

- [Pure C STB-Style Header](./logger.h)
- [C++ Stream (<<) support](./loggerstream.hpp)
- [1M Logs Test](tests/stress)
- [Usage in C](usage/c)
- [Usage in C89](usage/c89)
- [Usage in C++](usage/c++)
- [Usage in Python](usage/python)
- [Usage in Rust](usage/rust)
- [Header-only in C++, exported via C ABI (Deprecated)](https://github.com/ilpenSE/logger/tree/deprecated)

# Library Linking/Usage

- You can use this as a true stb-style single-header library like this:
```c
#define LOGGER_IMPLEMENTATION
#include "logger.h"
#include <stdbool.h>

int main() {
  Logger* lg = lg_alloc();
  LoggerConfig config = {
    .localTime = true, .generateDefaultFile = true,
    .policy = LG_DROP
  };
  lg_append_sink(&config, stdout, LG_OUT_TTY);
  lg_init(lg, "logs", config);

  lg_info("Hello, World!");

  lg_destroy(lg);
  lg_free(lg);
}
```

- Compile the header (`logger.h`) and use in any language that supports C FFI. (C, Rust, C++ etc.)
- You can generate shared objects with these commands:

(These commands generate shared objects at build folder)

For GCC and UNIX:
```bash
make
```
or
```bash
make debug=1
```
if you wanna have debug information

For MSVC and WINDOWS with nmake:
```bash
nmake /f Makefile.win
```
or
```bash
nmake /f Makefile.win debug=1
```
for debug information

# API Documentation
- Main initializer function that you may use in C/C++:

`int lg_init(Logger* instance, const char* logs_dir, LoggerConfig config);`

- Initializer with default configs:

`int lg_init_defaults(Logger* instance, const char* logs_dir);`

- The LoggerConfig struct:
```c
typedef struct {
  int localTime;
  int maxFiles;
  int generateDefaultFile;
  LgSinks sinks;
  LgLogPolicy logPolicy;
  log_formatter_t logFormatter;
} LoggerConfig;
```

- Flatted version of LoggerConfig
- It's for languages that doesnt support C structs

```c
int lg_init_flat(Logger* inst, const char* logs_dir,
                   int local_time, int max_log_files, int generateDefaultFile,
                   LgSinks sinks, LgLogPolicy log_policy,
                   log_formatter_t log_formatter);
```

- Main destroyer function that destroys logger instances (DOES NOT MANAGE MEMORY!)

`int lg_destroy(Logger* instance);`

- Check if specific instance is dead or alive (if instance = NULL, checks active instance)

`int lg_is_alive(const Logger* instance);`

- Main producer function that pushes message, level and message length into the queue
- (NOT RECOMMENDED TO USE DIRECTLY, USE MACROS OR F-FUNCTIONS)

`int lg_log_(Logger* inst, const LgLogLevel level, const char* msg, size_t msglen);`

- Wrapper for producer, takes variadics and processes it, used at macros

`int lg_vlog_(Logger* inst, const LgLogLevel level, const char* fmt, ...);`

- Functions that are used at FFIs (F-functions), the level-less and level-aware functions here:

`int lg_flog(const LgLogLevel level, const char* msg);`

`int lg_finfo(const char* msg);`

`int lg_fwarn(const char* msg);`

`int lg_ferror(const char* msg);`

- F-functions with explicit instance, you can put NULL there if you wanna use active instance
- (thats how functions above works)

`int lg_flogi(Logger* inst, const LgLogLevel level, const char* msg);`

`int lg_finfoi(Logger* inst, const char* msg);`

`int lg_ferrori(Logger* inst, const char* msg);`

`int lg_fwarni(Logger* inst, const char* msg);`

- Getter and setter for active instance
- (lg_init automatically sets active instance if it's NULL)

`int lg_set_active_instance(Logger* inst);`

`Logger* lg_get_active_instance();`

## Helper functions that you can use:
- Converts level enum to string

`const char* lg_lvl_to_str(const LgLogLevel level);`

- Gets the time string format which is used at logs and log file names

`int lg_get_time_str(char* buf, int isLocalTime);`

- Used at consumer and you can use these on your custom formatter, go check static format_msg

`void lg_str_format_into(lg_string* s, const char* fmt, ...);`

`void lg_str_write_into(lg_string* s, const char* already_formatted_str);`

- Returns you the default config struct, `lg_init_defaults` depends on this

`LoggerConfig lg_get_defaults();`

- Appends a sink to the config

`int lg_append_sink(LoggerConfig* config, FILE* f, LgOutType type);`

- These functions returns file pointers directly. Use them in FFIs.
And, DO NOT use **garbage**-collected languages' files because
their GC will close it anytime but destroy function also closes it.
This leads to double free.

In ONLY C/C++, you can use stdout/stderr directly.

Return stdout:

`FILE* lg_get_stdout();`

Return stderr:

`FILE* lg_get_stderr();`

Open a file with wb mode:

`FILE* lg_fopen(const char* path);`

(wb is mandatory because payload processor calls fwrite)

- Allocator and freer for heap allocated instances or foreign languages

`Logger* lg_alloc();`

`void lg_free(Logger* inst);`

# Explanation of this logger library (how it works):

- In this asynchronous logger, we have main and writer thread.
- Main thread (Producer) manages lifetime and pushes logs into ring buffer.
- Writer (Consumer) thread writes messages that in the ring buffer to file sinks.
- Sometimes, your log messages may disappear if you stress-test it.
That's because the ring buffer is a **fixed-size** buffer and it can be full.
I have decided the **DROP THE LOG** policy by default at those situations
but you can change it in config. To see in where it dropped, you can define
`LOGGER_DEBUG` in your compilation (with `-DLOGGER_DEBUG`) or runtime (`#define LOGGER_DEBUG`)

## Lock-Free MPSC (Multiple-Producer, Single-Consumer) Ring Buffer (since v4.0)

- This logger doesn't include or use ANY KIND OF MUTEX. It just uses atomics
- The threads won't block each other and cause any mutex contention.
- But the trade-off is that this library doesn't work in C99, requires min. C11 and C++17 (because of inline)
- LogQueue is MPSC ring buffer and has atomic head variable.
This is atomic because both of the sides reads/writes it.
- To avoid false sharing we aligned head and tail and ensured that these two going into different cachelines.
- I also aligned isAlive to avoid false sharing also.
- Why PPR (pop-process-payload) function (`lgi_ppr`) exists because pop does not release that slot
Releasing that slot marks that slot empty and can be overwritable. (Not length == 0 check anymore)
- In block policy, producer will adaptively waits until there's empty space in ring buffer.
- In drop policy, producer tries to fires a log but if ring is full, it'll drop it.
- Adaptive waiting is first, it spins then it spins with pause instruction finally it will sleep for 1 nanosecond
- The 3rd stage loops until there's enough space in ring buffer (we have constants that determines the threshold)
- Go check them: `LOGGER_WAIT_NO_PAUSE_MAGIC = 100` and `LOGGER_WAIT_PAUSE_MAGIC = 1000`
- First threshold determines the spin duration, second one does spin with pause duration

### The Interesting Situation

I was just stress-testing the logger with 1M logs in a row.
I appended stderr sink and measure the elapsed time, time per log and dropped log count.
Time per log call estimately is 35 nanoseconds. And I remove the stderr sink thinking that
it will highly increases the speed but it didn't (it's ~55 ns). Instead, it ran slower than before.
I thought this and I describe it like this:

If you provide stderr sink, consumer thread will significantly slows down. (like 4 times slower)
And, our lock-free approach seems like threads won't block each other. Yes, it doesn't but
consumer thread and producer threads depend on each other because of the MPSC fixed-size
ring buffer. If consumer is so slow and producer keeps pushing, ring buffer is mostly full and
this causes early-return in producer and creates a fake speed perception.
I checked dropped log count and it's like +900K in drop with stderr situation. This proofs my thought:

```
(in tests/stress)
❯ ./app drop true 2> /dev/null
Is stderr?: YES
Log Policy: Drop
Dropped: 967413 logs
Elapsed: 65322363 ns
Per log: 65.32 ns
../../logger.h:530: [DEBUG/INFO]: Writer thread is exiting
```

### About lg_destroy

As I said before, it does not manage your context struct's memory lifetime.
You can allocate it on the stack, heap or global context.

It is so dangerous that calling destroy while producer threads are still active will
trigger race condition or Use-After-Destroy ([see this](./tests/lifetime/race.c#L34))

And it's YOUR responsibility to manage init, push and destroy. The order
MUST be first init, then push some messages finally destroy.
(If you guarantee that the producers finished then you can call destroy safely)

## Sinks (since 4.0)

- In initialization of logger, it accepts max 8 file sink.
- These sinks can have output type (LgOutType) describing that what kind of messages it accepts.
- Formatter also prepares message pack for all kinds of out types.
- In FFI, you have to use library to get stdout/stderr and open a file (in wb mode that logger expects).
- And, DO NOT use language's default file opener or stdout/stderr. If you're NOT using C/C++.
- Instance has extra space for default file this prevents out-of-bounds and simplifies the whole process.

## Multiple Instances (since v3.0)

- This library comes with **multiple-instance** support
- You can set or get active (current) instance with `lg_get_active_instance` and `lg_set_active_instance`
- `lg_init` tries to set active instance if active's NULL
- `lg_destroy` DOES NOT make active instance NULL and frees the memory, it just clears its fields, shutdowns writer thread and flags it is dead (`isAlive=0`). You have to free or allocate your memory (we have `lg_alloc` and `lg_free` you can use them in anywhere else)
- You can make `lg_destroy` parameter NULL and it tries to destroy the active instance like you dont have to do `lg_destroy(lg_get_active_instance())`

```c
LoggerConfig cfg = lg_get_defaults();
Logger* lg1 = lg_alloc();
Logger* lg2 = lg_alloc();

lg_init(lg1, "logs1", cfg);
lg_init(lg2, "logs2", cfg);
// active: lg1

lg_info("This is in logs1 folder.");
lg_infoi(lg2, "This is in logs2 folder.");
// the "i" part stands for "explicit instance"

lg_infoi(NULL, "This is in logs2 folder.");
// this also works and it's using active one

lg_finfo("Hello from function"); // normal functions, used at FFIs
```

- **NOTE**: Every logger instance initialization creates a thread which is expensive to have.

## Customization (since v2.1)

- You can customize log message. The customization limited by just layout.
You can change time string, level and formatted message layout.
- Default Layout: `time_str [level] msg`
- Define `LOGGER_DONT_COLORIZE` if you dont want colorized stdout (in default formatter)
- Note that time_str is evaluated at consumer (writer) thread. It may not show correct time when you call producer.
- If you want to use custom log layout declare formatter function ([example in default](logger.h#L1329)) and assign it in logger config. Don't forget newline char.
- Python and Rust has transpiler for you to get better developer experience.

Latest usage in C:
```c
int myFormatter(const char* time_str, LgLogLevel level,
                const char* msg, uint32_t needed, LgMsgPack pack) {
  LgString* tty_str  = &pack[LG_OUT_TTY];
  LgString* file_str = &pack[LG_OUT_FILE];
  LgString* net_str = &pack[LG_OUT_NET];
  const char* level_str = lg_lvl_to_str(level);

  // equivalent to: needed & (1u << LG_OUT_TTY)
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_TTY)) {
    lg_str_format_into(tty_str, "%s/%s: %s\n", time_str, level_str, msg);
  }
  
  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_FILE)) {
    lg_str_format_into(file_str, "%s/%s: %s\n", time_str, level_str, msg);
  }

  if (LOGGER_CONTAINS_FLAG(needed, LG_OUT_NET)) {
    lg_str_format_into(net_str,
                       "{timestamp:%s, level:%s, msg:%s}\n",
                       time_str, level_str, msg);
  }
  return true;
}

Logger* lg = lg_alloc();
LoggerConfig conf = lg_get_defaults();
conf.logFormatter = myFormatter;
lg_init(lg, "logs", conf);
```

- And you can always add new level and new logger stream instance!
- All you have to do is define macros [like this](logger.h#L408)
- And define stream macro [like this](loggerstream.hpp#L75)
- That's it, you can use your custom level

# C++ Exclusives

- Use loggerstream.hpp if you use C++ in your project or app to enable this kind of stuff:
```cpp
sinfo << "Hello, World!";
serr  << "Error occured!";
swarn << "Warning:" << "Use loggerstream.hpp in C++";
```

- Streams also supports concatination of strings and delimiter char is ` `,
this means swarn in that example prints out `Warning: Use loggerstream.hpp in C++`
and inserts newline (`\n`) automatically.
And in streams, we have support for Qt string (QString) if you use Qt Core library.

# About

- [License (MIT)](./LICENSE)
- [YouTube](https://youtube.com/@ilpenwastaken)
- [Instagram](https://instagram.com/ilpenwastaken)
- [X](https://x.com/ilpenwastaken)
