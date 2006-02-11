CC = `wx-config --cxx`
INCLUDE = -Iunrar -Iunzip
CPPFLAGS = `wx-config --cxxflags` -O2 -Wall -pipe -D_UNIX $(INCLUDE)
LDFLAGS = `wx-config --libs` -Lunrar -lunrar -Lunzip -lminiunzip
INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

.SUFFIXES: .cpp .d .o .a

ALL.CPP = $(wildcard src/*.cpp)
ALL.D = $(patsubst %.cpp,%.d,$(wildcard src/*.cpp))
ALL.O = $(patsubst %.cpp,%.o,$(wildcard src/*.cpp))

all: comical

-include $(ALL.D)

%.o : %.cpp
	$(CC) $(CPPFLAGS) -c -o $*.o $<

%.d : %.cpp
	@set -e; rm -f $@; \
	 $(CC) -MM $(CPPFLAGS) -MT '$*.o' $< > $@.$$$$; \
	 sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	 rm -f $@.$$$$

comical: $(ALL.O) unrar/libunrar.a unzip/libminiunzip.a
	$(CC) -o $@ $(ALL.O) $(LDFLAGS)

unrar/libunrar.a:
	$(MAKE) lib -C unrar -f makefile.linux

unzip/libminiunzip.a:
	$(MAKE) -C unzip

install: comical
	$(INSTALL_PROGRAM) comical $(DESTDIR)$(bindir)/comical

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install

uninstall:
	rm -f $(DESTDIR)$(bindir)/comical

clean:
	rm -f $(ALL.O) comical

distclean:
	$(MAKE) clean
	$(MAKE) clean -C unrar -f makefile.linux
	$(MAKE) clean -C unzip
	rm -f $(ALL.D)

.PHONY : install install-strip uninstall clean distclean
