CFLAGS := -Wall -g -O2 -std=gnu99

all: clean examples

examples: example lookup dclient dserver sclient sserver ssserver ssclient

example:
	${CC} ${CFLAGS} samtest.c -o samtest ../libsam3/libsam3.o

lookup:
	${CC} ${CFLAGS} namelookup.c -o lookup ../libsam3/libsam3.o

dclient:
	${CC} ${CFLAGS} dgramc.c -o dgramc ../libsam3/libsam3.o

dserver:
	${CC} ${CFLAGS} dgrams.c -o dgrams ../libsam3/libsam3.o

sclient:
	${CC} ${CFLAGS} streamc.c -o streamc ../libsam3/libsam3.o

sserver:
	${CC} ${CFLAGS} streams.c -o streams ../libsam3/libsam3.o

ssclient:
	${CC} ${CFLAGS} streamcs.c -o streamcs ../libsam3/libsam3.o

ssserver:
	${CC} ${CFLAGS} streamss.c -o streamss ../libsam3/libsam3.o


clean:
	rm -f samtest lookup dgramc dgrams streamc streams streams.key test-lookup

debug:
	sed -i 's|// libsam3_debug = 1;|libsam3_debug = 1;|g' *.c

nodebug:
	sed -i 's|libsam3_debug = 1;|// libsam3_debug = 1;|g' *.c
