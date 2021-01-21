CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lncurses -lfmt -lm
VGFLAGS = --track-origins=yes --leak-check=full #--show-leak-kinds=all

ifdef RELEASE
	CFLAGS += -O3 -march=native -mtune=native
else
	CFLAGS += -O0 -g
endif

ifdef PROFILE
	CFLAGS += -pg
	LDFLAGS += -pg
endif

all: build/stdafx.h.gch sm

build/stdafx.h.gch: src/stdafx.h
	$(CC) -o $@ $<

build/list.o: src/list.c src/list.h
	$(CC) $(CFLAGS) -c -o $@ $<

build/sm.o: src/sm.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/util.o: src/util.c src/util.h
	$(CC) $(CFLAGS) -c -o $@ $<

build/canvas.o: src/canvas/canvas.c src/canvas/canvas.h
	$(CC) $(CFLAGS) -c -o $@ $<

build/ui.o: src/ui.c src/ui.h
	$(CC) $(CFLAGS) -c -o $@ $<

build/cpu.o: src/cpu.c src/cpu.h
	$(CC) $(CFLAGS) -c -o $@ $<

build/memory.o: src/memory.c src/memory.h
	$(CC) $(CFLAGS) -c -o $@ $<

build/network.o: src/network.c src/network.h
	$(CC) $(CFLAGS) -c -o $@ $<

build/proc.o: src/proc.c src/proc.h
	$(CC) $(CFLAGS) -c -o $@ $<

sm: build/list.o build/sm.o build/util.o build/canvas.o build/ui.o build/cpu.o build/memory.o build/network.o build/proc.o
	$(CC) $(LDFLAGS) -o $@ $^

vgclean:
	rm -f vgcore.*

cgclean:
	rm -f callgrind.out.*

clean: vgclean cgclean
	rm -f build/*.o sm

vg: sm
	valgrind $(VGFLAGS) ./sm

cg: sm
	valgrind --tool=callgrind -v ./sm -r 100

.PHONY: clean vg
