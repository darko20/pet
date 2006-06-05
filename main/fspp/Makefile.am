
CHEAPCPPFLAGS = @CHEAPCPPFLAGS@ 
CHEAPLDFLAGS = @CHEAPLDFLAGS@

# Development switches
OPTIM=-O2

include $(top_srcdir)/common/Makefile.common

######

bin_PROGRAMS = fspp

fspp_SOURCES = main.cpp petecl.c petecl.h unicode.cpp unicode.h errors.h
fspp_LDADD =  libfspp.a libpreprocessor.a libfspp.a # fix_me
fspp_LDFLAGS =  -L.
######

noinst_LIBRARIES = libfspp.a

DEFS += -DDYNAMIC_SYMBOLS

AM_CPPFLAGS = -I$(top_srcdir)/common $(CHEAPCPPFLAGS)
AM_LDFLAGS = $(CHEAPLDFLAGS)
LIBS = -lstdc++ -licui18n -licudata -lecl

AM_CXXFLAGS = -g -Wall $(OPTIM) $(DEFS)
AM_CFLAGS = -g -Wall $(OPTIM) $(DEFS)

CXXFLAGS =
CFLAGS =

libfspp_a_SOURCES = fspp.h fspp.cpp petecl.c petecl.h

libfspp_a_LIBADD = libfspp.a libpreprocessor.a libfspp.a

libpreprocessor.a:
	( \
	echo "(load \"$(PREPROCDIR)/src/general/loadup\")"; \
	echo "(setq lib-dir (make-pathname :directory '(:relative)))"; \
	echo "(mk::ecl-initialize-libbuild :module \"preprocessor\")"; \
	echo "(compile-system \"preprocessor\" :force t)"; \
	echo "(mk::ecl-finalize-libbuild :module \"preprocessor\")"; \
	) | $(ECL)

libfspp.a: fspp.o libpreprocessor.a
	ar rcs libfspp.a fspp.o # libpreprocessor.a

EXTRA_DIST = Jamfile

