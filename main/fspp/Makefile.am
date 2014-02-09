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
LIBS += -lstdc++ -licui18n -licudata -licuuc -lecl

bin_PROGRAMS = fspp

fspp_SOURCES = main.cpp petecl.c petecl.h unicode.cpp unicode.h errors.h
fspp_CPPFLAGS = -I$(top_srcdir)/common @CHEAPCPPFLAGS@
#fspp_LDADD = libfspp.a libpreprocessor.a libfspp.a # fix_me
fspp_LDADD = libfspp.la libpreprocessor.a
fspp_LDFLAGS = -lecl @CHEAPLDFLAGS@

noinst_LTLIBRARIES = libfspp.la

libfspp_la_SOURCES = fspp.h fspp.cpp petecl.c petecl.h
#libfspp_la_LIBADD = libpreprocessor.a
libfspp_la_LDFLAGS = @CHEAPLDFLAGS@ -lecl

# Add ecl libraries to be deleted on "make clean" ...
MOSTLYCLEANFILES = libfspp.la libpreprocessor.a

libpreprocessor.a:
	( \
	echo "(load \"$(PREPROCDIR)/src/general/loadup\")"; \
	echo "(setq lib-dir (make-pathname :directory '(:relative)))"; \
	echo "(mk::ecl-initialize-libbuild :module \"preprocessor\")"; \
	echo "(compile-system \"preprocessor\" :force t)"; \
	echo "(mk::ecl-finalize-libbuild :module \"preprocessor\")"; \
	) | $(ECL)
