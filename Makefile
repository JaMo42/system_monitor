CC ?= gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lncurses -lm -pthread
VGFLAGS = --track-origins=yes #--leak-check=full

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

source_files = $(wildcard src/*.c src/canvas/*.c src/ps/*.c) \
               src/nc-help/help.c
object_files = $(patsubst src/%.c,build/%.o,$(source_files))

all: build_dirs build/stdafx.h.gch sm

build_dirs:
	@mkdir -p build
	@mkdir -p build/nc-help
	@mkdir -p build/canvas
	@mkdir -p build/ps

build/stdafx.h.gch: src/stdafx.h
	$(CC) -o $@ $<

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

sm: $(object_files)
	$(CC) -o $@ $^ $(LDFLAGS)

vgclean:
	rm -f vgcore.* callgrind.out.*

clean: vgclean
	rm -f build/*.o sm

vg: sm
	valgrind $(VGFLAGS) ./sm $(VGARGS) 2>err

cg: sm
	valgrind --tool=callgrind -v ./sm -r 100 2>err

install: sm
	cp sm $(PREFIX)/sm

.PHONY: clean vg
