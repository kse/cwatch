DESTDIR ?= /

SRC      = code/cwatch.c

CC       = gcc
CFLAGS   = -O3 -Wall -Wextra -Wno-unused-but-set-variable -ggdb
OUTPUT   = cwatch

all: compile

compile:
	$(CC) $(CFLAGS) $(SRC) -o $(OUTPUT)

install:
	mkdir -p $(DESTDIR)usr/bin
	install -m 755 $(OUTPUT) $(DESTDIR)usr/bin/

clean:
	rm -f $(OUTPUT)
	rm -rf pkg
	rm -rf src
	rm -rf build

buildclean: clean
	rm -f cwatch-git-*.pkg.tar.xz
