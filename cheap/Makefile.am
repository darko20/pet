# -*- makefile -*-

# build the current directory first, then the test directory since we
# want to test the objects in this directory
if CPPUNIT
SUBDIRS = . test
endif

include $(top_srcdir)/common/Makefile.common

# Add cheap-specific CPPFLAGS/CXXFLAGS/LDFLAGS/LIBS as determined by configure
CPPFLAGS += -I$(top_srcdir)/common -I$(top_srcdir)/fspp -I$(top_srcdir)/mrs @CHEAPCPPFLAGS@
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
	restrictor.cpp restrictor.h \
	sessionmanager.cpp sessionmanager.h \
	sm.cpp sm.h \
	task.cpp task.h \
	tsdb++.h tsdb++.cpp \
	pcfg.h pcfg.cpp \
	yy-tokenizer.cpp yy-tokenizer.h \
	$(top_srcdir)/mrs/mrs.cpp \
	$(top_srcdir)/mrs/mrs.h \
	$(top_srcdir)/mrs/mrs-printer.cpp \
	$(top_srcdir)/mrs/mrs-printer.h \
	$(top_srcdir)/mrs/vpm.cpp \
	$(top_srcdir)/mrs/vpm.h \
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

if ECL
cheaplibsources += petecl.c petecl.h
endif
if ECLPREPROC
if ICU
cheaplibsources += eclpreprocessor.h eclpreprocessor.cpp #fspp.h fspp.cpp
cheap_LDADD += ../fspp/libfspp.la -lpreprocessor #fix_me
LDFLAGS += -L../fspp
CPPFLAGS += -I../fspp
else
# This looks strange. If ICU is a dependency, it should be ensured by
# configure that ECLPREPROC is only set if all dependencies are met.
endif
endif
if ECLMRS
cheaplibsources += cppbridge.cpp cppbridge.h petmrs.h petmrs.c
cheap_LDADD += libmrs.a
endif

# Add ecl libraries to be deleted on "make clean" ...
# Eric Nichols <eric-n@is.naist.jp>, Jun. 18, 2005
MOSTLYCLEANFILES = libmrs.a

../fspp/libfspp.la:
	make -C ../fspp libfspp.la

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
cheaplibsources += pic-handler.cpp pic-handler.h pic-states.h \
                      pic-tokenizer.h pic-tokenizer.cpp \
                      xmlparser.cpp xmlparser.h \
                      smaf-tokenizer.h smaf-tokenizer.cpp \
                      fsc-tokenizer.cpp fsc-tokenizer.h
endif

if ICU
cheaplibsources += unicode.h unicode.cpp
endif

if XMLRPC_C
cheaplibsources += server-xmlrpc.cpp server-xmlrpc.h
endif

# if LEXDB
# cheaplibsources += psqllex.h psqllex.cpp
# endif

if YY
cheaplibsources += yy.h yy.cpp
endif

#if EXTDICT
#cheaplibsources += extdict.cpp extdict.h
#endif

#if RCU
# QC_PATH_COMP could be done robustly, ie with weights.  The aim would be
# to find the paths that most quickly find out the biggest losses.
#DEFS += -DROBUST
#cheaplibsources += rcu-types.h rcu-types.cpp rcu-dag-node.h
#nodist_libcheap_la_SOURCES += dag-chunk-alloc.h dag-chunk-alloc.cpp \
#                             type-chunk-alloc.cpp type-chunk-alloc.h \
#                             type-alloc.h type-alloc.cpp
# Not done for fair comparison
#DEFS += -UQC_PATH_COMP
#endif

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
EXTRA_DIST = dumpgram.cpp mtest.cpp pet.cpp psqltest.c

profclean:
	rm -f *.gcov gmon.out *.bb *.bbg *.da

