CC = `wx-config --cxx`
INCLUDE = -Iunrar -Iunzip
CFLAGS = -O2 -Wall -pipe
CPPFLAGS = `wx-config --cxxflags` $(CFLAGS) -D_UNIX $(INCLUDE)
LDFLAGS = `wx-config --libs` -Lunrar -lunrar -Lunzip -lminiunzip
INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

.SUFFIXES: .c .cpp .png .h .d .o .a

DEPS = $(patsubst %.cpp,%.d,$(wildcard src/*.cpp))
OBJS = $(patsubst %.cpp,%.o,$(wildcard src/*.cpp))
ICONS = $(patsubst %.png,%.h,$(wildcard src/*.png))

all: comical

-include $(DEPS)

%.h : %.png bin2h
	./bin2h -c $< $*.h

%.o : %.c
	$(CC) $(CFLAGS) -c -o $*.o $<

%.o : %.cpp
	$(CC) $(CPPFLAGS) -c -o $*.o $<

# make the ICONS a build dep for the .d files, because -MG doesn't include the
# path on dependencies it can't find
%.d : %.cpp $(ICONS)
	@set -e; rm -f $@; \
	 $(CC) -MM -MG $(CPPFLAGS) -MT '$*.o' $< > $@.$$$$; \
	 sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	 rm -f $@.$$$$

comical: $(OBJS) unrar/libunrar.a unzip/libminiunzip.a
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

unrar/libunrar.a:
	$(MAKE) lib -C unrar -f makefile.linux

unzip/libminiunzip.a:
	$(MAKE) -C unzip

bin2h: src/bin2h.o
	$(CC) -o $@ src/bin2h.o

install: comical
	$(INSTALL_PROGRAM) comical $(DESTDIR)$(bindir)/comical

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install

uninstall:
	rm -f $(DESTDIR)$(bindir)/comical

clean:
	rm -f $(OBJS) $(ICONS) comical

distclean:
	$(MAKE) clean
	$(MAKE) clean -C unrar -f makefile.linux
	$(MAKE) clean -C unzip
	rm -f $(DEPS) bin2h src/bin2h.o

.PHONY : install install-strip uninstall clean distclean
