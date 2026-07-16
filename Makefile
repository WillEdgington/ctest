CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -Iinclude -MMD -MP
LDFLAGS =
DEVFLAGS = -fsanitize=address,undefined -g
RELFLAGS = -O3

DEP_DIRS := $(wildcard deps/*)
DEP_NAMES := $(notdir $(DEP_DIRS))

CFLAGS += $(foreach dir,$(DEP_DIRS),-I$(dir)/include)
LDFLAGS += $(foreach dir,$(DEP_DIRS),-L$(dir))
LDFLAGS += $(foreach name,$(DEP_NAMES),-l$(name))

# Assume expected archive targets for dependencies are of the path deps/$(name)/lib$(name).a
DEP_LIBS := $(foreach name,$(DEP_NAMES),deps/$(name)/lib$(name).a)

# Core outputs for ctest framework (CLI runner (ctest) and framework library (libctest.a))
BIN = ctest
LIB_A = libctest.a

LIB_SRC := $(wildcard src/libctest/*.c)
LIB_OBJ := $(LIB_SRC:.c=.o)

CLI_SRC := $(wildcard src/ctest_cli/*.c)
CLI_OBJ := $(CLI_SRC:.c=.o)

ALL_OBJ = $(CLI_OBJ) $(LIB_OBJ)

TEST_SRC := $(wildcard tests/libctest/*.c) $(wildcard tests/ctest_cli/*.c)
TEST_BIN := $(TEST_SRC:.c=)

DEPS = $(ALL_OBJ:.o=.d) $(TEST_SRC:.c=.d)

.PHONY: all clean test debug setup_deps

all: $(BIN) $(LIB_A)

debug: CFLAGS += $(DEVFLAGS)
debug: LDFLAGS += $(DEVFLAGS)
debug: all

# Command line runner binary links against core library objects
$(BIN): $(CLI_OBJ) $(LIB_OBJ) $(DEP_LIBS)
		$(CC) $(CFLAGS) $(CLI_OBJ) $(LIB_OBJ) $(LDFLAGS) -o $@

# Package libctest.a so projects can easily link with -lctest
$(LIB_A): $(LIB_OBJ)
		ar rcs $@ $(LIB_OBJ)

$(DEP_LIBS):
		$(MAKE) -C $(@D)

%.o: %.c
		$(CC) $(CFLAGS) -c -o $@ $<

setup_deps:
		git submodule update --init --recursive

# Test target runs suite through ctest binary
test: CFLAGS += $(DEVFLAGS)
test: LDFLAGS += $(DEVFLAGS)
test: $(BIN) $(TEST_BIN)
		@echo "Executing framework test suite..."
		./$(BIN) tests/

# Test code binaries are linked cleanly against core engine objects
tests/%: tests/%.c $(LIB_OBJ) $(DEP_LIBS)
		$(CC) $(CFLAGS) $< $(LIB_OBJ) $(LDFLAGS) -o $@

-include $(DEPS)

clean:
		rm -f $(ALL_OBJ) $(TEST_BIN) $(DEPS) $(BIN) $(LIB_A)

clean_deps: clean
		@for dir in $(DEP_DIRS); do $(MAKE) -C $$dir clean; done
