%.o: %.cpp
	$(COMPILE.cc) $< $(OUTPUT_OPTION)

RM = rm
OBJ = o
CXX = g++
C = c
CC = gcc
LD = gcc
# CC = KCC --
# CXX = KCC
cpp=cpp
c=c

ECLPATH = /project/cl/deepthought/local/lib/ecl

#ICUDIR = /usr
ICUDIR = /project/cl/deepthought/local

ITSDBPATH = /project/cl/deepthought/lkb
INCLUDE = -I. -I../common  -I$(ITSDBPATH)/include -I$(ICUDIR)/include

# for Unix/gcc
CPPFLAGS1 = -Wall -g -DHASH_MAP_AVAIL -DUSEMMAP -DYY $(INCLUDE) \
  -DTSDBAPI -DSOCKET_INTERFACE -DONLINEMORPH -DICU -DECL
# for CygWin
# CPPFLAGS1 = -Wall -g -O3 -DYY $(INCLUDE)

#
# choice here has to correspond to choice for DAGOBJ (see below):
#

# CPPFLAGS = -DDAG_SIMPLE -DWROBLEWSKI2 $(CPPFLAGS1)
CPPFLAGS = -DDAG_TOMABECHI $(CPPFLAGS1) -DQC_PATH_COMP # -DPACKING
# CPPFLAGS = -DDAG_FIXED $(CPPFLAGS1) -DNO_CAST
# CPPFLAGS = -DDAG_SYNTH1 $(CPPFLAGS1)


CFLAGS = $(CPPFLAGS)

# solaris
# LIBS = -Wl,-R,/proj/perform/uc/5lib -lm -lstdc++ -ldl -liberty
# linux dynamic
# LIBS = -lm -lstdc++ -ldl -liberty
# linux static
LIBS = -Wl,--defsym,__ctype_b=0,--defsym,__ctype_tolower=0 \
       -lm -lstdc++ -ldl -liberty
# cygwin
# LIBS = -lm -lstdc++ -liberty

# solaris
# ITSDBLIBS = $(ITSDBPATH)/lib/solaris/libitsdb.a $(ITSDBPATH)/lib/solaris/libpvm3.a -lsocket -lnsl -lkstat -lposix4
# linux dynamic
# ITSDBLIBS = -L$(ITSDBPATH)/lib/linux -litsdb -lpvm3 -liberty
# linux static
ITSDBLIBS = -L$(ITSDBPATH)/lib/linux $(ITSDBPATH)/lib/linux/libitsdb.a -lpvm3 -liberty
# cygwin (no itsdb available)
# ITSDBLIBS =

ECLLIBS = -L$(ECLPATH) -lecl -lclos -llsp -lgc -lgmp \
         -L$(ITSDBPATH)/lib/linux -lmrs

#ICULIBS = -L$(ICUDIR)/lib -licuuc -licui18n -lpthread -ldl
ICULIBS = -Wl,-R,$(ICUDIR)/lib -L$(ICUDIR)/lib -licuuc -licui18n -lpthread -ldl


VPATH = .:../common

#
# choice here has to correspond to choice for CPPFLAGS (see above):
#

# DAGSRC = dag-io.$(cpp) dag-common.$(cpp) dag-arced.$(cpp) dag-alloc.$(cpp) dag-simple.$(cpp)
DAGSRC = dag-io.$(cpp) dag-common.$(cpp) dag-arced.$(cpp) dag-alloc.$(cpp) dag-tomabechi.$(cpp) failure.$(cpp) qc.$(cpp)
# DAGSRC = dag-common.$(cpp) dag-fixed.$(cpp)
# DAGSRC = dag-io.$(cpp) dag-common.$(cpp) dag-arced.$(cpp) dag-alloc.$(cpp) dag-synth1.$(cpp)

COMMONSRCS = bitcode.$(cpp) chunk-alloc.$(cpp) errors.$(cpp) mfile.$(c) \
	lex-io.$(cpp) lex-tdl.$(cpp) settings.$(cpp) fs.$(cpp) dumper.$(cpp) grammar-dump.$(cpp) grammar.$(cpp) types.$(cpp) tsdb++.$(cpp) utility.$(cpp) $(DAGSRC)

PARSESRCS = parse.$(cpp) item.$(cpp) task.$(cpp) chart.$(cpp) agenda.$(cpp) tokenizer.$(cpp) options.$(cpp) postags.$(cpp) inputtoken.$(cpp) inputchart.$(cpp) mrs.$(cpp) yy.$(cpp) morph.$(cpp) unicode.$(cpp) paths.$(cpp) sm.$(cpp) hash.$(cpp) parser.$(cpp) lexicon.$(cpp) petecl.$(c) petmrs.$(c) cppbridge.$(cpp)

#
# cheap
#

CHEAPSRCS = cheap.$(cpp) $(COMMONSRCS) $(PARSESRCS)
CHEAPOBJS = $(patsubst %.$(c), %.o, $(CHEAPSRCS:.$(cpp)=.o))

cheap: $(CHEAPOBJS)
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS) $(ITSDBLIBS) $(ICULIBS) $(ECLLIBS)

petecl.$(OBJ): petecl.$(c) 
	$(CC) $(CFLAGS) -I$(ECLPATH)/h \
	  -o $@ -Dlinux -fstrict-aliasing -c $<
petmrs.$(OBJ): petmrs.$(c) 
	$(CC) $(CFLAGS) -I$(ECLPATH)/h \
	  -o $@ -Dlinux -fstrict-aliasing -c $<


#
# mtest
#

MTESTOBJS = mtest.$(OBJ) chunk-alloc.$(OBJ)
mtest: $(MTESTOBJS)
	$(CPP) $(CFLAGS) -o $@ $^ $(LIBS)

#
# sanity
#

clean:
	$(RM) *.$(OBJ)

#
# rebuild MRS library, using an installed ECL binary (in the PATH)
#
mrs:
	( \
	  echo "(load \"$(ITSDBPATH)/src/general/loadup.lisp\")"; \
	  echo "(compile-system \"mrs\" :force t)"; \
	) | ecl

#
# dependencies
#

depend: $(CHEAPSRCS)
	makedepend -f- $(INCLUDE) -I$(ECLPATH)/h $^ | sed "s/\.c\.o/.o/" > .depend
#	makedepend -f- $(INCLUDE) $^ > .depend

ifneq ("$(shell ls .depend 2>/dev/null)", "")
include .depend
endif