#
# NOTE: make will not pick up changed to Lisp modules unless
#       'make clean' is first called
#


CHEAPCPPFLAGS = @CHEAPCPPFLAGS@ 
CHEAPLDFLAGS = @CHEAPLDFLAGS@

# Development switches
OPTIM=-O2

include $(top_srcdir)/common/Makefile.common

######

bin_PROGRAMS = fspp

fspp_SOURCES = main.cpp unicode.cpp unicode.h errors.h
fspp_LDADD = libfspp.la libpreprocessor.a # fix_me
fspp_LDFLAGS =  -L. $(CHEAPLDFLAGS)
######

noinst_LTLIBRARIES = libfspp.la

DEFS += -DDYNAMIC_SYMBOLS

AM_CPPFLAGS = -I$(top_srcdir)/common $(CHEAPCPPFLAGS)
AM_LDFLAGS = $(CHEAPLDFLAGS)
LIBS = -lstdc++ -licui18n -licudata -lecl

AM_CXXFLAGS = -g -Wall $(OPTIM) $(DEFS)
AM_CFLAGS = -g -Wall $(OPTIM) $(DEFS)

CXXFLAGS =
CFLAGS =

libfspp_la_SOURCES = fspp.h fspp.cpp petecl.c petecl.h

libfspp_la_LIBADD = libpreprocessor.a

# Add ecl libraries to be deleted on "make clean" ... 
MOSTLYCLEANFILES = libfspp.a libpreprocessor.a

libpreprocessor.a:
	( \
	echo "(load \"$(PREPROCDIR)/src/general/loadup\")"; \
	echo "(setq lib-dir (make-pathname :directory '(:relative)))"; \
	echo "(mk::ecl-initialize-libbuild :module \"preprocessor\")"; \
	echo "(compile-system \"preprocessor\" :force t)"; \
	echo "(mk::ecl-finalize-libbuild :module \"preprocessor\")"; \
	) | $(ECL)

#libfspp.a: fspp.o libpreprocessor.a
#	ar rcs libfspp.a fspp.o # libpreprocessor.a

