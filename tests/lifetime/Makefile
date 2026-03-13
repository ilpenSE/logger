CFLAGS = -I../.. -Wall -Wextra -g -DLOGGER_IMPLEMENTATION

main: main.c ../../logger.h
	$(CC) $(CFLAGS) -o app main.c 

race: race.c ../../logger.h
	$(CC) $(CFLAGS) -o race race.c
