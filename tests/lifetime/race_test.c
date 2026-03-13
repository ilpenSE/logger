#include <stdio.h>
#include <pthread.h>
#define LOGGER_DEBUG
#include <logger.h>

static pthread_t thread;
static Logger lg;

void* producer(void* arg) {
  (void)arg;
  size_t i = 0;
  size_t failed = 0;
  for (;;) {
    int ok = lg_finfo("hello");
    if (ok) i++;
    else {
      failed++;
      if (failed > 1000000) break;
    }
  }
  printf("Iterated: %ld\n", i);
  return NULL;
}

int main() {
  LoggerConfig cfg = lg_get_defaults();

  // IF YOU UNCOMMENT THIS: RACE CONDITION WILL BE TRIGGERED (at least in my machine)
  //cfg.logPolicy = LG_BLOCK;
  lg_init(&lg, "logs", cfg);
  pthread_create(&thread, NULL, producer, NULL);
  sleep(1); // Wait for producer to push some messages

  // Make sure that all of your producer threads
  // finished (call pthread_join) and then call destroy
  // otherwise it'll trigger something like Use-After-Destroy
  // (btw, destroy does not free your instance struct)

  // THIS IS WRONG:
  lg_destroy(&lg);
  pthread_join(thread, NULL);

  // THIS IS RIGHT:
  //pthread_join(thread, NULL);
  //lg_destroy(&lg);
}
