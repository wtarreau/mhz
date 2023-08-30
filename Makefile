CC         ?= gcc
CFLAGS     ?= -O3 -Wall -fomit-frame-pointer

all: mhz

mhz: %: %.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

clean:
	rm -f mhz *.o *~
