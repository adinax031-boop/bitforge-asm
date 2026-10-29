CC ?= cc
CFLAGS ?= -O3 -std=c11 -Wall -Wextra -Wpedantic
CPPFLAGS += -I.
LDLIBS += -pthread
.PHONY: all test bench clean
all: build/libbitforge.a build/test build/bench
build:
	mkdir -p build
build/dispatch.o: dispatch.c bitforge.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
build/kernels.o: kernels.S | build
	$(CC) -c $< -o $@
build/libbitforge.a: build/dispatch.o build/kernels.o
	ar rcs $@ $^
build/test: test.c build/libbitforge.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@
build/bench: bench.c build/libbitforge.a
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@
test: build/test
	./build/test
bench: build/bench
	./build/bench
clean:
	rm -rf build
