# -*- makefile -*-

MRSDIR = @MRSDIR@

CHEAPCPPFLAGS = @CHEAPCPPFLAGS@
CHEAPLDFLAGS = @CHEAPLDFLAGS@
CHEAPLIBS = @CHEAPLIBS@

# Development switches
# PROFILE = -pg
# COVERAGE = -fprofile-arcs -ftest-coverage
OPTIM=-O2
#-O0 
# not in our gcc yet -fno-eliminate-unused-debug-types
#DEFS += -UNDEBUG -D_GLIBCXX_DEBUG

include $(top_srcdir)/common/Makefile.common

# What is this?
#if TSDB
bin_PROGRAMS = cheap #mtest
#else
#bin_PROGRAMS = dumpgram #mtest
#endif

DEFS += -DDYNAMIC_SYMBOLS

# These should be tested appropriately and appear in config.h
AM_CPPFLAGS = -I$(top_srcdir)/common $(CHEAPCPPFLAGS)
# Is needed in some cases (required by libpvm3.a)
# AM_LDFLAGS = -Wl,--defsym,__ctype_b=0,--defsym,__ctype_tolower=0
AM_LDFLAGS = $(CHEAPLDFLAGS)
LIBS += $(CHEAPLIBS)

# In $(top_srcdir)/common
nodist_cheap_SOURCES = bitcode.cpp 	\
	chunk-alloc.cpp			\
	dag-alloc.cpp			\
	dag-arced.cpp			\
	dag-common.cpp			\
	dag-io.cpp			\
	dumper.cpp			\
	grammar-dump.cpp		\
	hash.cpp			\
	lex-io.cpp			\
	lex-tdl.cpp			\
	mfile.c				\
	settings.cpp 			\
	types.cpp			\
	utility.cpp

cheap_SOURCES = agenda.h chart.cpp chart.h cheap.cpp cheap.h 		\
	fs.cpp fs.h grammar.cpp grammar.h		 		\
	input-modules.h item-printer.cpp item-printer.h item.cpp item.h	\
	lexicon.cpp lexicon.h lexparser.cpp lexparser.h 		\
	lingo-tokenizer.cpp lingo-tokenizer.h				\
	morph.cpp morph.h options.cpp options.h parse.cpp parse.h 	\
	paths.cpp paths.h postags.cpp postags.h restrictor.h 		\
	sm.cpp sm.h task.cpp task.h itsdb.h tsdb++.h tsdb++.cpp		\
	yy-tokenizer.cpp yy-tokenizer.h

TOMABECHI_UNIF_FILES = dag-tomabechi.cpp dag-tomabechi.h failure.h	\
	failure.cpp qc.h qc.cpp
if TOMABECHI_UNIFIER
DEFS += -DDAG_TOMABECHI -DQC_PATH_COMP
cheap_SOURCES += $(TOMABECHI_UNIF_FILES)
else
if SIMPLE_UNIFIER
DEFS += -DDAG_SIMPLE -DWROBLEWSKI2
nodist_cheap_SOURCES += dag-simple.cpp dag-simple.h
endif
endif

#There is currently no other way to do MRS than via ECL
# Is -Dlinux -fPIC -fstrict-aliasing needed as compiler options?
#MRS_FILES = petmrs.h petmrs.c
#if ECL
#cheap_LDADD += $(ITSDBDIR)/lib/linux/libmrs.a
#cheap_SOURCES += $(MRS_FILES)
#endif

# Is -Dlinux -fstrict-aliasing needed as compiler options?
ECL_MRS_FILES = cppbridge.cpp cppbridge.h petecl.c petecl.h petmrs.h petmrs.c
if ECLMRS
# ? Dynamically linked or statically?
# LDFLAGS +=  -Wl,--export-dynamic?
# cheap_LDADD += $(ECLDIR)/libecl.a
cheap_SOURCES += $(ECL_MRS_FILES)
cheap_LDADD = libmrs.a
endif

XML_FILES = pic-handler.cpp pic-handler.h xmlparser.cpp xmlparser.h xml-tokenizer.h
if XML
cheap_SOURCES += $(XML_FILES)
endif


EXTDICT_FILES = extdict.cpp extdict.h
if EXTDICT
cheap_SOURCES += $(EXTDICT_FILES)
endif

ICU_FILES = unicode.h unicode.cpp
if ICU
cheap_SOURCES += $(ICU_FILES)
endif

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

PSQL_FILES = psqllex.h psqllex.cpp
if PSQLLEX
#DEFS += +DPSQLLEX
cheap_SOURCES += $(PSQL_FILES)
endif

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

YY_FILES = yy.h yy.cpp
if YY
# Should force appropriate recompilation
#DEFS += -DYY
cheap_SOURCES += $(YY_FILES)
endif

# Not done for fair comparison
# if RCU
# DEFS += -UQC_PATH_COMP
# endif

# This is always used! <-> documentation
AM_CXXFLAGS = -g $(PROFILE) $(OPTIM) $(COVERAGE) -Wall
AM_CFLAGS = -g $(PROFILE) $(OPTIM) $(COVERAGE) -Wall

# I don't want optimisation to be switched on
CXXFLAGS =
CFLAGS =

# ?
EXTRA_cheap_SOURCES = dumpgram.cpp mtest.cpp pet.cpp			\
	$(ECL_MRS_FILES) $(EXTDICT_FILES) $(ICU_FILES)			\
	$(RCU_FILES) $(TOMABECHI_UNIF_FILES) $(YY_FILES) $(XML_FILES)	\
	$(PSQL_FILES) psqltest.c

EXTRA_DIST = Jamfile

MOSTLYCLEANFILES = libmrs.a

# ADDED :I686 TO *FEATURES*
# Executed unconditionally!  Dependencies should be done by ECL
libmrs.a:
	( \
	  echo "(pushnew :i686 *features*)"; \
	  echo ";;(pushnew :debug *features*)"; \
	  echo "(load \"$(MRSDIR)/src/general/loadup\")"; \
	  echo "(si::system (format nil \"cd ~a; find -type f -exec rm {} \;\" (get-binaries-dir \"mrs\")))"; \
	  echo "(setq lib-dir (make-pathname :directory '(:relative)))"; \
	  echo "(compile-system \"mrs\" :force t)"; \
	) | $(ECL)

profclean:
	rm -f *.gcov gmon.out *.bb *.bbg *.da

# To make ECL:
# ./configure ...
# cd build
# make
# make install

# The dependencies are computed by the compiler.  It may be the best
# idea to switch tracking only on for maintainer-mode.
# The headers do not need to be listed for compilation, but they need
# to be found for making a distribution.
