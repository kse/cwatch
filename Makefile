SRC=src/cwatch.c
CC=cc
CFLAGS=-O3 -Wall -Wextra -Wno-unused-but-set-variable -ggdb
DEST=cwatch

all: compile

compile:
	$(CC) $(CFLAGS) $(SRC) -o $(DEST)


install:
	install -m 755 $(DEST) /usr/bin/
