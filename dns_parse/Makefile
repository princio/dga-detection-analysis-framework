CC = gcc
CFLAGS = -Wall -O2 -mtune=native

build: bin/dns_parse

DEBUG=-g

install:
	mkdir -p ${DESTDIR}/usr/local/sbin/
	cp bin/* ${DESTDIR}/usr/local/sbin/
	mkdir -p ${DESTDIR}/etc/init.d/
	cp init/dnscapture ${DESTDIR}/etc/init.d/
	cp etc/* ${DESTDIR}/etc/

tar: clean
	if [ -z ${version} ]; then \
		echo "set 'version' env variable first."; \
		false;\
	fi;
	mkdir dns_parse-${version}
	cp -r README LICENSE Makefile *.c *.h etc bin init Makefile dns_parse-${version}/
	tar -czf dns_parse-${version}.tar.gz dns_parse-${version}
	rm -rf dns_parse-${version}

OBJ=rtypes.o strutils.o network.o tcp.o

PSLTRIEDIR := /home/princio/Repo/princio/malware_detection_preditc_file/psltrie/export/

LIBSEARCH := -L$(PSLTRIEDIR)/lib
LIBINCLUDE := -I$(PSLTRIEDIR)/include

LIBS := -lpcap -lpsltrie

bin/dns_parse: dns_parse.c ${OBJ}
	mkdir -p bin
	${CC} ${CFLAGS} ${DEBUG} $(LIBSEARCH) $(LIBINCLUDE) ${OBJ} -o bin/dns_parse dns_parse.c $(LIBS)

rtypes.o: rtypes.c rtypes.h
	${CC} ${CFLAGS} ${DEBUG} $(LIBSEARCH) $(LIBINCLUDE) -c rtypes.c

strutils.o: strutils.h strutils.c
	${CC} ${CFLAGS} ${DEBUG} $(LIBSEARCH) $(LIBINCLUDE) -c strutils.c

tcp.o:  tcp.h tcp.c dns_parse.h
	${CC} ${CFLAGS} ${DEBUG} $(LIBSEARCH) $(LIBINCLUDE) -c tcp.c

network.o:  network.h network.c dns_parse.h
	${CC} ${CFLAGS} ${DEBUG} $(LIBSEARCH) $(LIBINCLUDE) -c network.c

clean:
	clear;
	rm -f *.o
	rm -rf bin/dns_parse dns_parse-*

test_strutils: strutils.c
	mkdir bin
	${CC} -o bin/test_strutils strutils.c
	./bin/test_strutils

ca: clean build