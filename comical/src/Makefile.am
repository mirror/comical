# these are the headers for your project


#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = comical

# the application source, library search path, and link libraries

comical_LDFLAGS = $(WX_LIBS) 

comical_SOURCES =  ComicalApp.cpp ComicalCanvas.cpp ComicBook.cpp ComicBookBZ2.cpp ComicBookRAR.cpp ComicBookZIP.cpp Resize.cpp
noinst_HEADERS = ComicalApp.h ComicalCanvas.h ComicBook.h ComicBookBZ2.h ComicBookRAR.h ComicBookZIP.h Filters.h pstream.h Resize.h ComicalLog.h
