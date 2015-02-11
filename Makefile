# Install destination and prefix
DESTDIR ?=
PREFIX ?= /usr/local

# Build flags
FLAGS = -std=gnu99

# Build rules
all: backly
.PHONY: all install clean

backly: backly.c Makefile
	gcc $(FLAGS) -o backly backly.c

install: backly
	install -Dm 0755 backly $(DESTDIR)/$(PREFIX)/bin/backly

clean:
	rm -f backly
