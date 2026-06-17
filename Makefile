CC      ?= cc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra -Wpedantic -Wno-unused-parameter
INCLUDE := -Iinclude
SRC     := src/balloc.c

BUILD   := build
LIB     := $(BUILD)/libballoc.a
TEST_BIN := $(BUILD)/test_runner
BENCH_BIN := $(BUILD)/bench
DEMO_BIN  := $(BUILD)/demo

TEST_SRC := tests/main.c tests/test_arena.c tests/test_pool.c tests/test_heap.c
BENCH_SRC := bench/bench.c
DEMO_SRC := examples/demo.c

.PHONY: all lib test bench demo clean

all: lib

lib: $(LIB)

$(LIB): $(SRC) include/balloc.h | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDE) -c $(SRC) -o $(BUILD)/balloc.o
	ar rcs $@ $(BUILD)/balloc.o

$(TEST_BIN): $(TEST_SRC) $(LIB) tests/test.h | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDE) $(TEST_SRC) $(LIB) -o $@

$(BENCH_BIN): $(BENCH_SRC) $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDE) $(BENCH_SRC) $(LIB) -o $@

$(DEMO_BIN): $(DEMO_SRC) $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDE) $(DEMO_SRC) $(LIB) -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

bench: $(BENCH_BIN)
	./$(BENCH_BIN)

demo: $(DEMO_BIN)
	./$(DEMO_BIN)

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
