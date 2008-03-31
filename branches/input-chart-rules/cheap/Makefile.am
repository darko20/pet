# -*- makefile -*-

# build the current directory first, then the test directory since we
# want to test the objects in this directory
if CPPUNIT
SUBDIRS = . test
endif

include $(top_srcdir)/common/Makefile.common

# Add cheap-specific CPPFLAGS/CXXFLAGS/LDFLAGS/LIBS as determined by configure
CPPFLAGS += -I$(top_srcdir)/common -I$(top_srcdir)/fspp @CHEAPCPPFLAGS@
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

bin_PROGRAMS = cheap
#bin_PROGRAMS += dumpgram
#bin_PROGRAMS += mtest

cheap_SOURCES = cheap.cpp cheap.h
cheap_LDADD   = libcheap.a

#dumpgram_SOURCES = dumpgram.cpp \
#                   $(top_srcdir)/common/errors.h \
#                   $(top_srcdir)/common/grammar-dump.cpp \
#                   $(top_srcdir)/common/grammar-dump.h \
#                   $(top_srcdir)/common/dumper.cpp \
#                   $(top_srcdir)/common/dumper.h

# Convenience library (makes building several executables easier and is
# a tiny step towards a general pet library):
noinst_LIBRARIES = libcheap.a

# The include-file dependencies are automatically computed by Automake
# (`Automatic dependency tracking'). Thus, the headers do not need to be listed
# for compilation, but they need to be found for making a distribution.
# Note that the headers in $(top_srcdir)/common are always included
# (cf. $(top_srcdir)/common/Makefile.am) and are therefore not listed here.
libcheap_a_SOURCES = \
	agenda.h \
	chart.cpp chart.h \
	chart-mapping.cpp chart-mapping.h \
	cheaptimer.h \
	fs.cpp fs.h \
	fs-chart.cpp fs-chart.h \
	fs-chart-util.cpp fs-chart-util.h \
	fsc-tokenizer.cpp fsc-tokenizer.h \
	grammar.cpp grammar.h \
	input-modules.cpp input-modules.h \
	item-printer.cpp item-printer.h \
	item.cpp item.h \
	itsdb.h \
	lexicon.cpp lexicon.h \
	lexparser.cpp lexparser.h \
	lingo-tokenizer.cpp lingo-tokenizer.h \
	morph.cpp morph.h morph-inner.h \
	options.cpp options.h \
	parse.cpp parse.h \
	paths.cpp paths.h \
	position-mapper.h \
	postags.cpp postags.h \
	restrictor.cpp restrictor.h \
	sm.cpp sm.h \
	task.cpp task.h \
	tsdb++.h tsdb++.cpp \
	mrs.h mrs.cpp \
	vpm.h vpm.cpp \
	yy-tokenizer.cpp yy-tokenizer.h \
	$(top_srcdir)/common/bitcode.cpp \
	$(top_srcdir)/common/chunk-alloc.cpp \
	$(top_srcdir)/common/dag-alloc.cpp \
	$(top_srcdir)/common/dag-arced.cpp \
	$(top_srcdir)/common/dag-common.cpp \
	$(top_srcdir)/common/dag-io.cpp \
	$(top_srcdir)/common/dumper.cpp \
	$(top_srcdir)/common/grammar-dump.cpp \
	$(top_srcdir)/common/hash.cpp \
	$(top_srcdir)/common/lex-io.cpp \
	$(top_srcdir)/common/lex-tdl.cpp \
	$(top_srcdir)/common/mfile.c \
	$(top_srcdir)/common/settings.cpp \
	$(top_srcdir)/common/types.cpp \
	$(top_srcdir)/common/utility.cpp

if TOMABECHI_UNIFIER
DEFS += -DDAG_TOMABECHI
libcheap_a_SOURCES += dag-tomabechi.cpp dag-tomabechi.h failure.cpp failure.h \
                      qc.h qc.cpp
endif
if SIMPLE_UNIFIER
DEFS += -DDAG_SIMPLE -DWROBLEWSKI2
libcheap_a_SOURCES += $(top_srcdir)/common/dag-simple.cpp
endif

if ECL
libcheap_a_SOURCES += petecl.c petecl.h
endif
if ECLPREPROC
if ICU
libcheap_a_SOURCES += eclpreprocessor.h eclpreprocessor.cpp #fspp.h fspp.cpp
cheap_LDADD += ../fspp/libfspp.a -lpreprocessor ../fspp/libfspp.a #fix_me
LDFLAGS += -L../fspp
CPPFLAGS += -I../fspp
else
# This looks strange. If ICU is a dependency, it should be ensured by
# configure that ECLPREPROC is only set if all dependencies are met.
endif
endif
if ECLMRS
libcheap_a_SOURCES += cppbridge.cpp cppbridge.h petmrs.h petmrs.c
cheap_LDADD += libmrs.a
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

# if LEXDB
# libcheap_a_SOURCES += psqllex.h psqllex.cpp
# endif

if YY
libcheap_a_SOURCES += yy.h yy.cpp
endif

#if EXTDICT
#libcheap_a_SOURCES += extdict.cpp extdict.h
#endif

#if RCU
# QC_PATH_COMP could be done robustly, ie with weights.  The aim would be
# to find the paths that most quickly find out the biggest losses.
#DEFS += -DROBUST
#libcheap_a_SOURCES += rcu-types.h rcu-types.cpp rcu-dag-node.h
#nodist_libcheap_a_SOURCES += dag-chunk-alloc.h dag-chunk-alloc.cpp \
#                             type-chunk-alloc.cpp type-chunk-alloc.h \
#                             type-alloc.h type-alloc.cpp
# Not done for fair comparison
#DEFS += -UQC_PATH_COMP
#endif

# Distribute the following files in any case:
# (N.B.: all conditional SOURCES are also distributed in any case)
EXTRA_DIST = dumpgram.cpp mtest.cpp pet.cpp psqltest.c

profclean:
	rm -f *.gcov gmon.out *.bb *.bbg *.da

