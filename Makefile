INCLUDE_DIRS = -Isrc/rb-tree -Isrc/c-vector
CC = gcc
CFLAGS = -Wall -Wextra $(INCLUDE_DIRS)
LDFLAGS = -lm -pthread
VGFLAGS = --track-origins=yes #--leak-check=full

PREFIX ?= ~/.local/bin

ifneq (, $(shell which mold))
	LDFLAGS += -fuse-ld=mold
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

# Use -lncursesw on apt based systems and -lncurses otherwise,
# may not be correct for all systems but works for me on debian and manjaro :^)
ifneq ($(which apt),"")
	LDFLAGS += -lncursesw
else
	LDFLAGS += -lncurses
endif

source_files = $(wildcard src/*.c src/canvas/*.c src/ps/*.c) \
               src/nc-help/help.c src/rb-tree/rb_tree.c src/ini/ini.c
object_files = $(patsubst src/%.c,build/%.o,$(source_files))

all: build_dirs build/stdafx.h.gch sm

build_dirs:
	@mkdir -p build
	@mkdir -p build/nc-help
	@mkdir -p build/canvas
	@mkdir -p build/ps
	@mkdir -p build/rb-tree
	@mkdir -p build/ini

build/stdafx.h.gch: src/stdafx.h
	$(CC) $(INCLUDE_DIRS) -o $@ $<

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
