%.o: %.cpp
	$(COMPILE.cc) $< $(OUTPUT_OPTION)

CXX = g++
ifeq ($(shell test -d /project/cl/deepthought && echo "yes"), yes)
BOOSTDIR=/project/cl/deepthought/local/lib/boost-1.30.2
else
BOOSTDIR=/home/kiefer/tmp/boost-1.30.2
endif

INCLUDE = -I$(BOOSTDIR) -I. -I../common

# unix
CPPFLAGS = -Wall -g -O3 $(INCLUDE) -DNew=new -DGITSETWK -DUSEMMAP -DDAG_SIMPLE -DWROBLEWSKI3 -DFLOP
# cygwin
# CPPFLAGS = -Wall -g -O3 $(INCLUDE) -DLEDA_PREFIX -DGITSETWK -DDAG_SIMPLE -DFLOP

CFLAGS = $(CPPFLAGS)

# linux dynamic
# LIBS = -L$(LEDAROOT) -lG -lL -lm -lstdc++ -ldl -liberty
# linux static
LIBS = -lm -lstdc++ -ldl -liberty
# cygwin
# LIBS = $(LEDAROOT)/libG.a $(LEDAROOT)/libL.a -lm -lstdc++ -liberty

VPATH = .:../common

FLOPOBJS = flop.o builtins.o terms.o settings.o full-form.o lex-io.o lex-tdl.o\
	parse-tdl.o print-chic.o print-tdl.o template.o corefs.o util.o\
	options.o bitcode.o types.o hierarchy.o expand.o reduction.o\
	partition.o dag-arced.o dag-simple.o dag-common.o dag-tdl.o dag-io.o\
	dag-alloc.o dumper.o grammar-dump.o dump.o errors.o chunk-alloc.o\
	utility.o

flop: $(FLOPOBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	/bin/rm -f *.o *~

.depend: $(FLOPOBJS:.o=.cpp)
	$(CC) -MM $(CFLAGS) $^ > $@

ifneq ("$(shell ls .depend 2>/dev/null)", "")
include .depend
endif

