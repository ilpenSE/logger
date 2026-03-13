#include <stdio.h>
#include <logger.h>

static Logger logger;

int main() {
  LoggerConfig cfg = lg_get_defaults();
  lg_init(&logger, "logs", cfg);

  printf("Is logger alive: %s\n", lg_is_alive(&logger) ? "YES" : "NO");
  lg_info("This should be visible!");

  lg_destroy(&logger);

  printf("Is logger alive: %s\n", lg_is_alive(&logger) ? "YES" : "NO");
  lg_info("This should NOT be visible!");
  return 0;
}
