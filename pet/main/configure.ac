# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT([PET], [0.99.14svn], [kiefer@dfki.de])
AC_REVISION($LastChangedRevision$)
AC_CONFIG_SRCDIR([pet-config.h.in])
AC_CONFIG_HEADERS([pet-config.h])

AM_INIT_AUTOMAKE([foreign])
dnl dejagnu dist-bzip2 no-dependencies

# Let's try putting stuff in delphin/
# ... Eric Nichols <eric-n@is.naist.jp> -- Jun. 18th, 2005
if test -d "/usr/local/delphin" ; then
   delphin_prefix="/usr/local/delphin"
else
   delphin_prefix="$HOME/delphin"
fi

ITSDBDIR="$delphin_prefix/lkb"

# Assume that the following packages are all in system default locations
ICUDIR=
ECLDIR=
XERCESCDIR=
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
  if test "${CPU}" = "unknown" ; then CPU=$(uname -m); fi
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
  Power?Macintosh)
    OS="linux.ppc"
    ;;
  *)
    echo "unknown Linux architecture $CPU (check \`uname -m');"
    OS="UNKNOWN"
    ;;
esac
#elif [ "$OSTYPE" = "solaris" -o "${OSTYPE%%?.?}" = "solaris" ]; then
#  os="solaris";
#elif [ "$OSTYPE" = "cygwin" ]; then
#  os="windows";
#fi

# Linux and SunOS have different flags for the dynlib rpath mechanism
case $(uname -s) in
  ( linux | Linux)
    rpath="-Wl,-rpath,";;
  SunOS)
    rpath="-R";;
  Darwin)
    rpath="";;
esac

addlibdir() {
  res="$1 -L$2";
  if test x$rpath != x ; then res="$res $rpath$2" ; fi
  echo $res;
}

case $(uname -s) in
   CYGWIN* )
     AC_DEFINE(PATH_SEP,"\","directory path separator char") ;;
   * )
     AC_DEFINE(PATH_SEP,"/","directory path separator char") ;;
esac

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
    echo "unknown Linux kernel version (check \`uname -r');"
    kernel_2_6=true
    ;;
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

AC_LANG_PUSH(C++)

AC_ARG_WITH(boost,
[AC_HELP_STRING([--with-boost=DIR], [use the BOOST library in DIR])],
[
if test -d "${withval}"; then 
    BOOSTDIR=${withval}
else
    AC_MSG_ERROR(directory ${withval} not found for --with-boost)
fi
], [])
if test x${BOOSTDIR} != x ; then
    CPPFLAGS="$CPPFLAGS -I${BOOSTDIR}/include"
fi
AC_CHECK_HEADERS([boost/graph/adjacency_list.hpp boost/graph/topological_sort.hpp boost/lambda/lambda.hpp], [] ,
[ AC_MSG_ERROR([boost graph library required to build flop and cheap]) ], [])
dnl Necessary only for test binary boost:
dnl AC_CHECK_HEADERS([boost/graph/graph_utility.hpp boost/graph/graphviz.hpp boost/graph/reverse_graph.hpp], [ boost=true ] ,
dnl [ AC_MSG_ERROR([boost binary library required to build testBoost]) ], [])
if test -n "${BOOSTDIR}" ; then
    FLOPCPPFLAGS="$FLOPCPPFLAGS -I${BOOSTDIR}/include"
    CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${BOOSTDIR}/include"
fi
CPPFLAGS="$saveCPPFLAGS"
LDFLAGS="$saveLDFLAGS"

# Features/packages header files and libraries need to be done with
# dependencies: only check if asked, and if not found switch off all
# related/dependent checks.
# IF requested AND libraries and headers OK; THEN add to configuration
# For libraries: it should be possible to choose for static ones

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

if test x$icu = xtrue && check_icu ; then
    if test x$ICUDIR != x ; then
        CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${ICUDIR}/include"
        CHEAPLDFLAGS=`addlibdir "$CHEAPLDFLAGS" "${ICUDIR}/lib"`
    fi
    CHEAPLIBS="-lpthread -licuuc -licui18n -licudata $CHEAPLIBS"
    AC_DEFINE(HAVE_ICU, [1], [use international character library from IBM]) 
else
    icu=false
fi
AM_CONDITIONAL(ICU, [test X$icu = Xtrue])
CPPFLAGS="$saveCPPFLAGS"
LDFLAGS="$saveLDFLAGS"

AC_ARG_WITH(xml,
[AC_HELP_STRING([--with-xml@<:@=DIR@:>@], [use the Xerces C++ library (enables XML input modes) (default=no)])],
[
case "${withval}" in
    yes) xml=true ;;
    no)  xml=false ;;
    *)   xml=true
            XERCESCDIR=${withval} ;;
esac
save_LD="$LDFLAGS"
if test x$xml = xtrue -a -n "${XERCESCDIR}" ; then
   if test -d "${XERCESCDIR}" ; then
      LDFLAGS="$LDFLAGS -L${XERCESCDIR}/lib -licuuc -licudata"
   else 
      AC_MSG_WARN([directory ${XERCESCDIR} not found for --with-xml; DISABLED])
      xml=false
   fi
fi
if test x$xml = xtrue ; then
    AC_CHECK_LIB([xerces-c], [_init], [ xml=true ],
        [
        xml=false
        AC_MSG_WARN([Xerces C++ library not found for --with-xml; DISABLED])
        ])
else
    xml=false
fi
LDFLAGS="$save_LD"
], [xml=false])
if test x$xml = xtrue ; then
    CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${XERCESCDIR}/include"
    CHEAPLDFLAGS=`addlibdir "$CHEAPLDFLAGS" "${XERCESCDIR}/lib"`
    CHEAPLIBS="-lxerces-c $CHEAPLIBS"
    AC_DEFINE(HAVE_XML, [1], [Enable XML input modes, using Xerces C++ library])
fi
AM_CONDITIONAL(XML, [test x$xml = xtrue])
CPPFLAGS="$saveCPPFLAGS"
LDFLAGS="$saveLDFLAGS"


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

if test x$tsdb = xtrue && check_tsdb ; then 
      AC_DEFINE(TSDBAPI, [1], ["incr[tsdb()] libraries available, supply API"])
      CHEAPLIBS="-litsdb $CHEAPLIBS"
      CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${ITSDBDIR}/include"
      # Use OS variable to point to correct linux library ...
      # Eric Nichols <eric-n@is.naist.jp> -- Jun. 18th, 2005
      CHEAPLDFLAGS=`addlibdir "$CHEAPLDFLAGS" "${ITSDBDIR}/lib/${OS}"`
else
    tsdb=false
fi
AM_CONDITIONAL(TSDB, [test x$tsdb = xtrue])
CPPFLAGS="$saveCPPFLAGS"
LDFLAGS="$saveLDFLAGS"


MRSDIR="$ITSDBDIR"
dnl check for the Lisp code directory for MRS
AC_ARG_WITH(mrs,
[AC_HELP_STRING([--with-mrs@<:@=DIR@:>@], 
                [specify the path to the MRS Lisp sources (usually the LKB top level directory) (default=${ITSDBDIR})])],
[ dnl the command line switch was given, eventually check provided MRSDIR
case "${withval}" in
    yes) mrs=true ;;
    no)  mrs=false ;;
    *)   mrs=true
         MRSDIR=${withval};;
esac
], [mrs=true])
if test "x$mrs" = xtrue ; then
  if test \! -f "${MRSDIR}/src/general/loadup.lisp" ; then
    if test -f "${withval}/general/loadup.lisp" ; then
       MRSDIR=${withval%%/}
       MRSDIR=${MRSDIR%%/src}
    fi
  fi
  if test \! -f "${MRSDIR}/src/general/loadup.lisp" ; then
     AC_MSG_WARN([no MRS code found in ${MRSDIR}; DISABLED])
     MRSDIR=""
     mrs=false
  else
     mrs=true
  fi
else
  MRSDIR=""
fi

PREPROCDIR="${ITSDBDIR}"
dnl check the Lisp code directory for the preprocessor
AC_ARG_WITH(preproc,
[AC_HELP_STRING([--with-preproc@<:@=DIR@:>@], [specify the path to the x-preprocessor Lisp sources (usually the LKB top level directory) (default=${ITSDBDIR})\])],
dnl (default=no)
[
dnl AC_MSG_WARN([PREPROC WITHVAL = ${withval}])
case "${withval}" in
    yes) preproc=true ;;
    no)  preproc=false ; echo "Disabling preprocessor code" ;;
    *)   preproc=true
         PREPROCDIR=${withval};;
esac
], [ preproc=true ])
if test x$preproc = xtrue ; then
  if test \! -f "${PREPROCDIR}/src/general/loadup.lisp" ; then
     if test -f "${PREPROCDIR}/general/loadup.lisp" ; then
        PREPROCDIR=${PREPROCDIR%%/}
        PREPROCDIR=${PREPROCDIR%%/src}
     else
        AC_MSG_WARN([no PREPROC code found in ${PREPROCDIR} for --with-preprocdir])
        PREPROCDIR=""
        preproc=false
     fi
  fi
else
  PREPROCDIR=""
fi


AC_ARG_WITH(ecl,
[AC_HELP_STRING([--with-ecl@<:@=DIR@:>@], 
                [use ECL for mrs or preprocessing or both (default=yes)])],
[
case "${withval}" in
    yes) ecl=true ;;
    no)  ecl=false ;;
    *)   ecl=true
         ECLDIR=${withval} ;;
esac
], [ ecl=true ])

eclincldir=""
if test x$ecl = xtrue; then
  dnl check ECL: binary, include dir, libs
  AC_PATH_PROG(ECL, ecl, no, [$PATH:${ECLDIR}/bin])
  if test $ECL = no ; then 
    AC_MSG_WARN([no ecl binary found in ${ECLDIR}/bin; ECL DISABLED])
    ecl=false
  else
    if test -z "${ECLDIR}" ; then
       ECLDIR=`dirname $ECL`
       ECLDIR=${ECLDIR%%/bin}
    fi
    if test -d ${ECLDIR}/h ; then
      eclincl="-I${ECLDIR}/h"
    fi
    if test -z "$eclincl" -a -d ${ECLDIR}/lib/ecl/h ; then
      eclincl="-I${ECLDIR}/lib/ecl/h"
    fi
    if test -z "$eclincl" -a -d ${ECLDIR}/lib/ecl/ecl ; then
      eclincl="-I${ECLDIR}/lib/ecl -I${ECLDIR}/lib/ecl/ecl"
    fi
    if test -z "$eclincl" ; then
      AC_MSG_WARN([in ${ECLDIR}/lib, neither ./h nor ./ecl/h nor ./ecl/ecl found; ECL DISABLED])
      ecl=false
    else
      CPPFLAGS="$CPPFLAGS ${eclincl}"
      export CPPFLAGS
      AC_CHECK_HEADERS([ecl.h], [],
         [ AC_MSG_WARN([header files not found: ${eclincl}; ECL DISABLED])
           ecl=false 
         ],[])
    fi
  fi
fi
if test "x$ecl" == xtrue ; then
   LDFLAGS="$LDFLAGS -L${ECLDIR}/lib/ecl"
   export LDFLAGS
   # check ECL Libraries
   AC_CHECK_LIB([ecl], [cl_map], [ ecl=true ], 
        [ AC_MSG_WARN([libecl.so not found in ${ECLDIR}/lib/ecl; ECL DISABLED])
          ecl=false
        ])
fi

dnl all tests for ECL components have been executed, draw conclusions
if test x$ecl = xtrue ; then
  CHEAPLIBS="-lecl $CHEAPLIBS"
  CHEAPLDFLAGS=`addlibdir "$CHEAPLDFLAGS" "${ECLDIR}/lib/ecl"`
  CHEAPCPPFLAGS="$CHEAPCPPFLAGS ${eclincl}"
  if test x$mrs == xtrue ; then
     AC_DEFINE(HAVE_MRS, [1], [use ECL MRS library])
     ECLUSER="_mrs"
  fi
  if test x$preproc == xtrue; then
     AC_DEFINE(HAVE_PREPROC, [1], [use ECL x-preprocessing library])
     ECLUSER=${ECLUSER}"_preproc"
  fi
  if test x$ECLUSER != x ; then
     AC_DEFINE(HAVE_ECL, [1], [Use ECL for MRS or preprocessing library])
  else
     ecl=false
  fi
else
  preproc=false
  mrs=false    
fi
AM_CONDITIONAL(ECL, [test x$ECLUSER != x])
dnl Substitute this to compile libmrs.a
AM_CONDITIONAL(ECLMRS, [test x$mrs = xtrue])
AM_CONDITIONAL(ECLPREPROC, [test x$preproc = xtrue])

AC_SUBST(MRSDIR)
AC_SUBST(PREPROCDIR)
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


dnl external lexicon based on postgresql database

dnl AC_ARG_WITH(lexdb,
dnl [AC_HELP_STRING([--with-lexdb@<:@=DIR@:>@], [use the postgres DB lexicon (postgres library and header files required) (default=yes)])],
dnl [
dnl case "${withval}" in
dnl     yes) lexdb=true ;;
dnl     no)  lexdb=false ;;
dnl     *)   lexdb=true
dnl          PSQLDIR=${withval} ;;
dnl esac
dnl ], [ lexdb=true ])

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

dnl if test x$lexdb = xtrue && check_psql ; then
dnl    if test x$PSQLDIR != x ; then
dnl       CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${PSQLDIR}/include"
dnl       CHEAPLDFLAGS=`addlibdir "$CHEAPLDFLAGS" "${PSQLDIR}/lib"`
dnl    fi
dnl    CHEAPLIBS="-lpq $CHEAPLIBS"
dnl    AC_DEFINE(HAVE_PSQL_LEXDB, [1], [use external postgres lexical database]) 
dnl else
dnl    lexdb=false
dnl fi
dnl AM_CONDITIONAL(LEXDB, [test x$lexdb = xtrue])
dnl CPPFLAGS="$saveCPPFLAGS"
dnl LDFLAGS="$saveLDFLAGS"


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

AC_ARG_ENABLE(instfc,
AC_HELP_STRING([--enable-instfc@<:@=ARG@:>@], [use the instantiation failure cache (default=yes)]),
[case "${enableval}" in
   yes) instfc=true ;;
   no)  instfc=false ;;
   *)   instfc=false
        AC_MSG_ERROR(bad value ${enableval} for --enable-instfc) ;;
 esac
], [instfc=true])
if test x$instfc = xtrue; then 
    AC_DEFINE(INSTFC, [1], [Include instantiation failure cache code])
fi
AM_CONDITIONAL(INSTFC, [test x$instfc = xtrue])

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

dnl What the *&^@~! is that?
dnl AC_ARG_ENABLE(external-dictionary,
dnl [AC_HELP_STRING([--enable-external-dictionary@<:@=DIR@:>@], [use an external dictionary (default=no)])],
dnl [case "${enableval}" in
dnl    yes) extdict=true ;;
dnl    no)  extdict=false ;;
dnl    *)   AC_MSG_ERROR([bad value ${enableval} for --enable-external-dictionary])
dnl  esac
dnl  if test x$extdict = xtrue ; then
dnl     AC_DEFINE(HAVE_EXTDICT, [1], [enable external dictionary])
dnl  fi
dnl ], [extdict=false])
dnl AM_CONDITIONAL(EXTDICT, [test x$extdict = xtrue])

dnl robust processing code by frederik fouvry, not supported at the moment
dnl AC_ARG_ENABLE(rcu,
dnl [AC_HELP_STRING([--enable-rcu@<:@=ARG@:>@], [use the RCU extensions (default=no)])],
dnl [case "${enableval}" in
dnl    yes) rcu=true ;;
dnl    no)  rcu=false ;;
dnl    *)   AC_MSG_ERROR([bad value ${enableval} for --enable-rcu])
dnl esac], [rcu=false])
dnl AM_CONDITIONAL(RCU, [test x$rcu = xtrue])

AC_CHECK_HEADERS([hash_map],,
   [AC_CHECK_HEADERS([ext/hash_map],,[AC_MSG_ERROR("No hash_map header found")],)])


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
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h locale.h netdb.h netinet/in.h],,
    [AC_MSG_ERROR("some system header file not found")])
AC_CHECK_HEADERS([stddef.h stdlib.h string.h strings.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h unistd.h],,
    [AC_MSG_ERROR("some system header file not found")])

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
[AC_HELP_STRING([--enable-mmapalloc], [enable/disable use of mmap allocator (default=check for mmap)])],
[case "${enableval}" in
   yes) mmapalloc=yes;;
   *)   mmapalloc=no;;
 esac
], [mmapalloc=no])
if test "$mmapalloc" = "yes" ; then
AC_FUNC_MMAP
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
		 fspp/Makefile
		 fspp/Jamfile
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
dnl feature_avail x$boost xtrue "Boost (required for flop)"
feature_avail x$tsdb xtrue "tsdb"
feature_avail x$ecl xtrue "ecl lisp processor"
feature_avail x$mrs xtrue "MRS code with ecl"
feature_avail x$preproc xtrue "preprocessor code with ecl"
feature_avail x$icu xtrue "ICU"
feature_avail x$xml xtrue "Xerces C++ (for XML input modes)"
dnl feature_avail x$lexdb xtrue "External SQL dictionary"
feature_avail x$mmapalloc xyes "Memory allocation with mmap"
echo "=============================================================" ; echo


AC_OUTPUT
