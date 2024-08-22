INCLUDE_DIRS = -Isrc/rb-tree -Isrc/c-vector -Isrc/ini
CC = gcc
CFLAGS = -Wall -Wextra $(INCLUDE_DIRS)
LDFLAGS = -lm -pthread
VGFLAGS = --track-origins=yes #--leak-check=full

LDFLAGS += $(shell pkg-config --libs ncurses)

PREFIX ?= ~/.local/bin
# Install path to use in mod_and_install since it has to be run as root so we can't use ~
_SU_INSTALL_PATH = $(shell echo "`pwd | cut -d/ -f1-3`/`echo $(PREFIX) | cut -d/ -f3-`")

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

source_files = $(wildcard src/*.c src/canvas/*.c src/ps/*.c) \
               src/nc-help/help.c src/ini/ini.c
object_files = $(patsubst src/%.c,build/%.o,$(source_files))

formatting_files = $(wildcard src/*.c src/*.h src/canvas/*.c src/canvas/*.h \
                     src/ps/*.c src/ps/*.h)

# Changing the theme structure requires a full rebuild or colors will be
# messed up due to wrong offsets into the theme structure
# XXX: should really just use correct dependencies for all files
deps = src/stdafx.h src/theme.h

all: build_dirs build/stdafx.h.gch sm

build_dirs:
	@mkdir -p build
	@mkdir -p build/nc-help
	@mkdir -p build/canvas
	@mkdir -p build/ps
	@mkdir -p build/rb-tree
	@mkdir -p build/ini

build/stdafx.h.gch: src/stdafx.h
	$(CC) $(INCLUDE_DIRS) -o $@ -x c-header $<

build/%.o: src/%.c $(deps)
	$(CC) $(CFLAGS) -c -o $@ $<

sm: $(object_files)
	$(CC) -o $@ $^ $(LDFLAGS)

vgclean:
	rm -f vgcore.* callgrind.out.*

clean: vgclean
	rm -rf build sm

vg: sm
	valgrind $(VGFLAGS) ./sm $(VGARGS) 2>err

cg: sm
	valgrind --tool=callgrind -v ./sm -r 100 2>err

install: sm
	cp sm $(PREFIX)/sm

mod: sm
	@echo "Warning: Giving sm root permissions under normal execution from now on."
	chown root sm
	chmod +s sm

mod_and_install: mod
	cp sm $(_SU_INSTALL_PATH)/sm

format:
	@clang-format -i $(formatting_files)

.PHONY: clean vg cg install mod mod_and_install format
