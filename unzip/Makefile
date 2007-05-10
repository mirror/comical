#
# Makefile for minizip unzip library
#  Unix-like systems
#

CC=gcc
CFLAGS=-Os

##########################

OBJECTS=unzip.o ioapi.o

.c.o: $(CC) -c $(CFLAGS) $*.c

all: unziplib

clean:
	@rm -f *.o *.bak *~ *.a

unziplib: $(OBJECTS)
	@rm -f libminiunzip.a
	ar rcs libminiunzip.a $(OBJECTS)
