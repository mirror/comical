CC = `wx-config --cxx`
LD = `wx-config --ld`
INCLUDE = -Iunrar
CFLAGS =  `wx-config --cxxflags` -g -Wall -pedantic -pipe -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DRARDLL -D_UNIX $(INCLUDE)
LDFLAGS = `wx-config --libs` -Lunrar -lunrar


.SUFFIXES: .o .cpp .c .h

.cpp.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

ALL.O = src/ComicBook.o src/ComicBookRAR.o src/ComicBookZIP.o src/ComicalApp.o src/ComicalCanvas.o src/Resize.o src/rarstrm.o

comical: $(ALL.O) unrar/libunrar.a
	$(CC) -o $@ $(ALL.O) $(LDFLAGS)

unrar/libunrar.a:
	$(MAKE) lib -C unrar -f makefile.linux

clean:
	rm -f $(ALL.O)
