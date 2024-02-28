# Variables declaration
CC = gcc
CFLAGS = -Wall -Werror -pedantic -std=gnu18
LOGIN = xingke
SUBMITPATH = ~cs537-1/handin/xingke/P3 

# Targets
.PHONY: all
all: wsh

wsh: wsh.c wsh.h
	$(CC) $(CFLAGS) $^ -o $@

run: wsh
	./wsh

pack: wsh.h wsh.c Makefile README.md
	tar -czvf $(LOGIN).tar.gz wsh.h wsh.c Makefile README.md

submit: pack
	cp $(LOGIN).tar.gz $(SUBMITPATH)

