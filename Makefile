CC = gcc
CFLAGS = -O0 -g
LDFLAGS = -lncurses -lfmt -lm
VGFLAGS = --track-origins=yes --leak-check=full #--show-leak-kinds=all

all: build/stdafx.h.gch sm

build/stdafx.h.gch: stdafx.h
	$(CC) -o $@ $<

build/list.o: list.c list.h
	$(CC) $(CFLAGS) -c -o $@ $<

build/sm.o: sm.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/util.o: util.c util.h
	$(CC) $(CFLAGS) -c -o $@ $<

build/canvas.o: canvas/canvas.c canvas/canvas.h
	$(CC) $(CFLAGS) -c -o $@ $<

build/cpu.o: cpu.c cpu.h
	$(CC) $(CFLAGS) -c -o $@ $<

build/memory.o: memory.c memory.h
	$(CC) $(CFLAGS) -c -o $@ $<

sm: build/list.o build/sm.o build/util.o build/canvas.o build/cpu.o build/memory.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f build/*.o sm vgcore.*

vg: sm
	valgrind $(VGFLAGS) ./sm

.PHONY: clean vg
