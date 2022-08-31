CC ?= gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lncurses -lm -pthread
VGFLAGS = --track-origins=yes #--leak-check=full #--show-leak-kinds=all

PREFIX ?= ~/.local/bin

ifeq ($(CC),gcc)
	CFLAGS += -Wno-stringop-truncation
else ifeq ($(CC),cc)
	ifeq ($(shell readlink `which cc`),gcc)
		CFLAGS += -Wno-stringop-truncation
	endif
endif

ifdef DEBUG
	CFLAGS += -O0 -g
else
	CFLAGS += -O3 -march=native -mtune=native
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

build/help.o: src/nc-help/help.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/layout_parser.o: src/layout_parser.c src/layout_parser.h
	$(CC) $(CFLAGS) -c -o $@ $<

build/input.o: src/input.c src/input.h
	$(CC) $(CFLAGS) -c -o $@ $<

build/disk.o: src/disk.c src/disk.h
	$(CC) $(CFLAGS) -c -o $@ $<

sm: build/list.o build/sm.o build/util.o build/canvas.o build/ui.o build/cpu.o \
		build/memory.o build/network.o build/proc.o build/help.o build/layout_parser.o \
		build/input.o build/disk.o
	$(CC) $(LDFLAGS) -o $@ $^

vgclean:
	rm -f vgcore.*

cgclean:
	rm -f callgrind.out.*

clean: vgclean cgclean
	rm -f build/*.o sm

vg: sm
	valgrind $(VGFLAGS) ./sm 2> err

cg: sm
	valgrind --tool=callgrind -v ./sm -r 100

install: sm
	cp sm $(PREFIX)/sm

.PHONY: clean vg
