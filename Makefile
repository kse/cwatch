DESTDIR ?=
SRC=src/cwatch.c
CC=gcc
CFLAGS=-O3 -Wall -Wextra -Wno-unused-but-set-variable -ggdb
OUTPUT=cwatch

all: compile

compile:
	$(CC) $(CFLAGS) $(SRC) -o $(OUTPUT)

install:
	install -m 755 $(OUTPUT) $(DESTDIR)/usr/bin/
