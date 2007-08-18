# -*- makefile -*-

# build the current directory first, then the test directory since we
# want to test the objects in this directory
SUBDIRS = . test

include $(top_srcdir)/common/Makefile.common

AM_CPPFLAGS = -I$(top_srcdir)/common -I$(top_srcdir)/fspp @CHEAPCPPFLAGS@
AM_CXXFLAGS = -Wall
AM_CFLAGS = -Wall
AM_LDFLAGS = @CHEAPLDFLAGS@

# Development switches
# profiling with gprof
#AM_CXXFLAGS += -pg
# coverage testing with gprof
#AM_CXXFLAGS += -fprofile-arcs -ftest-coverage
# not in our gcc yet -fno-eliminate-unused-debug-types
#AM_CXXFLAGS += -UNDEBUG -D_GLIBCXX_DEBUG

# Is needed in some cases (required by libpvm3.a)
#AM_LDFLAGS += -Wl,--defsym,__ctype_b=0,--defsym,__ctype_tolower=0

LIBS += @CHEAPLIBS@

# types can be added to the type hierarchy at run-time ('dynamic types')
DEFS += -DDYNAMIC_SYMBOLS

bin_PROGRAMS = cheap

# The include-file dependencies are automatically computed by Automake
# (`Automatic dependency tracking'). Thus, the headers do not need to be listed
# for compilation, but they need to be found for making a distribution.
# Note that the headers in $(top_srcdir)/common are always included
# (cf. $(top_srcdir)/common/Makefile.am) and are therefore not listed here.
cheap_SOURCES = cheap.cpp cheap.h
cheap_LDADD   = libcheap.a

# convenience library (makes building several executables easier):
noinst_LIBRARIES = libcheap.a
libcheap_a_SOURCES = \
	agenda.h \
	chart.cpp chart.h \
	cheaptimer.h \
	fs.cpp fs.h \
	grammar.cpp grammar.h \
	input-modules.h \
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
	restrictor.h \
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
	$(top_srcdir)/common/mfile.c $(top_srcdir)/common/mfile.h \
	$(top_srcdir)/common/settings.cpp $(top_srcdir)/common/settings.h \
	$(top_srcdir)/common/types.cpp $(top_srcdir)/common/types.h \
	$(top_srcdir)/common/utility.cpp $(top_srcdir)/common/utility.h

TOMABECHI_UNIF_FILES = dag-tomabechi.cpp dag-tomabechi.h failure.h \
	failure.cpp qc.h qc.cpp
if TOMABECHI_UNIFIER
DEFS += -DDAG_TOMABECHI
libcheap_a_SOURCES += $(TOMABECHI_UNIF_FILES)
else
if SIMPLE_UNIFIER
DEFS += -DDAG_SIMPLE -DWROBLEWSKI2
libcheap_a_SOURCES += $(top_srcdir)/common/dag-simple.cpp
endif
endif

# Add "mrs.h" to MRS source files ... Eric Nichols <eric-n@is.naist.jp>, Jun. 18, 2005
# Is -Dlinux -fstrict-aliasing needed as compiler options?
# Peter Adolphs, Jul 24 2007: a clean solution would be to query `ecl-config'...
ECL_FILES = petecl.c petecl.h
ECL_MRS_FILES = cppbridge.cpp cppbridge.h petmrs.h petmrs.c
ECL_PREPROCESSOR_FILES = eclpreprocessor.h eclpreprocessor.cpp #fspp.h fspp.cpp

if ECL
libcheap_a_SOURCES += $(ECL_FILES) 
if ECLPREPROC
# does the following check of ICU mean that the preprocessor should not be
# compiled if ICU is missing? shouldn't this be checked in configure, then?
# (Peter Adolphs, Jul 24 2007)
if ICU
cheap_SOURCES += $(ECL_PREPROCESSOR_FILES)
cheap_LDADD +=  ../fspp/libfspp.a -lpreprocessor ../fspp/libfspp.a #fix_me
AM_LDFLAGS += -L../fspp
AM_CPPFLAGS +=  -I../fspp
endif
endif
if ECLMRS
libcheap_a_SOURCES += $(ECL_MRS_FILES)
cheap_LDADD += libmrs.a   # this should actually be part of libcheap.a
endif
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

XML_FILES = pic-handler.cpp pic-handler.h pic-states.h \
	pic-tokenizer.h pic-tokenizer.cpp xmlparser.cpp xmlparser.h \
	smaf-tokenizer.h smaf-tokenizer.cpp

if XML
libcheap_a_SOURCES += $(XML_FILES)
endif

ICU_FILES = unicode.h unicode.cpp
if ICU
libcheap_a_SOURCES += $(ICU_FILES)
endif

# PSQL_FILES = psqllex.h psqllex.cpp
# if LEXDB
# libcheap_a_SOURCES += $(PSQL_FILES)
# endif

YY_FILES = yy.h yy.cpp
if YY
# Should force appropriate recompilation
#DEFS += -DYY
libcheap_a_SOURCES += $(YY_FILES)
endif

# source files that should be distributed in any case, even if not used.
EXTRA_libcheap_a_SOURCES = dumpgram.cpp mtest.cpp pet.cpp \
	$(ECL_MRS_FILES) $(EXTDICT_FILES) $(ICU_FILES) \
	$(RCU_FILES) $(TOMABECHI_UNIF_FILES) $(YY_FILES) $(XML_FILES) \
	$(PSQL_FILES) psqltest.c


profclean:
	rm -f *.gcov gmon.out *.bb *.bbg *.da

#EXTDICT_FILES = extdict.cpp extdict.h
#if EXTDICT
#cheap_SOURCES += $(EXTDICT_FILES)
#endif

#RCU_FILES = rcu-types.h rcu-types.cpp rcu-dag-node.h
#if RCU
# QC_PATH_COMP could be done robustly, ie with weights.  The aim would be
# to find the paths that most quickly find out the biggest losses.
#DEFS += -DROBUST
#cheap_SOURCES += $(RCU_FILES)
# nodist_cheap_SOURCES += dag-chunk-alloc.h dag-chunk-alloc.cpp	\
#                         type-chunk-alloc.cpp type-chunk-alloc.h	\
# 	                type-alloc.h type-alloc.cpp
#endif

# Not done for fair comparison
# if RCU
# DEFS += -UQC_PATH_COMP
# endif

# tsdb sources are always needed for cheap. Does somebody really need dumpgram?
# Confusion with LKB itsdb.h possible?  Not really: it should be the same file, but local one is always taken first
#TSDB_FILES = itsdb.h tsdb++.h tsdb++.cpp
#if TSDB
#using dynamic library
#cheap_LDADD += $(ITSDBDIR)/lib/linux/libitsdb.a
#cheap_SOURCES += $(TSDB_FILES)
# else
# nodist_dumpgram_SOURCES = errors.h grammar-dump.cpp	\
# 	grammar-dump.h dumper.cpp dumper.h
# dumpgram_SOURCES = dumpgram.cpp
#endif

