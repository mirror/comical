# these are the headers for your project


#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = comical

# the application source, library search path, and link libraries

comical_LDFLAGS = $(WX_LIBS) -L/home/jathey/dev/unrar-3.3.6

comical_SOURCES = ComicalApp.cpp ComicalCanvas.cpp ComicBookRAR.cpp ComicBookZIP.cpp Resize.cpp rarstrm.cpp ComicBook.cpp
noinst_HEADERS = ComicalApp.h ComicalCanvas.h ComicBook.h ComicBookRAR.h ComicBookZIP.h Filters.h Resize.h unzip.h

AM_CFLAGS = -DRAR_DLL
AM_CXXFLAGS = -DRAR_DLL

INCLUDES = -I/home/jathey/dev/unrar-3.3.6
comical_LDADD = -lunrar
