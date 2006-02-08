CC = `wx-config --cxx`
INCLUDE = -Iunrar -Iunzip
CFLAGS = `wx-config --cxxflags` -O2 -Wall -pipe -D_UNIX $(INCLUDE)
LDFLAGS = `wx-config --libs` -Lunrar -lunrar -Lunzip -lminiunzip
STRIP = strip

ALL.O := $(patsubst %.cpp,%.o,$(wildcard src/*.cpp))

comical: $(ALL.O) unrar/libunrar.a unzip/libminiunzip.a
	$(CC) -o $@ $(ALL.O) $(LDFLAGS)
	$(STRIP) $@

unrar/libunrar.a:
	$(MAKE) lib -C unrar -f makefile.linux

unzip/libminiunzip.a:
	$(MAKE) -C unzip

%.o : %.cpp
	$(CC) $(CFLAGS) -c -o $*.o $<

clean:
	rm -f $(ALL.O) comical
	$(MAKE) clean -C unrar -f makefile.linux
	$(MAKE) clean -C unzip
