#
# NOTE: make will not pick up changed to Lisp modules unless
#       'make clean' is first called
#

include $(top_srcdir)/common/Makefile.common

AM_CPPFLAGS = @CHEAPCPPFLAGS@
AM_CXXFLAGS = -Wall
AM_CFLAGS = -Wall
AM_LDFLAGS = @CHEAPLDFLAGS@

DEFS += -DDYNAMIC_SYMBOLS
LIBS += -lstdc++ -licui18n -licudata -lecl

bin_PROGRAMS = fspp

fspp_SOURCES = main.cpp petecl.c petecl.h unicode.cpp unicode.h errors.h
fspp_CPPFLAGS = -I$(top_srcdir)/common @CHEAPCPPFLAGS@
fspp_LDADD = libfspp.a libpreprocessor.a libfspp.a # fix_me
fspp_LDFLAGS = -L. @CHEAPLDFLAGS@

noinst_LIBRARIES = libfspp.a

libfspp_a_SOURCES = fspp.h fspp.cpp petecl.c petecl.h
libfspp_a_LIBADD = libfspp.a libpreprocessor.a libfspp.a

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

libfspp.a: fspp.o libpreprocessor.a
	ar rcs libfspp.a fspp.o # libpreprocessor.a

