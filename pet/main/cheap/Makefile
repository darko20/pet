%.o: %.cpp
	$(COMPILE.cc) $< $(OUTPUT_OPTION)

RM = rm
OBJ = o
CXX = g++
# CC = KCC --c
# CXX = KCC

ITSDBPATH = /home/uc/itsdb
INCLUDE = -I. -I../common  -I$(ITSDBPATH)/include

# for Unix/gcc
CPPFLAGS1 = -Wall -g -O3 -DHASH_MAP_AVAIL -DUSEMMAP -DYY $(INCLUDE) -DTSDBAPI
# for Unix/KCC
# CPPFLAGS1 = -DKCC -DCRASHES_ON_DYNAMIC_CASTS +K3 -DUSEMMAP -DTSDBAPI $(INCLUDE)
# for CygWin
# CPPFLAGS1 = -Wall -g -O3 -DYY $(INCLUDE)

#
# choice here has to correspond to choice for DAGOBJ (see below):
#

# CPPFLAGS = -DDAG_SIMPLE -DWROBLEWSKI2 $(CPPFLAGS1)
CPPFLAGS = -DDAG_TOMABECHI $(CPPFLAGS1) # -DPACKING
# CPPFLAGS = -DDAG_FIXED $(CPPFLAGS1) -DNO_CAST
# CPPFLAGS = -DDAG_SYNTH1 $(CPPFLAGS1)


CFLAGS = $(CPPFLAGS)

# solaris
# LIBS = -Wl,-R,/proj/perform/uc/5lib -lm -lstdc++ -ldl -liberty
# linux dynamic
# LIBS = -lm -lstdc++ -ldl -liberty
# linux static
LIBS = -lm -lstdc++ -ldl -liberty
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

VPATH = .:../common

#
# choice here has to correspond to choice for CPPFLAGS (see above):
#

# DAGOBJ = dag-io.$(OBJ) dag-common.$(OBJ) dag-arced.$(OBJ) dag-alloc.$(OBJ) dag-simple.$(OBJ)
DAGOBJ = dag-io.$(OBJ) dag-common.$(OBJ) dag-arced.$(OBJ) dag-alloc.$(OBJ) dag-tomabechi.$(OBJ) failure.$(OBJ) qc.$(OBJ)
# DAGOBJ = dag-common.$(OBJ) dag-fixed.$(OBJ)
# DAGOBJ = dag-io.$(OBJ) dag-common.$(OBJ) dag-arced.$(OBJ) dag-alloc.$(OBJ) dag-synth1.$(OBJ)

COMMONOBJS = bitcode.$(OBJ) chunk-alloc.$(OBJ) errors.$(OBJ) mfile.$(OBJ) \
	lex-io.$(OBJ) lex-tdl.$(OBJ) settings.$(OBJ) fs.$(OBJ) dumper.$(OBJ) grammar-dump.$(OBJ) grammar.$(OBJ) types.$(OBJ) tsdb++.$(OBJ) utility.$(OBJ) $(DAGOBJ)

PARSEOBJS = parse.$(OBJ) item.$(OBJ) task.$(OBJ) chart.$(OBJ) agenda.$(OBJ) tokenlist.$(OBJ) options.$(OBJ) postags.$(OBJ) mrs.$(OBJ) k2y.$(OBJ) yy.$(OBJ)

#
# cheap
#

CHEAPOBJS = cheap.$(OBJ) $(COMMONOBJS) $(PARSEOBJS)
cheap: $(CHEAPOBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS) $(ITSDBLIBS)

#
# mtest
#

MTESTOBJS = mtest.$(OBJ) chunk-alloc.$(OBJ)
mtest: $(MTESTOBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS)

#
# sanity
#

clean:
	$(RM) *.$(OBJ)

#
# dependencies
#

include .depend

# DO NOT DELETE
