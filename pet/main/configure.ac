# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT([PET], [0.99.11], [kiefer@dfki.de])
AC_REVISION($LastChangedRevision$)
AC_CONFIG_SRCDIR([pet-config.h.in])
AC_CONFIG_HEADERS([pet-config.h])

AM_INIT_AUTOMAKE([foreign])
dnl dejagnu dist-bzip2 no-dependencies

# These are defaults to make life easier for the developers.  Normally
# you'll write something like this (as far as the libraries are not in
# a standard location):
# $ ./configure CPPFLAGS="-I/proj/contrib/lkb/include -I/proj/contrib/lib/icu/include" LDFLAGS="-L/proj/contrib/lkb/lib/linux -L/proj/contrib/lib/icu/lib"
# or alternatively
# $ ./configure --with-boost=/path/to/boost ...
# (which is not quite the same)

# Let's try putting stuff in delphin/
# ... Eric Nichols <eric-n@is.naist.jp> -- Jun. 18th, 2005
contrib_prefix="/usr/local/delphin"
#contrib_prefix="$HOME"

ITSDBDIR="$contrib_prefix/lkb"
MRSDIR="$ITSDBDIR"

# Assume that PET_DEPENDENCIES are all in "/usr/local"
# ... Eric Nichols <eric-n@is.naist.jp> -- Jun. 18th, 2005
ICUDIR=/usr/local
ECLDIR=/usr/local
XERCESCDIR=/usr/local
# default install location for postgresql and boost is /usr/lib & /usr/include
PSQLDIR=
BOOSTDIR=

# Check whether linux is 32 or 64 bit and create variable "OS" to point to
# correct library location.
# Code liberally borrowed from LKB install script ...
# Eric Nichols <eric-n@is.naist.jp> -- Jun. 18th, 2005


# next, determine the current operating system, one of `linux' (x86), `solaris'
# (sparc), or `windows' (x86); anything else will require manual installation.
#
#if [ "$OSTYPE" = "linux" -o "$OSTYPE" = "linux-gnu" ]; then
# apparently, (some) Debian installations come with an older uname(1), where
# `-i' is not available :-{.                                 (11-mar-05; oe)
#
if uname -i > /dev/null 2>&1; then
  CPU=$(uname -i)
  if test "${cpu}" = "unknown" ; then cpu=$(uname -m); fi
else
  CPU=$(uname -m)
fi
case "${CPU}" in
  i?86)
    OS="linux.x86.32"
     ;;
  x86_64)
    OS="linux.x86.64"
    ;;
  *)
    echo "lkb: unknown Linux architecture (check \`uname -m'); exit."
    exit 1;
esac
#elif [ "$OSTYPE" = "solaris" -o "${OSTYPE%%?.?}" = "solaris" ]; then
#  os="solaris";
#elif [ "$OSTYPE" = "cygwin" ]; then
#  os="windows";
#fi

# let's use the method given in LKB's install script to get the kernel version
# too ... Eric Nichols <eric-n@is.naist.jp> -- Jun. 19, 2005
KERNEL=$(uname -r)
case "${KERNEL}" in
  2.4*)
    kernel_2_4=true
    AC_DEFINE([KERNEL_2_4],[1],[Current Linux kernel is 2.4.x])
     ;;
  2.6*)
    kernel_2_6=true
    AC_DEFINE([KERNEL_2_6],[1],[Current Linux kernel is 2.6.x])
    ;;
  *)
    echo "lkb: unknown Linux kernel version (check \`uname -r'); exit."
    exit 1;
esac


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

saveCPPFLAGS="$CPPFLAGS"
saveLDFLAGS="$LDFLAGS"

# Features/packages header files and libraries need to be done with
# dependencies: only check if asked, and if not found switch off all
# related/dependent checks.
# IF requested AND libraries and headers OK; THEN add to configuration
# For libraries: it should be possible to choose for static ones

check_tsdb() {
if test -d ${ITSDBDIR}/include ; then
    CPPFLAGS="$CPPFLAGS -I${ITSDBDIR}/include"
    AC_CHECK_HEADERS([itsdb.h], [],
        [
        AC_MSG_WARN([itsdb header not found for --with-tsdb; DISABLED])
        return 1
        ], [])
else 
    AC_MSG_WARN([directory ${ITSDBDIR}/include for --with-tsdb not found; DISABLED])
    return 1
fi 

# Use OS variable to point to correct linux library
# ... Eric Nichols <eric-n@is.naist.jp> -- Jun. 18th, 2005
#if test -d ${ITSDBDIR}/lib/linux ; then
if test -d ${ITSDBDIR}/lib/${OS} ; then
    # Use OS variable to point to correct linux library ...
    # Eric Nichols <eric-n@is.naist.jp> -- Jun. 18th, 2005
    #LDFLAGS="$LDFLAGS -L${ITSDBDIR}/lib/linux"
    LDFLAGS="$LDFLAGS -L${ITSDBDIR}/lib/${OS}"
    export LDFLAGS
    AC_CHECK_LIB([itsdb], [capi_register], [ return 0 ], 
        [
        AC_MSG_WARN([itsdb library not found for --with-tsdb; DISABLED])
        return 1
        ])
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
      AC_DEFINE(TSDBAPI, [1], ["incr[tsdb()] libraries available, supply API"])
      CHEAPLIBS="-litsdb $CHEAPLIBS"
      CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${ITSDBDIR}/include"
      # Use OS variable to point to correct linux library ...
      # Eric Nichols <eric-n@is.naist.jp> -- Jun. 18th, 2005
      CHEAPLDFLAGS="$CHEAPLDFLAGS -L${ITSDBDIR}/lib/${OS} -Wl,-rpath,${ITSDBDIR}/lib/${OS}"
else
    tsdb=false
fi
AM_CONDITIONAL(TSDB, [test x$tsdb = xtrue])
CPPFLAGS="$saveCPPFLAGS"
LDFLAGS="$saveLDFLAGS"


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

check_ecl() {
eclincldir=""
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
        CPPFLAGS="$CPPFLAGS -I${eclincldir} -I${MRSDIR}/include"
        export CPPFLAGS
	# rmrs.h is no longer being distributed, so we don't need to check for
        # it any more ... Eric Nichols <eric-n@is.naist.jp> -- Jun. 18, 2005
	AC_CHECK_HEADERS([ecl.h], [],
            [ AC_MSG_WARN([header files not found for --with-eclmrs; DISABLED])
              return 1 ],[])
        LDFLAGS="$LDFLAGS -L${ECLDIR}/lib/ecl"
        export LDFLAGS
        # ECL Libraries
        AC_CHECK_LIB([ecl], [cl_map], 
            [ return 0 ], 
            [ 
            AC_MSG_WARN([libecl.so not found for --with-eclmrs; DISABLED])
            return 1
            ])
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
    AC_DEFINE(HAVE_ECL, [1], [Use ECL made MRS library])
else 
    eclmrs=false
fi
AM_CONDITIONAL(ECLMRS, [test x$eclmrs = xtrue])
dnl Substitute this to compile libmrs.a
AC_SUBST(MRSDIR)
CPPFLAGS="$saveCPPFLAGS"
LDFLAGS="$saveLDFLAGS"


check_icu() {
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
AC_CHECK_LIB([icudata], [main], [ icu=true ], 
    [ AC_MSG_WARN([icudata library not found for --with-icu; DISABLED])
        return 1 ])
dnl AC_CHECK_LIB([icuuc], [main], [ icu=true ], 
dnl     [ AC_MSG_WARN([icuuc library not found for --with-icu; DISABLED])
dnl         return 1 ])
dnl AC_CHECK_LIB([icui18n], [main], [ icu=true ], 
dnl     [ AC_MSG_WARN([icui18n library not found for --with-icu; DISABLED])
dnl           return 1 ])
return 0
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
    AC_DEFINE(HAVE_ICU, [1], [use international character library from IBM]) 
else
    icu=false
fi
AM_CONDITIONAL(ICU, [test X$icu = Xtrue])
CPPFLAGS="$saveCPPFLAGS"
LDFLAGS="$saveLDFLAGS"



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
    if test x${BOOSTDIR} != x ; then
       CPPFLAGS="$CPPFLAGS -I${BOOSTDIR}/include"
    fi
    AC_CHECK_HEADERS([boost/graph/adjacency_list.hpp boost/graph/topological_sort.hpp boost/lambda/lambda.hpp], [ boost=true ] ,[ boost=false ; break ], [])
    dnl Necessary only for test binary boost:
    dnl AC_CHECK_HEADERS([boost/graph/graph_utility.hpp boost/graph/graphviz.hpp boost/graph/reverse_graph.hpp], [ boost=true ] ,[ boost=false ; break ], [])
fi
AM_CONDITIONAL(BOOST, [test x$boost = xtrue])
if test x$boost = xtrue -a -n "${BOOSTDIR}" ; then
    FLOPCPPFLAGS="$FLOPCPPFLAGS -I${BOOSTDIR}/include"
fi
CPPFLAGS="$saveCPPFLAGS"
LDFLAGS="$saveLDFLAGS"



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
#   AC_DEFINE([DYNAMIC_SYMBOLS],[1],[Enable the use of dynamic symbols/atoms])
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
    AC_DEFINE(HAVE_XML, [1], [Enable XML input mode, using XERCES C++ library])
fi
AM_CONDITIONAL(XML, [test x$xml = xtrue])
CPPFLAGS="$saveCPPFLAGS"
LDFLAGS="$saveLDFLAGS"



check_psql() {
if test x$PSQLDIR != x ; then
    if test -d ${PSQLDIR} && test -d $PSQLDIR/include && test -d $PSQLDIR/lib ; then 
        LDFLAGS="$LDFLAGS -L${PSQLDIR}/lib"
        export LDFLAGS
        CPPFLAGS="$CPPFLAGS -I${PSQLDIR}/include"
    else 
        AC_MSG_WARN([directory ${PSQLDIR}, ${PSQLDIR}/lib or ${PSQLDIR}/include not found for --with-lexdb; DISABLED])
        return 1
    fi
fi
CPPFLAGS="$CPPFLAGS -I."
export CPPFLAGS
AC_CHECK_HEADERS([libpq-fe.h], [],
    [ AC_MSG_WARN([pq header file not found for psql lex DB; DISABLED])
      return 1
    ])
AC_CHECK_LIB([pq], [main], [],
    [ AC_MSG_WARN([pq library file not found for psql lex DB; DISABLED])
      return 1
    ])
return 0
}

AC_ARG_WITH(lexdb,
[AC_HELP_STRING([--with-lexdb@<:@=DIR@:>@], [use the postgres DB lexicon (postgres library and header files required) (default=yes)])],
[
case "${withval}" in
    yes) lexdb=true ;;
    no)  lexdb=false ;;
    *)   lexdb=true
         PSQLDIR=${withval} ;;
esac
], [ lexdb=true ])
if test x$lexdb = xtrue && check_psql ; then
   if test x$PSQLDIR != x ; then
      CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${PSQLDIR}/include"
      CHEAPLDFLAGS="$CHEAPLDFLAGS -L${PSQLDIR}/lib -Wl,-rpath,${PSQLDIR}/lib"
   fi
   CHEAPLIBS="-lpq $CHEAPLIBS"
   AC_DEFINE(HAVE_PSQL_LEXDB, [1], [use external postgres lexical database]) 
else
   lexdb=false
fi
AM_CONDITIONAL(LEXDB, [test x$lexdb = xtrue])
CPPFLAGS="$saveCPPFLAGS"
LDFLAGS="$saveLDFLAGS"

dnl What the *&^@~! is that?

AC_ARG_ENABLE(external-dictionary,
[AC_HELP_STRING([--enable-external-dictionary@<:@=DIR@:>@], [use an external dictionary (default=no)])],
[case "${enableval}" in
   yes) extdict=true ;;
   no)  extdict=false ;;
   *)   AC_MSG_ERROR([bad value ${enableval} for --enable-external-dictionary])
 esac
 if test x$extdict = xtrue ; then
    AC_DEFINE(HAVE_EXTDICT, [1], [enable external dictionary])
 fi
], [extdict=false])
AM_CONDITIONAL(EXTDICT, [test x$extdict = xtrue])


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
    AC_DEFINE(YY, [1], [Include some YY specific code])
fi
AM_CONDITIONAL(YY, [test x$yy = xtrue])

AC_ARG_ENABLE(lui,
AC_HELP_STRING([--enable-lui@<:@=ARG@:>@], [enable LUI dumping (default=no)]),
[case "${enableval}" in
   yes) lui=true ;;
   no)  lui=false ;;
   *)   lui=false
        AC_MSG_ERROR(bad value ${enableval} for --enable-lui) ;;
 esac
], [lui=false])
if test x$lui = xtrue; then 
    AC_DEFINE(LUI, [1], [Enable grammar dumping in LUI format])
fi
# no extra files, don't need Makefile conditional
# AM_CONDITIONAL(LUI, [test x$lui = xtrue])

AC_LANG_PUSH(C++)
AC_CHECK_HEADERS([hash_map ext/hash_map],,,)


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
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h locale.h malloc.h netdb.h netinet/in.h stddef.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_CHECK_SIZEOF(int *)

# Checks for library functions.
AC_SEARCH_LIBS([getopt_long_only], [iberty], [],
               [AC_MSG_ERROR([No library containing getopt_long_only()], [1])])

AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC

AC_ARG_ENABLE(mmapalloc,
[AC_HELP_STRING([--enable-mmap], [enable/disable use of mmap allocator (default=check for mmap)])],
[case "${enableval}" in
   no)   mmapalloc=no;;
   *)    mmapalloc=yes;;
 esac
], [mmapalloc=yes])
if test "$mmapalloc" = "yes" ; then
AC_FUNC_MMAP
fi

AC_ARG_ENABLE(qccomp,
[AC_HELP_STRING([--enable-qccomp], [enable/disable recording of failures for quick check computation])],
[case "${enableval}" in
   no)   qccomp=no;;
   *)    qccomp=yes;;
 esac
], [qccomp=no])
if test x$qccomp = xyes ; then
   AC_DEFINE(QC_PATH_COMP, [1], [Enable code for QuickCheck computation])
fi


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
AC_CHECK_FUNCS([bzero dup2 gethostbyaddr gettimeofday inet_ntoa setlocale socket strcasecmp strchr strdup strerror strpbrk strrchr strtol])

# Excluded doc and goofy
AC_CONFIG_FILES([Makefile
	         borland/Makefile
                 cheap/Makefile
		 cheap/Jamfile
                 common/Makefile
                 flop/Makefile
		 flop/Jamfile])

feature_avail() {
(
  if test x$1 != x$2 ; then foo="NOT " ; fi
  echo "$3 is ${foo}available"
)
}
echo ; echo "============================================================="
feature_avail x$boost xtrue "Boost (required for flop)"
feature_avail x$tsdb xtrue "tsdb"
feature_avail x$eclmrs xtrue "MRS code with ecl"
feature_avail x$icu xtrue "ICU"
feature_avail x$xml xtrue "XML input mode"
feature_avail x$lexdb xtrue "External SQL dictionary"
feature_avail x$mmapalloc xyes "Memory allocation with mmap"
feature_avail x$qccomp xyes "Code for quick-check computation"
echo "=============================================================" ; echo


AC_OUTPUT
