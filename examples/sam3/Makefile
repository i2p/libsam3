CFLAGS := -Wall -g -O2 -std=gnu99

examples: example lookup dclient dserver sclient sserver

example:
	${CC} ${CFLAGS} samtest.c -o samtest ../libsam3/libsam3.o

lookup:
	${CC} ${CFLAGS} namelookup.c -o lookup ../libsam3/libsam3.o

dclient:
	${CC} ${CFLAGS} dgramc.c -o datagram-client ../libsam3/libsam3.o

dserver:
	${CC} ${CFLAGS} dgrams.c -o datagram-server ../libsam3/libsam3.o

sclient:
	${CC} ${CFLAGS} streamc.c -o stream-client ../libsam3/libsam3.o

sserver:
	${CC} ${CFLAGS} streams.c -o stream-server ../libsam3/libsam3.o

clean:
	rm -f samtest lookup datagram-client datagram-server stream-client stream-server
