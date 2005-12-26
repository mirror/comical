CC = `wx-config --cxx`
INCLUDE = -Iunrar
CFLAGS =  `wx-config --cxxflags` -O2 -Wall -pipe -D_UNIX $(INCLUDE)
LDFLAGS = `wx-config --libs` -Lunrar -lunrar
STRIP = strip

.SUFFIXES: .o .cpp .h

.cpp.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

ALL.O := $(patsubst %.cpp,%.o,$(wildcard src/*.cpp))

comical: $(ALL.O) unrar/libunrar.a
	$(CC) -o $@ $(ALL.O) $(LDFLAGS)
	$(STRIP) $@

unrar/libunrar.a:
	$(MAKE) lib -C unrar -f makefile.linux

clean:
	rm -f $(ALL.O)
	rm -f comical
	$(MAKE) clean -C unrar -f makefile.linux
