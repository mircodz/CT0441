CACHE := ccache
CC := clang

CFLAGS := -Walloca -Wcast-qual -Wconversion -Wformat=2 -Wformat-security -Wnull-dereference -Wstack-protector -Wvla -Warray-bounds -Warray-bounds-pointer-arithmetic -Wassign-enum -Wbad-function-cast -Wconditional-uninitialized -Wconversion -Wfloat-equal -Wformat-type-confusion -Widiomatic-parentheses -Wimplicit-fallthrough -Wloop-analysis -Wpointer-arith -Wshift-sign-overflow -Wshorten-64-to-32 -Wswitch-enum -Wtautological-constant-in-range-compare -Wunreachable-code-aggressive -Wthread-safety -Wthread-safety-beta -Wcomma -fstack-protector-strong -fPIE -fstack-clash-protection

DEBUG ?= 1
ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG -O0 -ggdb
	CFLAGS += -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak -fno-omit-frame-pointer -fsanitize=undefined -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow
	export ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_invalid_pointer_pairs=2
	PREFIX := debug
else
	CFLAGS += -DNDEBUG -O2 -D_FORTIFY_SOURCE=2
	PREFIX := release
endif

LDLIBS   := -pthread -lm

EXEC     := client
DESTDIR  := /usr/local
EXECDIR  := build
OBJDIR   := build
SRCDIR   := src
INCLUDE  := include include/libdz/include

SRCS    = $(shell find $(SRCDIR) -name '*.c')
OBJS    = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

all: $(EXEC)
$(EXEC): $(OBJS) $(HDRS) Makefile
	mkdir -p $(EXECDIR)/$(PREFIX)
	$(CACHE) $(CC) -o $(EXECDIR)/$(PREFIX)/$@ $(OBJS) $(LDLIBS) $(CFLAGS) ./include/libdz/build/release/dz.a

$(OBJDIR)/%.o: $(SRCDIR)/%.c Makefile
	mkdir -p $(shell dirname $@)
	$(CACHE) $(CC) -o $@ $(CFLAGS) $(INCLUDE:%=-I%) -c $<

.PHONY: install
install: $(EXEC)
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	cp $< $(DESTDIR)/$(PREFIX)/bin/$(EXEC)

.PHONY: uninstall
uninstall:
	rm $(DESTDIR)/$(PREFIX)/bin/$(EXEC)

.PHONY: clean
clean:
	ccache -C
	rm -f $(EXECDIR)/$(PREFIX)/$(EXEC) $(OBJS)

$(OBJDIR)/compile_commands.json: Makefile
	make --always-make --dry-run \
		| grep -wE 'gcc|g++|clang|clang++' \
		| grep -w '\-c' \
		| jq -nR '[inputs|{directory:".", command:., file: match(" [^ ]+$$").string[1:]}]' \
		> $(OBJDIR)/compile_commands.json
	ln -fs $(OBJDIR)/compile_commands.json compile_commands.json


run: 
	./$(EXECDIR)/$(PREFIX)/$(EXEC) 2> stderr
