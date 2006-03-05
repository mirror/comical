INCLUDE = -I../unrar -I../unzip
CFLAGS = -O2 -Wall -pipe
CPPFLAGS = `wx-config --cxxflags` $(CFLAGS) -D_UNIX $(INCLUDE)

.SUFFIXES: .cpp .png .h .d .o

DEPS = $(patsubst %.cpp,%.d,$(wildcard *.cpp))
OBJS = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
ICONS = $(patsubst %.png,%.h,$(wildcard *.png))

all: $(OBJS)

-include $(DEPS)

%.h : %.png
	@echo "Converting" $<
	@echo "#ifndef _"$*"_png_h" > $@
	@echo -e "#define _"$*"_png_h\n" >> $@
	@echo "static const unsigned char "$*"_png[] = {" >> $@
	@hexdump -e "13/1 \"0x%02X, \" \"\n\"" $< | sed 's/0x  ,//g' >> $@
	@echo -e "};\n\n#endif" >> $@

%.o : %.cpp
	$(CC) $(CPPFLAGS) -c -o $*.o $<

%.d : %.cpp
	@set -e; rm -f $@; \
	 $(CC) -MM -MG $(CPPFLAGS) -MT '$*.o' $< > $@.$$$$; \
	 sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	 rm -f $@.$$$$

clean:
	rm -f $(OBJS) $(ICONS)

.PHONY : clean
