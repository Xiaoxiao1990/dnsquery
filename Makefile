CC=clang
CFLAGS=-g -Wall

all: dnsq

clean:
	$(RM) -v dnsq *.o

depend: dnsq.o

dnsq.o:
	$(CC) $(CFLAGS) -c dnsq.c

dnsq: dnsq.o
	$(CC) $(CFLAGS) -o dnsq dnsq.o

.FORCE:
	

