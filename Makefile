CFLAGS+=-std=c11 -D_GNU_SOURCE
CFLAGS+=-Wall -Wextra -Wpedantic
CFLAGS+=-Wwrite-strings -Wstack-usage=1024 -Wfloat-equal -Waggregate-return -Winline -fstack-usage

LDLIBS+=-lm

all: uct

uct: uct.o

.PHONY: clean debug all

clean:
		-rm *.o *.su uct

debug: CFLAGS+=-g
debug: uct

profile: CFLAGS +=-pg
profile: uct

