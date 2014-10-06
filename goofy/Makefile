CXX = g++

ITSDBPATH = /proj/perform/itsdb
#ITSDBPATH = .
#ITSDBPATH = /home/uc/flop/itsdb
INCLUDE = -I$(LEDAROOT)/incl -I. -I../common -I$(QTDIR)/include -I$(ITSDBPATH)/src/tsdb/pvm

CPPFLAGS = -Wall -g $(INCLUDE) -DDAG_FIXED
# CPPFLAGS = -Wall -g $(INCLUDE) -DDAG_TOMABECHI
# CPPFLAGS = -Wall -g -O3 $(INCLUDE) -DDAG_SIMPLE
CFLAGS = $(CPPFLAGS)

#LIBS = -L$(LEDAROOT) -Wl,-R,/proj/perform/uc/5lib -lG -lL -lm -lstdc++ -ldl -liberty
LIBS = -L$(LEDAROOT) -lG -lL -lm -lstdc++ -ldl -liberty

#ITSDBLIBS = -L$(ITSDBPATH)/lib/solaris -lpvm3 -lsocket -lnsl -lkstat -lposix4 -litsdb
ITSDBLIBS = -L$(ITSDBPATH)/lib/linux -litsdb -lpvm3 -liberty

VPATH = .:../common

# DAGOBJ = dag-io.o dag-common.o dag-alloc.o trail.o dag-tomabechi.o
# DAGOBJ = dag-io.o dag-common.o dag-alloc.o dag-simple.o
DAGOBJ = dag-dump.o dag-fixed.o

COMMONOBJS = bitcode.o chunk-alloc.o errors.o \
	lex-io.o lex-tdl.o settings.o fs.o grammar-dump.o grammar.o sorts.o tsdb++.o $(DAGOBJ)

PARSEOBJS = parse.o item.o task.o chart.o agenda.o tokenlist.o options.o

#
# cheap
#

CHEAPOBJS = cheap.o $(COMMONOBJS) $(PARSEOBJS)
cheap: $(CHEAPOBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS) $(ITSDBLIBS)
#
# mtest
#

MTESTOBJS = mtest.o chunk-alloc.o
mtest: $(MTESTOBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS)

#
# goofy
#

MOC = moc

OBJMOC = moc_goofy.o

QTLIBS = -L$(QTDIR)/lib -lqt -L/usr/X11R6/lib -lX11 -lXext
GOOFYLIBS = $(LIBS) $(QTLIBS)

GOOFYOBJS = goofy.o $(PARSEOBJS) $(COMMONOBJS) $(OBJMOC)
goofy: $(GOOFYOBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(GOOFYLIBS)

#
# dependencies
#

goofy.o: goofy.cc \
	goofy.h

moc_goofy.o: moc_goofy.cc \
	goofy.h

moc_goofy.cc: goofy.h
	$(MOC) goofy.h -o moc_goofy.cc

include .depend
