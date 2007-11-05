# -*- makefile -*-

# build the current directory first, then the test directory since we
# want to test the objects in this directory
SUBDIRS = . test

include $(top_srcdir)/common/Makefile.common

AM_CPPFLAGS = -I$(top_srcdir)/common -I$(top_srcdir)/fspp @CHEAPCPPFLAGS@
AM_CXXFLAGS = -Wall
AM_CFLAGS   = -Wall
AM_LDFLAGS  = @CHEAPLDFLAGS@
LIBS += @CHEAPLIBS@

# Development switches
# profiling with gprof
#AM_CXXFLAGS += -pg
# coverage testing with gprof
#AM_CXXFLAGS += -fprofile-arcs -ftest-coverage
# not in our gcc yet -fno-eliminate-unused-debug-types
#AM_CXXFLAGS += -UNDEBUG -D_GLIBCXX_DEBUG

# Is needed in some cases (required by libpvm3.a)
#AM_LDFLAGS += -Wl,--defsym,__ctype_b=0,--defsym,__ctype_tolower=0

# types can be added to the type hierarchy at run-time ('dynamic types')
DEFS += -DDYNAMIC_SYMBOLS

bin_PROGRAMS = cheap

cheap_SOURCES = cheap.cpp cheap.h
cheap_LDADD   = libcheap.a

# convenience library (makes building several executables easier):
noinst_LIBRARIES = libcheap.a
# The include-file dependencies are automatically computed by Automake
# (`Automatic dependency tracking'). Thus, the headers do not need to be listed
# for compilation, but they need to be found for making a distribution.
# Note that the headers in $(top_srcdir)/common are always included
# (cf. $(top_srcdir)/common/Makefile.am) and need not be listed here.
libcheap_a_SOURCES = \
	agenda.h \
	chart.cpp chart.h \
	cheaptimer.h \
	fs.cpp fs.h \
	grammar.cpp grammar.h \
	input-modules.cpp input-modules.h \
	item-printer.cpp item-printer.h \
	item.cpp item.h \
	itsdb.h \
	lexicon.cpp lexicon.h \
	lexparser.cpp lexparser.h \
	lingo-tokenizer.cpp lingo-tokenizer.h \
	morph.cpp morph.h \
	options.cpp options.h \
	parse.cpp parse.h \
	paths.cpp paths.h \
	position-mapper.h \
	postags.cpp postags.h \
	restrictor.cpp restrictor.h \
	sm.cpp sm.h \
	task.cpp task.h \
	tsdb++.h tsdb++.cpp \
	yy-tokenizer.cpp yy-tokenizer.h \
	$(top_srcdir)/common/bitcode.cpp $(top_srcdir)/common/bitcode.h \
	$(top_srcdir)/common/chunk-alloc.cpp $(top_srcdir)/common/chunk-alloc.h \
	$(top_srcdir)/common/dag-alloc.cpp $(top_srcdir)/common/dag-alloc.h \
	$(top_srcdir)/common/dag-arced.cpp $(top_srcdir)/common/dag-arced.h \
	$(top_srcdir)/common/dag-common.cpp $(top_srcdir)/common/dag-common.h \
	$(top_srcdir)/common/dag-io.cpp \
	$(top_srcdir)/common/dumper.cpp $(top_srcdir)/common/dumper.h \
	$(top_srcdir)/common/grammar-dump.cpp $(top_srcdir)/common/grammar-dump.h \
	$(top_srcdir)/common/hash.cpp $(top_srcdir)/common/hash.h \
	$(top_srcdir)/common/hashing.h \
	$(top_srcdir)/common/lex-io.cpp $(top_srcdir)/common/lex-io.h \
	$(top_srcdir)/common/lex-tdl.cpp $(top_srcdir)/common/lex-tdl.h \
	$(top_srcdir)/common/list-int.h \
	$(top_srcdir)/common/mfile.c $(top_srcdir)/common/mfile.h \
	$(top_srcdir)/common/settings.cpp $(top_srcdir)/common/settings.h \
	$(top_srcdir)/common/types.cpp $(top_srcdir)/common/types.h \
	$(top_srcdir)/common/utility.cpp $(top_srcdir)/common/utility.h

if TOMABECHI_UNIFIER
DEFS += -DDAG_TOMABECHI
libcheap_a_SOURCES += dag-tomabechi.cpp dag-tomabechi.h failure.h \
                      failure.cpp qc.h qc.cpp
endif
if SIMPLE_UNIFIER
DEFS += -DDAG_SIMPLE -DWROBLEWSKI2
libcheap_a_SOURCES += $(top_srcdir)/common/dag-simple.cpp
endif

# Is -Dlinux -fstrict-aliasing needed as compiler options?
# Peter Adolphs, Jul 24 2007: a clean solution would be to query `ecl-config'...
if ECL
libcheap_a_SOURCES += petecl.c petecl.h
endif
if ECLPREPROC
libcheap_a_SOURCES += eclpreprocessor.h eclpreprocessor.cpp #fspp.h fspp.cpp
cheap_LDADD += ../fspp/libfspp.a -lpreprocessor #fix_me
AM_LDFLAGS += -L../fspp
AM_CPPFLAGS += -I../fspp
endif
if ECLMRS
libcheap_a_SOURCES += cppbridge.cpp cppbridge.h petmrs.h petmrs.c
cheap_LDADD += libmrs.a   # this should actually be part of libcheap.a
endif

# Add ecl libraries to be deleted on "make clean" ... 
# Eric Nichols <eric-n@is.naist.jp>, Jun. 18, 2005
MOSTLYCLEANFILES = libmrs.a

../fspp/libfspp.a:
	make -C ../fspp libfspp.a

libmrs.a:
	( \
	echo ";;(pushnew :debug *features*)"; \
	echo "(load \"@MRSDIR@/src/general/loadup\")"; \
	echo "(setq lib-dir (make-pathname :directory '(:relative)))"; \
	echo "(mk::ecl-initialize-libbuild :module \"mrs\")"; \
	echo "(compile-system \"mrs\" :force t)"; \
	echo "(mk::ecl-finalize-libbuild :module \"mrs\")"; \
	) | $(ECL)

if XML
libcheap_a_SOURCES += pic-handler.cpp pic-handler.h pic-states.h \
                      pic-tokenizer.h pic-tokenizer.cpp \
                      xmlparser.cpp xmlparser.h \
                      smaf-tokenizer.h smaf-tokenizer.cpp
endif

if ICU
libcheap_a_SOURCES += unicode.h unicode.cpp
endif

if YY
libcheap_a_SOURCES += yy.h yy.cpp
endif

#if EXTDICT
#libcheap_a_SOURCES += extdict.cpp extdict.h
#endif

#if LEXDB
#libcheap_a_SOURCES += psqllex.h psqllex.cpp
#endif

#if RCU
# QC_PATH_COMP could be done robustly, ie with weights.  The aim would be
# to find the paths that most quickly find out the biggest losses.
#DEFS += -DROBUST
#libcheap_a_SOURCES += rcu-types.h rcu-types.cpp rcu-dag-node.h
#libcheap_a_SOURCES += dag-chunk-alloc.h dag-chunk-alloc.cpp \
#                      type-chunk-alloc.cpp type-chunk-alloc.h \
#                      type-alloc.h type-alloc.cpp
# Not done for fair comparison
#DEFS += -UQC_PATH_COMP
#endif

# distribute the following files in any case:
EXTRA_DIST = dumpgram.cpp mtest.cpp pet.cpp psqltest.c

profclean:
	rm -f *.gcov gmon.out *.bb *.bbg *.da

