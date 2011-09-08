# -*- makefile -*-

# build the current directory first, then the test directory since we
# want to test the objects in this directory
if CPPUNIT
SUBDIRS = . test
endif

include $(top_srcdir)/common/Makefile.common

# Add cheap-specific CPPFLAGS/CXXFLAGS/LDFLAGS/LIBS as determined by configure
CPPFLAGS += -I$(top_srcdir)/common @CHEAPCPPFLAGS@
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
# TODO remove no-deprecated when switched to unordered_{set|map}
AM_CXXFLAGS = -Wall -Wno-deprecated

bin_PROGRAMS = cheap
#bin_PROGRAMS += dumpgram
#bin_PROGRAMS += mtest

cheap_SOURCES = cheap.cpp cheap.h
cheap_LDADD = libcheap.la

#dumpgram_SOURCES = dumpgram.cpp \
#                   $(top_srcdir)/common/errors.h \
#                   $(top_srcdir)/common/grammar-dump.cpp \
#                   $(top_srcdir)/common/grammar-dump.h \
#                   $(top_srcdir)/common/dumper.cpp \
#                   $(top_srcdir)/common/dumper.h

# The include-file dependencies are automatically computed by Automake
# (`Automatic dependency tracking'). Thus, the headers do not need to be listed
# for compilation, but they need to be found for making a distribution.
# Note that the headers in $(top_srcdir)/common are always included
# (cf. $(top_srcdir)/common/Makefile.am) and are therefore not listed here.
cheaplibsources = \
	agenda.h api.h \
	chart.cpp chart.h \
	chart-mapping.cpp chart-mapping.h \
	cheaptimer.h \
	fs.cpp fs.h \
	fs-chart.cpp fs-chart.h \
	grammar.cpp grammar.h \
	input-modules.cpp input-modules.h \
	item-printer.cpp item-printer.h \
	item.cpp item.h \
	item-unpack.cpp \
	itsdb.h \
	lexicon.cpp lexicon.h \
	lexparser.cpp lexparser.h \
	lingo-tokenizer.cpp lingo-tokenizer.h \
	morph.cpp morph.h morph-inner.h \
	options.cpp options.h \
	parse.cpp parse.h \
	parsenodes.cpp parsenodes.h \
	paths.cpp paths.h \
	position-mapper.h \
	postags.cpp postags.h \
	resources.cpp resources.h \
	restrictor.cpp restrictor.h \
	sessionmanager.cpp sessionmanager.h \
	sm.cpp sm.h \
	task.cpp task.h \
	tsdb++.h tsdb++.cpp \
	mrs.h mrs.cpp \
	mrs-printer.h mrs-printer.cpp \
	vpm.h vpm.cpp \
	pcfg.h pcfg.cpp \
	yy-tokenizer.cpp yy-tokenizer.h \
	repp.cpp repp.h \
	tagger.cpp tagger.h \
	$(top_srcdir)/common/bitcode.cpp \
	$(top_srcdir)/common/chunk-alloc.cpp \
	$(top_srcdir)/common/configs.cpp \
	$(top_srcdir)/common/dag-alloc.cpp \
	$(top_srcdir)/common/dag-arced.cpp \
	$(top_srcdir)/common/dag-common.cpp \
	$(top_srcdir)/common/dag-io.cpp \
	$(top_srcdir)/common/dagprinter.cpp \
	$(top_srcdir)/common/dumper.cpp \
	$(top_srcdir)/common/grammar-dump.cpp \
	$(top_srcdir)/common/hash.cpp \
	$(top_srcdir)/common/lex-io.cpp \
	$(top_srcdir)/common/lex-tdl.cpp \
	$(top_srcdir)/common/logging.cpp \
	$(top_srcdir)/common/mfile.c \
	$(top_srcdir)/common/settings.cpp \
	$(top_srcdir)/common/types.cpp \
	$(top_srcdir)/common/utility.cpp

if TOMABECHI_UNIFIER
DEFS += -DDAG_TOMABECHI
cheaplibsources += dag-tomabechi.cpp dag-tomabechi.h failure.cpp failure.h \
                      qc.h qc.cpp
endif
if SIMPLE_UNIFIER
DEFS += -DDAG_SIMPLE -DWROBLEWSKI2
cheaplibsources += $(top_srcdir)/common/dag-simple.cpp
endif

if XML
cheaplibsources += xmlparser.cpp xmlparser.h \
                   fsc-tokenizer.cpp fsc-tokenizer.h
endif

if ICU
cheaplibsources += unicode.h unicode.cpp
endif

if XMLRPC_C
cheaplibsources += server-xmlrpc.cpp server-xmlrpc.h
endif

#if HAVE_LIBTOOL
# Convenience library (makes building several executables easier and is
# a tiny step towards a general pet library):
noinst_LTLIBRARIES = libcheap.la
libcheap_la_SOURCES=$(cheaplibsources)
# else
# noinst_LIBRARIES = libcheap.a
# libcheap_a_SOURCES=$(cheaplibsources)
# endif

# Distribute the following files in any case:
# (N.B.: all conditional SOURCES are also distributed in any case)
EXTRA_DIST = dumpgram.cpp mtest.cpp pet.cpp

profclean:
	rm -f *.gcov gmon.out *.bb *.bbg *.da

