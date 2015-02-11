# Install prefix
PREFIX ?= /usr/local

# Build flags
FLAGS = -std=gnu99

# Build rules
all: backly
.PHONY: all install clean

backly: backly.c Makefile
	gcc $(FLAGS) -o backly backly.c

install: backly
	install -m 0755 backly $(PREFIX)/bin

clean:
	rm -f backly
