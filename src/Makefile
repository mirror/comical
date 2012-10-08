INCLUDE = -I../unrar -I../unzip
CFLAGS = -pipe -std=gnu++0x
ifeq ($(DEBUG),1)
CFLAGS += -g -Wall
else
CFLAGS += -O2
endif
CPPFLAGS = `wx-config --cxxflags` $(CFLAGS) -D_UNIX $(INCLUDE)

.SUFFIXES: .cpp .png .h .d .o

DEPS = $(patsubst %.cpp,%.d,$(wildcard *.cpp))
OBJS = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
ICONS = $(patsubst %.png,%.h,$(wildcard *.png))

all: $(OBJS)

%.h : %.png
	@echo "Converting" $<
	@echo "#ifndef _"$*"_png_h" > $@
	@printf "#define _"$*"_png_h\n" >> $@
	@echo "static const unsigned char "$*"_png[] = {" >> $@
	@hexdump -e "13/1 \"0x%02X, \" \"\n\"" $< | sed 's/0x  ,//g' >> $@
	@printf "};\n\n#endif" >> $@

%.o : %.cpp
	$(CC) $(CPPFLAGS) -c -o $*.o $<

%.d : %.cpp
	@set -e; rm -f $@; \
	 $(CC) -MM -MG $(CPPFLAGS) -MT '$*.o' $< > $@.$$$$; \
	 sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	 rm -f $@.$$$$

clean:
	rm -f *.o *.d $(ICONS)

.PHONY : clean

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
