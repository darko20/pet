#%.o: %.cpp
#	$(COMPILE.cc) $< $(OUTPUT_OPTION)

RM = rm
OBJ = o
CXX = g++
CC = gcc
LD = gcc
# CC = KCC --
# CXX = KCC
cpp=cpp
c=c

ifeq ($(shell test -d /project/cl/deepthought && echo "yes"), yes)
ECLPATH = /project/cl/deepthought/local/ecl
ICUDIR = /project/cl/deepthought/local
ITSDBPATH = /project/cl/deepthought/lkb
XERCESPATH = /project/cl/deepthought/local/lib/xerces-c
else 
ECLPATH = /usr/local
ICUDIR = /usr/local
ITSDBPATH = /home/kiefer/Lisp/lkb
XERCESPATH = /usr/local/lib/xerces-c
endif

INCLUDE = -I. -I../common  -I$(ITSDBPATH)/include -I$(ICUDIR)/include

#PROFILING=-pg
MRS=-DECL
#XML=yes

DEFINES =-DHAVE_HASH_MAP -DHAVE_MMAP -DYY -DTSDBAPI -DSOCKET_INTERFACE -DONLINEMORPH -DICU -DDYNAMIC_SYMBOLS -DTSDBFILEAPI $(MRS)

#
# choice here has to correspond to choice for DAGOBJ (see below):
#

DEFINES += -DDAG_TOMABECHI -DQC_PATH_COMP # -DPACKING
# DEFINES += -DDAG_SIMPLE -DWROBLEWSKI2
# DEFINES += -DDAG_FIXED -DNO_CAST
# DEFINES += -DDAG_SYNTH1

# for Unix/gcc
CPPFLAGS = $(DEFINES) $(INCLUDE)
# for CygWin
# CPPFLAGS1 = -Wall -g -DYY $(INCLUDE)

CXXFLAGS = -g -Wall $(PROFILING) $(CPPFLAGS)
CFLAGS = -g -Wall $(PROFILING) $(CPPFLAGS)

# solaris
# LIBS = -Wl,-R,/proj/perform/uc/5lib -lm -lstdc++ -ldl -liberty
# linux dynamic: all standard libraries are provided by the compiler
# LIBS = -lm -lstdc++ -lc -ldl
# cygwin
# LIBS = -lm -lstdc++ -liberty

# solaris
# ITSDBLIBS = $(ITSDBPATH)/lib/solaris/libitsdb.a $(ITSDBPATH)/lib/solaris/libpvm3.a -lsocket -lnsl -lkstat -lposix4
# linux dynamic
#ITSDBLIBS = -L$(ITSDBPATH)/lib/linux -litsdb -lpvm3 -liberty
# linux static
ITSDBLIBS = -Wl,-R,$(ITSDBPATH)/lib/linux -L$(ITSDBPATH)/lib/linux -litsdb -lpvm3
#ITSDBLIBS = -L$(ITSDBPATH)/lib/linux $(ITSDBPATH)/lib/linux/libitsdb.a -lpvm3 -liberty
# cygwin (no itsdb available)
# ITSDBLIBS =

ICULIBS = -Wl,-R,$(ICUDIR)/lib -L$(ICUDIR)/lib -licuuc -licui18n -lpthread

ECLLIBS = -Wl,-R,$(ECLPATH)/lib/ecl -L$(ECLPATH)/lib/ecl -lecl -L$(ITSDBPATH)/lib/linux -lmrs

XERCESLIBS = -Wl,-R,$(XERCESPATH)/lib -L$(XERCESPATH)/lib -lxerces-c

LIBS+=$(ITSDBLIBS) $(ICULIBS)

VPATH = .:../common

#
# choice here has to correspond to choice for CPPFLAGS (see above):
#

# DAGSRC = dag-io.$(cpp) dag-common.$(cpp) dag-arced.$(cpp) dag-alloc.$(cpp) dag-simple.$(cpp)
DAGSRC = dag-io.$(cpp) dag-common.$(cpp) dag-arced.$(cpp) dag-alloc.$(cpp) dag-tomabechi.$(cpp) failure.$(cpp) qc.$(cpp)
# DAGSRC = dag-common.$(cpp) dag-fixed.$(cpp)
# DAGSRC = dag-io.$(cpp) dag-common.$(cpp) dag-arced.$(cpp) dag-alloc.$(cpp) dag-synth1.$(cpp)

COMMONSRCS = bitcode.$(cpp) chunk-alloc.$(cpp) mfile.$(c) \
	     lex-io.$(cpp) lex-tdl.$(cpp) settings.$(cpp) fs.$(cpp)\
             dumper.$(cpp) grammar-dump.$(cpp) grammar.$(cpp) types.$(cpp)\
	     tsdb++.$(cpp) utility.$(cpp) $(DAGSRC)

PARSESRCS = chart.$(cpp) hash.$(cpp) item.$(cpp) item-printer.$(cpp) \
	    lexicon.$(cpp) lexparser.$(cpp) lingo-tokenizer.$(cpp)\
            morph.$(cpp) options.$(cpp)\
            parse.$(cpp) paths.$(cpp) postags.$(cpp)\
	    sm.$(cpp) task.$(cpp) unicode.$(cpp)\
	    yy.$(cpp) yy-tokenizer.$(cpp) # mrs.$(cpp) 

ECLSRCS = cppbridge.$(cpp) petecl.$(c) petmrs.$(c) 

XMLSRCS = pic-handler.$(cpp) xmlparser.$(cpp)

# inputtoken.$(cpp) inputchart.$(cpp) tokenizer.$(cpp)

CHEAPSRCS = cheap.$(cpp) $(COMMONSRCS) $(PARSESRCS)

ifdef XML
  INCLUDE+= -I$(XERCESPATH)/include
  LIBS+= $(XERCESLIBS)
  CHEAPSRCS+= $(XMLSRCS)
endif

ifeq "$(MRS)" "-DECL"
  LIBS+= $(ECLLIBS)
  CHEAPSRCS+= $(ECLSRCS)
endif

CHEAPOBJS = $(patsubst %.$(c), %.o, $(CHEAPSRCS:.$(cpp)=.o))

#
# dependencies
#

.%.d: %.c
	set -e; $(CC) -MM $(CFLAGS) -I$(ECLPATH)/lib/ecl/h $< \
	    | sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	    [ -s $@ ] || rm -f $@

.%.d: %.cpp
	set -e; $(CPP) -MM $(CPPFLAGS) -I$(ECLPATH)/lib/ecl/h $< \
	    | sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	    [ -s $@ ] || rm -f $@

# .depend: $(CHEAPSRCS)
# 	$(CC) -MM $(CFLAGS) -I$(ECLPATH)/lib/ecl/h $^ > $@
#
# ifneq ("$(shell ls .depend 2>/dev/null)", "")
# include .depend
# endif

deap: $(CHEAPOBJS)
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS)

#
# cheap: full optimization and global libs
#

cheap: CPPFLAGS+= -O3
cheap: CFLAGS+= -O3
cheap: $(CHEAPOBJS)
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS)


petecl.$(OBJ): petecl.$(c) 
	$(CC) $(CFLAGS) -I$(ECLPATH)/lib/ecl/h \
	  -o $@ -Dlinux -fstrict-aliasing -c $<
petmrs.$(OBJ): petmrs.$(c) 
	$(CC) $(CFLAGS) -I$(ECLPATH)/lib/ecl/h \
	  -o $@ -Dlinux -fstrict-aliasing -c $<


#
# goofy
#

MOC = moc

OBJMOC = moc_goofy.o

moc_goofy.o: moc_goofy.cc \
	goofy.h

moc_goofy.cc: goofy.h
	$(MOC) goofy.h -o moc_goofy.cc

QTLIBS = -L$(QTDIR)/lib -lqt -lqt-mt -lqutil -L/usr/X11R6/lib -lX11 -lXext

GOOFYOBJS = goofy.o $(CHEAPOBJS) $(OBJMOC)
goofy: INCLUDE+= -I$(QTDIR)/include
goofy: $(GOOFYOBJS)
	$(LD) $(CFLAGS) -o $@ $^ $(QTLIBS) $(LIBS)

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
	find $(ITSDBPATH)/src/.lecl/ -type f -exec rm {} \;
	( \
	  echo "(load \"$(ITSDBPATH)/src/general/loadup.lisp\")"; \
	  echo "(compile-system \"mrs\")"; \
	) | $(ECLPATH)/bin/ecl
	if test -f cheap ; then rm cheap ; fi

include $(patsubst %.$(c), .%.d, $(patsubst %.$(cpp), .%.d, $(CHEAPSRCS)))
