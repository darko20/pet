# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT([PET], [0.99.6], [pet@coli.uni-saarland.de])
AC_REVISION($LastChangedRevision$)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign])
dnl dejagnu dist-bzip2 no-dependencies

# These are defaults to make life easier for the developers.  Normally
# you'll write something like this (as far as the libraries are not in
# a standard location):
# $ ./configure CPPFLAGS="-I/proj/contrib/lkb/include -I/proj/contrib/lib/icu/include" LDFLAGS="-L/proj/contrib/lkb/lib/linux -L/proj/contrib/lib/icu/lib"
# or alternatively
# $ ./configure --with-boost=/path/to/boost ...
# (which is not quite the same)
contrib_prefix="/project/cl/deepthought"

BOOSTDIR="$contrib_prefix/local/"
ICUDIR=/usr/local
ITSDBDIR="$contrib_prefix/lkb"
MRSDIR="$ITSDBDIR"
ECLDIR="$contrib_prefix/local"
XERCESCDIR="$contrib_prefix/lib/xerces-c"

# We need to do this here to get a properly working configure with dependency 
# tracking 
# FIXME: Replace `main' with a function in `-lstdc++':
# Check used and needed in linking?
AC_CHECK_LIB([stdc++], [main])

# This is necessary to find the libraries during configuration.
# The "proper way" to do this is via the configure command line.
# It would be nicer it these would only be added if they existed
# _and_ if they are needed (i.e. through an option AND not present yet).
# Currently, the presence is not tested.

export LDFLAGS
export CPPFLAGS

# Features/packages header files and libraries need to be done with
# dependencies: only check if asked, and if not found switch off all
# related/dependent checks.
# IF requested AND libraries and headers OK; THEN add to configuration
# For libraries: it should be possible to choose for static ones


check_tsdb() {
if test -d $ITSDBDIR/include ; then
    (
    CPPFLAGS="$CPPFLAGS -I${ITSDBDIR}/include"
    export CPPFLAGS
    AC_CHECK_HEADERS([itsdb.h], [],
        [
        AC_MSG_WARN([itsdb header not found for --with-tsdb; DISABLED])
        return 1
        ], [])
    )
else 
    AC_MSG_WARN([directory ${ITSDBDIR}/include for --with-tsdb not found; DISABLED])
    return 1
fi 

if test -d ${ITSDBDIR}/lib/linux ; then
    (
    LDFLAGS="$LDFLAGS -L${ITSDBDIR}/lib/linux"
    export LDFLAGS
    AC_CHECK_LIB([itsdb], [capi_register], [ return 0 ], 
        [
        AC_MSG_WARN([itsdb library not found for --with-tsdb; DISABLED])
        return 1
        ])
    )
else 
    AC_MSG_WARN([directory ${ITSDBDIR}/lib/linux for --with-tsdb not found; DISABLED])
    return 1
fi
}

AC_ARG_WITH(tsdb,
[AC_HELP_STRING([--with-tsdb@<:@=DIR@:>@], [use the TSDB library (default=yes)])],
[
case "${withval}" in
    yes) tsdb=true ;;
    no)  tsdb=false ;;
    *)   tsdb=true
         ITSDBDIR=${withval};;
esac
], [tsdb=true])
if test x$tsdb = xtrue && check_tsdb ; then
    AC_DEFINE(TSDBAPI, [], [incr[tsdb()] libraries available, supply API])
    CHEAPLIBS="-litsdb $CHEAPLIBS"
    CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${ITSDBDIR}/include"
    CHEAPLDFLAGS="$CHEAPLDFLAGS -L${ITSDBDIR}/lib/linux -Wl,-rpath,${ITSDBDIR}/lib/linux"
else
    tsdb=false
fi
AM_CONDITIONAL(TSDB, [test x$tsdb = xtrue])



AC_ARG_WITH(mrsdir,
[AC_HELP_STRING([--with-mrsdir@<:@=DIR@:>@], [specify the path to the MRS Lisp sources (LKB top level directory) (default=${ITSDBDIR})])],
[
if test -f "${withval}/src/general/loadup.lisp" ; then
    MRSDIR=${withval}
else
    if test -f "${withval}/general/loadup.lisp" ; then
        MRSDIR=${withval%%/}
        MRSDIR=${MRSDIR%%/src}
    else
        AC_MSG_WARN([no MRS code found in ${withval} for --with-mrsdir])
    fi
fi
], [MRSDIR=${ITSDBDIR}])

eclincldir=""
check_ecl() {
AC_PATH_PROG(ECL, ecl, no, [$PATH:${ECLDIR}/bin])
if test $ECL = no ; then 
    AC_MSG_WARN([no ecl binary found for --with-eclmrs; DISABLED])
    return 1
fi
if test -f ${MRSDIR}/src/general/loadup.lisp ; then
    if test -d ${ECLDIR}/h; then
        eclincldir=${ECLDIR}/h
    else
      if test -d ${ECLDIR}/lib/ecl/h ; then
          eclincldir=${ECLDIR}/lib/ecl/h
      fi
    fi
    if test x$eclincldir != x ; then
        (
        CPPFLAGS="$CPPFLAGS -I${eclincldir} -I${MRSDIR}/include"
        export CPPFLAGS
        AC_CHECK_HEADERS([ecl.h rmrs.h], [], 
            [ AC_MSG_WARN([header files not found for --with-eclmrs; DISABLED])
              return 1 ],[
              #if HAVE_ECL_H
              # include "ecl.h"
              #endif
              ])
        LDFLAGS="$LDFLAGS -L${ECLDIR}/lib/ecl"
        export LDFLAGS
        # ECL Libraries
        AC_CHECK_LIB([ecl], [cl_boot], 
            [ return 0 ], 
            [
            AC_MSG_WARN([libecl.so not found for --with-eclmrs; DISABLED])
            return 1
            ])
        # Not used: -lclos -llsp (?)
        # FIXME: Replace `main' with a function in `-lgc':
        # AC_CHECK_LIB([gc], [main])
        )
    else
        AC_MSG_WARN([directory neither ${ECLDIR}/h nor ${ECLDIR}/lib/h found for --with-eclmrs; DISABLED])
        return 1
    fi
else
    AC_MSG_WARN([no MRS code found in ${MRSDIR} for --with-eclmrs; DISABLED])
    return 1
fi
}

AC_ARG_WITH(eclmrs,
[AC_HELP_STRING([--with-eclmrs@<:@=DIR@:>@], [build MRS library with ECL (default=yes)])],
[
case "${withval}" in
    yes) eclmrs=true ;;
    no)  eclmrs=false ;;
    *)   eclmrs=true
         ECLDIR=${withval} ;;
esac
], [ eclmrs=true ])
if test x$eclmrs = xtrue && check_ecl ; then
    CHEAPLIBS="-lecl $CHEAPLIBS"
    CHEAPLDFLAGS="$CHEAPLDFLAGS -L${ECLDIR}/lib/ecl -Wl,-rpath,${ECLDIR}/lib/ecl"
    CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${eclincldir} -I${MRSDIR}/include"
    AC_DEFINE(HAVE_ECL, [], [Use ECL made MRS library])
else 
    eclmrs=false
fi
AM_CONDITIONAL(ECLMRS, [test x$eclmrs = xtrue])
dnl Substitute this to compile libmrs.a
AC_SUBST(MRSDIR)


check_icu() {
(
if test x$ICUDIR != x ; then
    if test -d ${ICUDIR} && test -d $ICUDIR/include && test -d $ICUDIR/lib ; then 
        LDFLAGS="$LDFLAGS -L${ICUDIR}/lib"
        export LDFLAGS
    else 
        AC_MSG_WARN([directory ${ICUDIR}, ${ICUDIR}/lib or ${ICUDIR}/include not found for --with-icu; DISABLED])
        return 1
    fi
fi
# Not detected by autoheader, but required by ICU
AC_CHECK_LIB([pthread], [pthread_exit], [ icu=true ],
    [ AC_MSG_WARN([pthread library not found for --with-icu; DISABLED])
        return 1 ])
AC_CHECK_LIB([icuuc], [main], [ icu=true ], 
    [ AC_MSG_WARN([icuuc library not found for --with-icu; DISABLED])
        return 1 ])
AC_CHECK_LIB([icui18n], [main], [ icu=true ], 
    [ AC_MSG_WARN([icui18n library not found for --with-icu; DISABLED])
          return 1 ])
AC_CHECK_LIB([icudata], [main], [ icu=true ], 
    [ AC_MSG_WARN([icudata library not found for --with-icu; DISABLED])
        return 1 ])
return 0
)
}

AC_ARG_WITH(icu,
[AC_HELP_STRING([--with-icu@<:@=DIR@:>@], [use the ICU library (default=yes)])],
[
case "${withval}" in
    yes) icu=true ;;
    no)  icu=false ;;
    *)   icu=true
         ICUDIR=${withval} ;;
esac
], [ icu=true ])
if test x$icu = xtrue && check_icu ; then
    if test x$ICUDIR != x ; then
        CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${ICUDIR}/include"
        CHEAPLDFLAGS="$CHEAPLDFLAGS -L${ICUDIR}/lib -Wl,-rpath,${ICUDIR}/lib"
    fi
    CHEAPLIBS="-lpthread -licuuc -licui18n -licudata $CHEAPLIBS"
    AC_DEFINE(HAVE_ICU, [], [use international character library from IBM]) 
else
    icu=false
fi
AM_CONDITIONAL(ICU, [test X$icu = Xtrue])



AC_ARG_WITH(boost,
[AC_HELP_STRING([--with-boost=DIR], [use the BOOST library in DIR])],
[
if test -d "${withval}"; then 
    boost=true
    BOOSTDIR=${withval}
else
    boost=false
    AC_MSG_ERROR(directory ${withval} not found for --with-boost)
fi
],
[ boost=true ])

if test x$boost = xtrue ; then
    AC_LANG_PUSH(C++)
    save_CPP="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS -I${BOOSTDIR}/include"
    AC_CHECK_HEADERS([boost/graph/adjacency_list.hpp boost/graph/topological_sort.hpp boost/lambda/lambda.hpp], [ boost=true ] ,[ boost=false ; break ], [])
    dnl Necessary only for test binary boost:
    dnl AC_CHECK_HEADERS([boost/graph/graph_utility.hpp boost/graph/graphviz.hpp boost/graph/reverse_graph.hpp], [ boost=true ] ,[ boost=false ; break ], [])
    CPPFLAGS="$save_CPP"
    AC_LANG_POP(C++)
fi
AM_CONDITIONAL(BOOST, [test x$boost = xtrue])
if test x$boost = xtrue ; then
    FLOPCPPFLAGS="$FLOPCPPFLAGS -I${BOOSTDIR}/include"
fi



# Not via defines in config.h because there is no choice for flop!
AC_ARG_ENABLE(unifier,
AC_HELP_STRING([--enable-unifier@<:@=ARG@:>@], [use the unifier (default=tomabec
hi)
Possible values are: tomabechi, simple]),
[case "${enableval}" in
   tomabechi) unifier=tomabechi ;;
   simple)    unifier=simple ;;
   *)         AC_MSG_ERROR(bad value ${enableval} for --with-unifier)
esac], [unifier=tomabechi])
# Should be done differently: now they are independent
AM_CONDITIONAL(TOMABECHI_UNIFIER, [test x$unifier = xtomabechi])
AM_CONDITIONAL(SIMPLE_UNIFIER, [test x$unifier = xsimple])


# Not via defines in config.h because there is no choice for flop!
#AC_ARG_ENABLE(dynamic-symbols,
#AC_HELP_STRING([--enable-dynamic-symbols], [enable or disable dynamic symbols (default=enabled)]),
#[dynsymbols=${enableval}], [dynsymbols=yes])
#if test x$dynsymbols=xyes ; then
#   AC_DEFINE([DYNAMIC_SYMBOLS],[],[Enable the use of dynamic symbols/atoms])
#fi

AC_ARG_WITH(xml,
[AC_HELP_STRING([--with-xml@<:@=DIR@:>@], [use the XML input mode (needs xerces c++ library) (default=no)])],
[
case "${withval}" in
    yes) xml=true ;;
    no)  xml=false ;;
    *)   xml=true
            XERCESCDIR=${withval} ;;
esac
if test $xml = true && test -d "${XERCESCDIR}" ; then
    save_LD="$LDFLAGS"
    LDFLAGS="$LDFLAGS -L${XERCESCDIR}/lib"
    AC_CHECK_LIB([xerces-c], [main], [ xml=true ],
        [
        xml=false
        AC_MSG_WARN([Xerces C++ library not found for --with-xml; DISABLED])
        ])
    LDFLAGS="$save_LD"
else
    xml=false
    AC_MSG_WARN([directory ${XERCESCDIR} not found for --with-xml; DISABLED])
fi
], [xml=false])
if test x$xml = xtrue ; then
    CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${XERCESCDIR}/include"
    CHEAPLDFLAGS="$CHEAPLDFLAGS -L${XERCESCDIR}/lib -Wl,-rpath,${XERCESCDIR}/lib"
    CHEAPLIBS="-lxerces-c $CHEAPLIBS"
    AC_DEFINE(HAVE_XML, [], [Enable XML input mode, using XERCES C++ library])
fi
AM_CONDITIONAL(XML, [test x$xml = xtrue])


AC_ARG_ENABLE(external-dictionary,
[AC_HELP_STRING([--enable-external-dictionary@<:@=DIR@:>@], [use an external dictionary (default=no)])],
[case "${enableval}" in
   yes) extdict=true ;;
   no)  extdict=false ;;
   *)   AC_MSG_ERROR([bad value ${enableval} for --enable-external-dictionary])
 esac
 if test x$extdict = xtrue ; then
    AC_DEFINE(HAVE_EXTDICT, [], [enable external dictionary])
 fi
], [extdict=false])
AM_CONDITIONAL(EXTDICT, [test x$extdict = xtrue])

dnl Completely depends on ECL now
dnl Should also test for ECL!
dnl Depends on ECL
dnl AC_ARG_ENABLE(mrs,
dnl AC_HELP_STRING([--enable-mrs@<:@=ARG@:>@], [use the MRS facilities, needs ECL! (default=yes)]),
dnl [case "${enableval}" in
dnl     yes) mrs=true 
dnl          MRSDIR=$ITSDBDIR ;;
dnl     no)  mrs=false ;;
dnl     *)   if test x"${enableval}" != x ; then
dnl            MRSDIR=${enableval}
dnl          fi
dnl  esac
dnl  if test x$mrs = xtrue && test x$ecl = xfalse ; then
dnl     AC_MSG_ERROR(MRS can only be enabled together with ECL!)
dnl     mrs=false
dnl  fi
dnl  if \! test -d $MRSDIR ; then
dnl     AC_MSG_ERROR(directory $(MRSDIR) not found for --enable-mrs!)
dnl  fi
dnl  if test x$mrs = xtrue; then
dnl     AC_SUBST(MRSDIR)
dnl     AC_SUBST(MRS, "true")
dnl  fi
dnl ], [mrs=true])

if test x$lexdb = xtrue; then
    AC_CHECK_LIB([pq], [main], [ CHEAPLIBS="-lpq $CHEAPLIBS" ],
        [
        AC_MSG_WARN([pq library file not found for lexdb; DISABLED])
        lexdb=false
        ])
fi
AM_CONDITIONAL(LEXDB, [test x$lexdb = xtrue])

if test x$psql = xtrue; then
    dnl Only for PGSQL DB
    AC_CHECK_HEADERS([libpq-fe.h],[],
        [
        AC_MSG_WARN([pq header file not found for psql DB; DISABLED])
        psql=false
        ])
fi
AM_CONDITIONAL(PSQLLEX, [test x$psql = xtrue])


AC_ARG_ENABLE(rcu,
[AC_HELP_STRING([--enable-rcu@<:@=ARG@:>@], [use the RCU extensions (default=no)])],
[case "${enableval}" in
   yes) rcu=true ;;
   no)  rcu=false ;;
   *)   AC_MSG_ERROR([bad value ${enableval} for --enable-rcu])
esac], [rcu=false])
AM_CONDITIONAL(RCU, [test x$rcu = xtrue])


AC_ARG_ENABLE(yy,
AC_HELP_STRING([--enable-yy@<:@=ARG@:>@], [use the YY extensions (default=yes)]),
[case "${enableval}" in
   yes) yy=true ;;
   no)  yy=false ;;
   *)   yy=false
        AC_MSG_ERROR(bad value ${enableval} for --enable-yy) ;;
 esac
], [yy=true])
if test x$yy = xtrue; then 
    AC_DEFINE(YY, [], [Include some YY specific code])
fi
AM_CONDITIONAL(YY, [test x$yy = xtrue])

AC_LANG_PUSH(C++)
AC_CHECK_HEADERS([hash_map ext/hash_map],,,)
AC_LANG_PUSH(C++)


AC_SUBST(CHEAPLIBS)
AC_SUBST(CHEAPLDFLAGS)
AC_SUBST(CHEAPCPPFLAGS)

AC_SUBST(FLOPCPPFLAGS)

# The library checks could be made conditional on the selected options

# Checks for programs.
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_MAKE_SET
AC_PROG_RANLIB

dnl Check for doxygen to generate documentation
AC_PATH_PROG(DOXYGEN,doxygen,,$PATH:/usr/local/bin:/usr/bin)

# Checks for libraries.
# FIXME: Replace `main' with a function in `-ldl':
#AC_CHECK_LIB([dl], [main])
# FIXME: Replace `main' with a function in `-liberty':
#AC_CHECK_LIB([iberty], [main])
# Removed because I want to link it statically
# FIXME: Replace `main' with a function in `-litsdb':
# AC_CHECK_LIB([itsdb], [main])
# FIXME: Replace `main' with a function in `-lm':
#AC_CHECK_LIB([m], [main])
# FIXME: Replace `main' with a function in `-lpvm3':
#AC_CHECK_LIB([pvm3], [main])

# Apparently not needed (yet)
# FIXME: Replace `main' with a function in `-lgmp':
# AC_CHECK_LIB([gmp], [main])

# Removed because I want to link it statically
# FIXME: Replace `main' with a function in `-lmrs':
# AC_CHECK_LIB([mrs], [main])

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h locale.h malloc.h netdb.h netinet/in.h stddef.h stdlib.h string.h strings.h sys/param.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_SEARCH_LIBS([getopt_long_only], [iberty], [],
               [AC_MSG_ERROR([No library containing getopt_long_only()], [1])])

AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_SETPGRP
AC_FUNC_SETVBUF_REVERSED
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
# Is becoming obsolete
AC_FUNC_WAIT3
AC_CHECK_FUNCS([bzero gethostbyaddr gettimeofday inet_ntoa munmap setlocale socket strcasecmp strchr strdup strerror strrchr strtol])

# Excluded doc and goofy
AC_CONFIG_FILES([Makefile
	         borland/Makefile
                 cheap/Makefile
		 cheap/Jamfile
                 common/Makefile
                 flop/Makefile
		 flop/Jamfile])
echo ; echo "============================================================="
if test x$boost = xtrue ; then
   echo "Boost is available"
else
   echo "Boost is not available: flop can not be compiled"
fi   
if test x$tsdb = xtrue ; then
   echo "tsdb is available"
else
   echo "tsdb is NOT available"
fi   
if test x$eclmrs = xtrue ; then
   echo "MRS code with ecl is available"
else
   echo "MRS code with ecl is NOT available"
fi   
if test x$icu = xtrue ; then
   echo "ICU is available"
else
   echo "ICU is NOT available"
fi   
if test x$xml = xtrue ; then
   echo "XML input mode is available"
else
   echo "XML input mode is NOT available"
fi   
if test x$extdict = xtrue -a x$lexdb = xtrue -a x$psql = xtrue ; then
   echo "External SQL dictionary is available"
else
   echo "External SQL dictionary is NOT available"
fi   
echo ; echo "============================================================="


AC_OUTPUT
