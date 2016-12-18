CFLAGS := -Wall -g -O2 -std=gnu99

SRCS := \
	src/libsam3/libsam3.c \
	src/libsam3a/libsam3a.c

TESTS := \
	src/ext/tinytest.c \
	test/test.c \
	test/libsam3/test_b32.c

OBJS := ${SRCS:c=o} ${TESTS:c=o}

.PHONY: check
check: libsam3-tests
	./libsam3-tests

libsam3-tests: ${OBJS}
	${CC} $^ -o $@

clean:
	rm -f libsam3-tests ${OBJS}

%.o: %.c Makefile
	${CC} ${CFLAGS} -c $< -o $@
