# -*- makefile -*-

MRSDIR = @MRSDIR@

CHEAPCPPFLAGS = @CHEAPCPPFLAGS@
CHEAPLDFLAGS = @CHEAPLDFLAGS@
CHEAPLIBS = @CHEAPLIBS@

DEFS = 
# Development switches
OPTIM=-O2
#-O0 
# profiling with gprof
# OPTIM += -pg
# coverage testing with gprof
# OPTIM += -fprofile-arcs -ftest-coverage
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

# This is always used! <-> documentation
AM_CXXFLAGS = -g -Wall $(OPTIM) $(DEFS)
AM_CFLAGS = -g -Wall $(OPTIM) $(DEFS)

# clear defaults, e.g., if i don't want optimisation to be switched on
CXXFLAGS =
CFLAGS =

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

# The dependencies are computed by the compiler.  It may be the best
# idea to switch tracking only on for maintainer-mode.
# The headers do not need to be listed for compilation, but they need
# to be found for making a distribution.

cheap_SOURCES = agenda.h chart.cpp chart.h cheap.cpp cheap.h 		\
	fs.cpp fs.h grammar.cpp grammar.h		 		\
	input-modules.h item-printer.cpp item-printer.h item.cpp item.h	\
	lexicon.cpp lexicon.h lexparser.cpp lexparser.h 		\
	lingo-tokenizer.cpp lingo-tokenizer.h				\
	morph.cpp morph.h options.cpp options.h parse.cpp parse.h 	\
	paths.cpp paths.h position-mapper.h postags.cpp postags.h       \
  restrictor.h sm.cpp sm.h task.cpp task.h itsdb.h timer.h tsdb++.h \
  tsdb++.cpp yy-tokenizer.cpp yy-tokenizer.h

TOMABECHI_UNIF_FILES = dag-tomabechi.cpp dag-tomabechi.h failure.h	\
	failure.cpp qc.h qc.cpp
if TOMABECHI_UNIFIER
DEFS += -DDAG_TOMABECHI
cheap_SOURCES += $(TOMABECHI_UNIF_FILES)
else
if SIMPLE_UNIFIER
DEFS += -DDAG_SIMPLE -DWROBLEWSKI2
nodist_cheap_SOURCES += dag-simple.cpp dag-simple.h
endif
endif

# Add "mrs.h" to MRS source files ... Eric Nichols <eric-n@is.naist.jp>, Jun. 18, 2005
# Is -Dlinux -fstrict-aliasing needed as compiler options?
ECL_FILES = petecl.c petecl.h
ECL_MRS_FILES = cppbridge.cpp cppbridge.h petmrs.h petmrs.c
ECL_PREPROCESSOR_FILES = eclpreprocessor.h eclpreprocessor.cpp #fspp.h fspp.cpp

cheap_LDADD =
if ECL
cheap_SOURCES += $(ECL_FILES) 
if ECLPREPROC
if ICU
cheap_SOURCES += $(ECL_PREPROCESSOR_FILES)
cheap_LDADD +=  ../fspp/libfspp.a -lpreprocessor ../fspp/libfspp.a #fix_me
AM_LDFLAGS += -L../fspp
AM_CPPFLAGS +=  -I../fspp
endif
endif
if ECLMRS
cheap_SOURCES += $(ECL_MRS_FILES)
cheap_LDADD += libmrs.a
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
	echo "(load \"$(MRSDIR)/src/general/loadup\")"; \
	echo "(setq lib-dir (make-pathname :directory '(:relative)))"; \
	echo "(mk::ecl-initialize-libbuild :module \"mrs\")"; \
	echo "(compile-system \"mrs\" :force t)"; \
	echo "(mk::ecl-finalize-libbuild :module \"mrs\")"; \
	) | $(ECL)

XML_FILES = pic-handler.cpp pic-handler.h pic-states.h xmlparser.cpp xmlparser.h xml-tokenizer.h xml-tokenizer.cpp smaf-tokenizer.h smaf-tokenizer.cpp
if XML
cheap_SOURCES += $(XML_FILES)
endif

ICU_FILES = unicode.h unicode.cpp
if ICU
cheap_SOURCES += $(ICU_FILES)
endif

PSQL_FILES = psqllex.h psqllex.cpp
if LEXDB
cheap_SOURCES += $(PSQL_FILES)
endif

YY_FILES = yy.h yy.cpp
if YY
# Should force appropriate recompilation
#DEFS += -DYY
cheap_SOURCES += $(YY_FILES)
endif

# source files that should be distributed in any case, even if not used.
EXTRA_cheap_SOURCES = dumpgram.cpp mtest.cpp pet.cpp			\
	$(ECL_MRS_FILES) $(EXTDICT_FILES) $(ICU_FILES)			\
	$(RCU_FILES) $(TOMABECHI_UNIF_FILES) $(YY_FILES) $(XML_FILES)	\
	$(PSQL_FILES) psqltest.c

EXTRA_DIST = Jamfile


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

# To make ECL:
# ./configure ...
# cd build
# make
# make install

