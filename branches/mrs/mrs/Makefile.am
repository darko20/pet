# -*- makefile -*-

#include $(top_srcdir)/common/Makefile.common

# Add cheap-specific CPPFLAGS/CXXFLAGS/LDFLAGS/LIBS as determined by configure
CPPFLAGS += -I$(top_srcdir)/common -I$(top_srcdir)/fspp -I$(top_srcdir)/cheap 
CXXFLAGS += @CHEAPCXXFLAGS@
LDFLAGS += @CHEAPLDFLAGS@
LIBS += @CHEAPLIBS@
DEFS += -DDYNAMIC_SYMBOLS

# Further CPPFLAGS/CXXFLAGS/LDFLAGS/LIBS should be determined by configure.ac
# if they are system-specific or passed as a command-line option to configure
# if they are requested by the user (this is the whole point of configure).
# So, pass the following flags to configure or adapt configure.ac:
# - superoptimized code and all warnings: "CXXFLAGS=-Wall -O3"
# - profiling with gprof: "CXXFLAGS=-pg"
# - coverage testing with gprof: "CXXFLAGS=-fprofile-arcs -ftest-coverage"
# - not in our gcc yet -fno-eliminate-unused-debug-types:
#   "CXXFLAGS=-UNDEBUG -D_GLIBCXX_DEBUG"
# - Is needed in some cases (required by libpvm3.a):
#   "LDFLAGS=-Wl,--defsym,__ctype_b=0,--defsym,__ctype_tolower=0"
# See README.

# This is always used! <-> documentation
AM_CXXFLAGS = -Wall

#bin_PROGRAMS = mrs

#cheap_SOURCES = cheap.cpp cheap.h
#cheap_LDADD = libcheap.la

#mrs_SOURCES = \
#	mrs.cpp mrs.h \
#	mrs-tfs.cpp mrs-tfs.h \
#	mrs-handler.cpp mrs-handler.h \
#	mrs-states.cpp mrs-states.h \
#	$(top_srcdir)/common/configs.cpp \
#	$(top_srcdir)/common/dag-alloc.cpp \
#	$(top_srcdir)/common/dag-arced.cpp \
#	$(top_srcdir)/common/dag-common.cpp \
#	$(top_srcdir)/common/dag-io.cpp \
#	$(top_srcdir)/common/dagprinter.cpp \
#	$(top_srcdir)/common/dumper.cpp \
#	$(top_srcdir)/common/hash.cpp \
#	$(top_srcdir)/common/lex-io.cpp \
#	$(top_srcdir)/common/lex-tdl.cpp \
#	$(top_srcdir)/common/logging.cpp \
#	$(top_srcdir)/common/mfile.c \
#	$(top_srcdir)/common/settings.cpp \
#	$(top_srcdir)/common/types.cpp \
#	$(top_srcdir)/common/utility.cpp

profclean:
	rm -f *.gcov gmon.out *.bb *.bbg *.da

