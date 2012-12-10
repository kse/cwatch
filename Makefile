DESTDIR ?= /

SRC      = code/cwatch.c

CC       = gcc
CFLAGS   = -O3 -Wall -Wextra -Wno-unused-but-set-variable -ggdb
OUTPUT   = cwatch

all: compile

compile:
	$(CC) $(CFLAGS) $(SRC) -o $(OUTPUT)

install:
	install -D -m 755 -d $(OUTPUT) $(DESTDIR)usr/bin

clean:
	rm -f $(OUTPUT)
	rm -rf pkg
	rm -rf src
