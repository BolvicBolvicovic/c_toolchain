BIN=tester
CC=gcc
CFLAGS=-Werror -Wextra -Wall -Wno-unused-function
INC=-I.

SRCS=$(wildcard tests/*.c) vtest.c
OBJS=$(SRCS:.c=.o)

all:    $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -rf $(OBJS) $(BIN)

commands:
	bear -- $(MAKE)


.PHONY: all clean
