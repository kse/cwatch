DESTDIR ?= /

SRC      = code/cwatch.c

CC       = gcc
CFLAGS    = -pipe -std=c99 -O3 -fstack-protector -Wl,-z,relro -Wl,-z,now -fvisibility=hidden -W -Wall -Wno-unused-parameter -Wno-unused-function -Wno-unused-label -Wpointer-arith -Wformat -Wreturn-type -Wsign-compare -Wmultichar -Wformat-nonliteral -Winit-self -Wuninitialized -Wno-deprecated -Wformat-security -Werror -pedantic
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
