# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT([PET], [0.99.14transfer], [Peter.Adolphs@dfki.de])
AC_REVISION($LastChangedRevision$)
AC_CONFIG_SRCDIR([pet-config.h.in])
AC_CONFIG_HEADERS([pet-config.h])

AM_INIT_AUTOMAKE([1.7 foreign])
dnl dejagnu dist-bzip2 no-dependencies



# --------------------------------------------------------------------
# Checks for programs.
# --------------------------------------------------------------------

AC_PROG_CXX
AC_PROG_CPP
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_GREP
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Check for doxygen to generate documentation
AC_PATH_PROG([DOXYGEN],[doxygen])
AM_CONDITIONAL([DOCBUILDERS], [ test "$DOXYGEN" != "" ])
if test "$DOXYGEN" == "" ; then
    AC_WARN([Cannot find doxygen. Disabling API doc targets.])
fi



# --------------------------------------------------------------------
# Checks for libraries.
# --------------------------------------------------------------------

# search a lib that provides getopt_long_only (lib is added to LIBS if found)
AC_SEARCH_LIBS([getopt_long_only], [iberty], [],
               [AC_MSG_ERROR([No library containing getopt_long_only()], [1])])

# the following are checked elsewhere
#AC_CHECK_LIB([dl], [main])
#AC_CHECK_LIB([iberty], [main])
# AC_CHECK_LIB([itsdb], [main])
#AC_CHECK_LIB([m], [main])
#AC_CHECK_LIB([pvm3], [main])
# AC_CHECK_LIB([gmp], [main])
# AC_CHECK_LIB([mrs], [main])



# --------------------------------------------------------------------
# Checks for header files.
# --------------------------------------------------------------------

# enable assertions; introduces the `--disable-assert' option
AC_HEADER_ASSERT

# defines HAVE_STDBOOL_H and HAVE__BOOL (if there is the type _Bool)
AC_HEADER_STDBOOL

# defines STDC_HEADERS if there are C header files conforming to ANSI C89
AC_HEADER_STDC

# defines HAVE_SYS_WAIT_H if there is a Posix-compatible `sys/wait.h' 
AC_HEADER_SYS_WAIT

# defines TIME_WITH_SYS_TIME if three are `time.h' and `sys/time.h'
AC_HEADER_TIME

# check for the presence of certain headers and raise an error if not
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h locale.h netdb.h netinet/in.h],,
                 [AC_MSG_ERROR("some system header file not found")])
AC_CHECK_HEADERS([sys/ioctl.h sys/param.h sys/socket.h sys/time.h unistd.h],,
                 [AC_MSG_ERROR("some system header file not found")])
AC_CHECK_HEADERS([stddef.h stdlib.h string.h strings.h],,
                 [AC_MSG_ERROR("some system header file not found")])
AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([hash_map],,
   [AC_CHECK_HEADERS([ext/hash_map],,[AC_MSG_ERROR("No hash_map header found")],)])
AC_LANG_POP([C++])



# --------------------------------------------------------------------
# Checks for typedefs, structures, and compiler characteristics.
# --------------------------------------------------------------------

AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_CHECK_SIZEOF(int *)



# --------------------------------------------------------------------
# Checks for library functions.
# --------------------------------------------------------------------

AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SETPGRP
AC_FUNC_SETVBUF_REVERSED
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
# Is becoming obsolete
AC_FUNC_WAIT3
AC_CHECK_FUNCS([bzero dup2 gethostbyaddr gettimeofday inet_ntoa setlocale socket])
AC_CHECK_FUNCS([strcasecmp strchr strdup strerror strpbrk strrchr strtol])



# --------------------------------------------------------------------
# path settings
# --------------------------------------------------------------------

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



# --------------------------------------------------------------------
# icu library.
# --------------------------------------------------------------------

AC_ARG_WITH(icu,
  [AC_HELP_STRING([--with-icu@<:@=DIR@:>@], [use the ICU library (default:check)])],
  [ case "${withval}" in
      yes) icu=yes ;;
      no)  icu=no ;;
      *)   icu=yes
           ICUDIR=${withval} ;;
    esac],
  [ icu=yes ])

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
  AC_CHECK_LIB([pthread], [pthread_exit], [ icu=yes ],
      [ AC_MSG_WARN([pthread library not found for --with-icu; DISABLED])
          return 1 ])
  AC_CHECK_LIB([icudata], [main], [ icu=yes ], 
      [ AC_MSG_WARN([icudata library not found for --with-icu; DISABLED])
          return 1 ])
  dnl AC_CHECK_LIB([icuuc], [main], [ icu=yes ], 
  dnl     [ AC_MSG_WARN([icuuc library not found for --with-icu; DISABLED])
  dnl         return 1 ])
  dnl AC_CHECK_LIB([icui18n], [main], [ icu=yes ], 
  dnl     [ AC_MSG_WARN([icui18n library not found for --with-icu; DISABLED])
  dnl           return 1 ])
  return 0
}

if test "$icu" = "yes" && check_icu ; then
    if test x$ICUDIR != x ; then
        CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${ICUDIR}/include"
        CHEAPLDFLAGS=`addlibdir "$CHEAPLDFLAGS" "${ICUDIR}/lib"`
    fi
    CHEAPLIBS="-lpthread -licuuc -licui18n -licudata $CHEAPLIBS"
    AC_DEFINE(HAVE_ICU, [1], [use international character library from IBM]) 
else
    icu=no
fi
AM_CONDITIONAL(ICU, [test "$icu" = "yes"])
CPPFLAGS="$saveCPPFLAGS"
LDFLAGS="$saveLDFLAGS"



# --------------------------------------------------------------------
# boost library.
# --------------------------------------------------------------------

AX_BOOST_BASE([1.32], [boost=yes], [boost=no])
if test "$boost" = "yes" ; then
  CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
  AC_LANG_PUSH([C++])
  AC_CHECK_HEADERS([boost/graph/adjacency_list.hpp \
                    boost/graph/topological_sort.hpp \
                    boost/lambda/lambda.hpp ],
                   [],
                   [ boost=no ; break ],
                   [])
  AC_LANG_POP([C++])
  CPPFLAGS="$saveCPPFLAGS"
  LDFLAGS="$saveLDFLAGS"
fi
if test "$boost" = "yes" ; then
  if echo " ${FLOPCPPFLAGS} " | $GREP -q -v " ${BOOST_CPPFLAGS} " ; then
    FLOPCPPFLAGS="${FLOPCPPFLAGS} ${BOOST_CPPFLAGS}"
  fi
fi
AM_CONDITIONAL(BOOST, [test "$boost" = "yes"])



# --------------------------------------------------------------------
# XML support, i.e. Xerces C++ library.
# --------------------------------------------------------------------

AC_ARG_WITH(xml,
[AC_HELP_STRING([--with-xml@<:@=DIR@:>@], [use the Xerces C++ library (enables XML input modes) (default: no)])],
[
case "${withval}" in
    yes) xml=yes ;;
    no)  xml=no ;;
    *)   xml=yes
            XERCESCDIR=${withval} ;;
esac
save_LD="$LDFLAGS"
if test "$xml" = "yes" -a -n "${XERCESCDIR}" ; then
   if test -d "${XERCESCDIR}" ; then
      LDFLAGS="$LDFLAGS -L${XERCESCDIR}/lib -licuuc -licudata"
   else 
      AC_MSG_WARN([directory ${XERCESCDIR} not found for --with-xml; DISABLED])
      xml=no
   fi
fi
if test "$xml" = "yes" ; then
    AC_CHECK_LIB([xerces-c], [_init], [ xml=yes ],
        [
        xml=no
        AC_MSG_WARN([Xerces C++ library not found for --with-xml; DISABLED])
        ])
else
    xml=no
fi
LDFLAGS="$save_LD"
], [xml=no])
if test "$xml" = "yes" ; then
    CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${XERCESCDIR}/include"
    CHEAPLDFLAGS=`addlibdir "$CHEAPLDFLAGS" "${XERCESCDIR}/lib"`
    CHEAPLIBS="-lxerces-c $CHEAPLIBS"
    AC_DEFINE(HAVE_XML, [1], [Enable XML input modes, using Xerces C++ library])
fi
AM_CONDITIONAL(XML, [test "$xml" = "yes"])
CPPFLAGS="$saveCPPFLAGS"
LDFLAGS="$saveLDFLAGS"



# --------------------------------------------------------------------
# TSDB library.
# --------------------------------------------------------------------

AC_ARG_WITH(tsdb,
[AC_HELP_STRING([--with-tsdb@<:@=DIR@:>@], [use the TSDB library (default:check)])],
[
case "${withval}" in
    yes) tsdb=yes ;;
    no)  tsdb=no ;;
    *)   tsdb=yes
         ITSDBDIR=${withval};;
esac
], [tsdb=yes])

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

if test "$tsdb" = "yes" && check_tsdb ; then 
      AC_DEFINE(TSDBAPI, [1], ["incr[tsdb()] libraries available, supply API"])
      CHEAPLIBS="-litsdb $CHEAPLIBS"
      CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${ITSDBDIR}/include"
      # Use OS variable to point to correct linux library ...
      # Eric Nichols <eric-n@is.naist.jp> -- Jun. 18th, 2005
      CHEAPLDFLAGS=`addlibdir "$CHEAPLDFLAGS" "${ITSDBDIR}/lib/${OS}"`
else
    tsdb=no
fi
AM_CONDITIONAL(TSDB, [test "$tsdb" = "yes"])
CPPFLAGS="$saveCPPFLAGS"
LDFLAGS="$saveLDFLAGS"



# --------------------------------------------------------------------
# Lisp code for MRS.
# --------------------------------------------------------------------

MRSDIR="$ITSDBDIR"
dnl check for the Lisp code directory for MRS
AC_ARG_WITH(mrs,
[AC_HELP_STRING([--with-mrs@<:@=DIR@:>@], 
                [specify the path to the MRS Lisp sources (usually the LKB top level directory) (default=${ITSDBDIR})])],
[ dnl the command line switch was given, eventually check provided MRSDIR
case "${withval}" in
    yes) mrs=yes ;;
    no)  mrs=no ;;
    *)   mrs=yes
         MRSDIR=${withval};;
esac
], [mrs=yes])
if test ""$mrs"" = "yes" ; then
  if test \! -f "${MRSDIR}/src/general/loadup.lisp" ; then
    if test -f "${withval}/general/loadup.lisp" ; then
       MRSDIR=${withval%%/}
       MRSDIR=${MRSDIR%%/src}
    fi
  fi
  if test \! -f "${MRSDIR}/src/general/loadup.lisp" ; then
     AC_MSG_WARN([no MRS code found in ${MRSDIR}; DISABLED])
     MRSDIR=""
     mrs=no
  else
     mrs=yes
  fi
else
  MRSDIR=""
fi

PREPROCDIR="${ITSDBDIR}"
dnl check the Lisp code directory for the preprocessor
AC_ARG_WITH(preproc,
[AC_HELP_STRING([--with-preproc@<:@=DIR@:>@], [specify the path to the x-preprocessor Lisp sources (usually the LKB top level directory) (default=${ITSDBDIR})])],
dnl (default=no)
[
dnl AC_MSG_WARN([PREPROC WITHVAL = ${withval}])
case "${withval}" in
    yes) preproc=yes ;;
    no)  preproc=no ; echo "Disabling preprocessor code" ;;
    *)   preproc=yes
         PREPROCDIR=${withval};;
esac
], [ preproc=yes ])
if test "$preproc" = "yes" ; then
  if test \! -f "${PREPROCDIR}/src/general/loadup.lisp" ; then
     if test -f "${PREPROCDIR}/general/loadup.lisp" ; then
        PREPROCDIR=${PREPROCDIR%%/}
        PREPROCDIR=${PREPROCDIR%%/src}
     else
        AC_MSG_WARN([no PREPROC code found in ${PREPROCDIR} for --with-preprocdir])
        PREPROCDIR=""
        preproc=no
     fi
  fi
else
  PREPROCDIR=""
fi



# --------------------------------------------------------------------
# ECL (Embeddable Common Lisp).
# --------------------------------------------------------------------

AC_ARG_WITH(ecl,
[AC_HELP_STRING([--with-ecl@<:@=DIR@:>@], 
                [use ECL for mrs or preprocessing or both (default:check)])],
[
case "${withval}" in
    yes) ecl=yes ;;
    no)  ecl=no ;;
    *)   ecl=yes
         ECLDIR=${withval} ;;
esac
], [ ecl=yes ])

eclincldir=""
if test "$ecl" = "yes"; then
  # check ECL: binary, include dir, libs
  AC_PATH_PROG(ECL, ecl, no, [${ECLDIR}/bin:$PATH])
  if test $ECL = no ; then 
    AC_MSG_WARN([no ecl binary found in ${ECLDIR}/bin; ECL DISABLED])
    ecl=no
  else
    if test -z "${ECLDIR}" ; then
       ECLDIR=`dirname $ECL`
       ECLDIR=${ECLDIR%%/bin}
    fi
    if test -f ${ECLDIR}/h/ecl.h ; then
      eclincl="-I${ECLDIR}/h"
    fi
    if test -z "$eclincl" -a -f ${ECLDIR}/lib/ecl/h/ecl.h ; then
      eclincl="-I${ECLDIR}/lib/ecl/h"
    fi
    if test -z "$eclincl" -a -f ${ECLDIR}/lib/ecl/ecl/ecl.h ; then
      eclincl="-I${ECLDIR}/lib/ecl -I${ECLDIR}/lib/ecl/ecl -I/usr/lib/ecl"
    fi
    if test -z "$eclincl" ; then
      AC_MSG_WARN([in ${ECLDIR}/lib, neither ./h nor ./ecl/h nor ./ecl/ecl found; ECL DISABLED])
      ecl=no
    else
      export CPPFLAGS="$CPPFLAGS ${eclincl}"
      AC_CHECK_HEADERS([ecl.h], [],
         [ AC_MSG_WARN([header files not found: ${eclincl}; ECL DISABLED])
           ecl=no 
         ],[])
    fi
  fi
fi
if test "$ecl" == "yes" ; then
   LDFLAGS="$LDFLAGS -L${ECLDIR}/lib/ecl -lpthread -ldl -lm "
   export LDFLAGS
   # check ECL Libraries
   AC_CHECK_LIB([ecl], [cl_map], [ ecl=yes ], 
        [ AC_MSG_WARN([libecl.so not found in ${ECLDIR}/lib/ecl; ECL DISABLED])
          ecl=no
        ])
fi
# check whether ECL provides the necessary functions (functions have changed
# between 0.9h and 0.9i):
# TODO: a proper solution should check the older and newer functions in one
#       step and should use the correct version depending on the HAVE_FUNCTIONs
#       but since these functions are not the only problem with ecl 0.9i, I
#       have not done this.... (Peter Adolphs, Jul 24, 2007)
if test "$ecl" = "yes" ; then
  LDFLAGS="$LDFLAGS -lecl"
  AC_CHECK_FUNCS([make_string_copy make_simple_string aref1],[ecl=yes],[ecl=no])
  AC_CHECK_FUNCS([make_base_string_copy make_simple_base_string ecl_aref1],
                 [ecl=no; ecl09i=yes], [ecl09i=no])
fi
if test "$ecl" = "no" -a "$ecl09i" = "yes" ; then
  AC_MSG_WARN([you seem to have ECL 0.9i, which is currently not supported;
               please use ECL 0.9h or earlier; ECL DISABLED])
fi

dnl all tests for ECL components have been executed, draw conclusions
if test "$ecl" = "yes" ; then
  CHEAPLIBS="-lecl $CHEAPLIBS"
  CHEAPLDFLAGS=`addlibdir "$CHEAPLDFLAGS" "${ECLDIR}/lib/ecl"`
  CHEAPCPPFLAGS="$CHEAPCPPFLAGS ${eclincl}"
  if test "$mrs" = "yes" ; then
     AC_DEFINE(HAVE_MRS, [1], [use ECL MRS library])
     ECLUSER="_mrs"
  fi
  if test "$preproc" = "yes"; then
     AC_DEFINE(HAVE_PREPROC, [1], [use ECL x-preprocessing library])
     ECLUSER=${ECLUSER}"_preproc"
  fi
  if test x$ECLUSER != x ; then
     AC_DEFINE(HAVE_ECL, [1], [Use ECL for MRS or preprocessing library])
  else
     ecl=no
  fi
else
  preproc=no
  mrs=no    
fi
AM_CONDITIONAL(ECL, [test -n "$ECLUSER"])
dnl Substitute this to compile libmrs.a
AM_CONDITIONAL(ECLMRS, [test "$mrs" = "yes"])
AM_CONDITIONAL(ECLPREPROC, [test "$preproc" = "yes"])

AC_SUBST(MRSDIR)
AC_SUBST(PREPROCDIR)
CPPFLAGS="$saveCPPFLAGS"
LDFLAGS="$saveLDFLAGS"



# --------------------------------------------------------------------
# Unit testing with CppUnit
# --------------------------------------------------------------------

# do not build unit tests using cppunit by default:
cppunit="no"
# check whether user requests the use of cppunit:
AC_ARG_WITH(cppunit,
    [AC_HELP_STRING([--with-cppunit], 
                     [use the CppUnit library (enables unit tests)])],
    [AS_IF([test "$withval" = "yes" -o "$withval" = "no"],
           [cppunit=$withval],
           [AC_MSG_ERROR([Bad value for --with-cppunit
                          (allowed values: yes or no).
                          Set CppUnit paths with --with-cppunit-prefix and
                          --with-cppunit-exec-prefix.])])])
# if the user wants cppunit, look for a suitable installation:
AS_IF([test "$cppunit" = "yes"],
      [AM_PATH_CPPUNIT([1.12.0],
            [AC_DEFINE(HAVE_CPPUNIT, [1], [define if CppUnit is available])],
            [cppunit="no"
             AC_MSG_WARN([--with-unitcpp given but test for CppUnit failed])])])
# define an automake conditional whether cppunit is requested *and* present:
AM_CONDITIONAL(CPPUNIT, [test "$cppunit" = "yes"])



# --------------------------------------------------------------------
# Choice of the unifier
# --------------------------------------------------------------------

# Not via defines in config.h because there is no choice for flop!
AC_ARG_ENABLE(unifier,
AC_HELP_STRING([--enable-unifier=ARG],
               [choice of the unifier @<:@ARG=tomabechi@:>@.
                Possible values are: tomabechi, simple]),
[case "${enableval}" in
   tomabechi) unifier=tomabechi ;;
   simple)    unifier=simple ;;
   *)         AC_MSG_ERROR(bad value ${enableval} for --with-unifier)
esac], [unifier=tomabechi])
# Should be done differently: now they are independent
AM_CONDITIONAL(TOMABECHI_UNIFIER, [test x$unifier = xtomabechi])
AM_CONDITIONAL(SIMPLE_UNIFIER, [test x$unifier = xsimple])



# --------------------------------------------------------------------
# external lexicon based on postgresql database
# --------------------------------------------------------------------

dnl AC_ARG_WITH(lexdb,
dnl [AC_HELP_STRING([--with-lexdb@<:@=DIR@:>@], [use the postgres DB lexicon (postgres library and header files required) (default=yes)])],
dnl [
dnl case "${withval}" in
dnl     yes) lexdb=yes ;;
dnl     no)  lexdb=no ;;
dnl     *)   lexdb=yes
dnl          PSQLDIR=${withval} ;;
dnl esac
dnl ], [ lexdb=yes ])

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

dnl if test "$lexdb" = "yes" && check_psql ; then
dnl    if test x$PSQLDIR != x ; then
dnl       CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${PSQLDIR}/include"
dnl       CHEAPLDFLAGS=`addlibdir "$CHEAPLDFLAGS" "${PSQLDIR}/lib"`
dnl    fi
dnl    CHEAPLIBS="-lpq $CHEAPLIBS"
dnl    AC_DEFINE(HAVE_PSQL_LEXDB, [1], [use external postgres lexical database]) 
dnl else
dnl    lexdb=no
dnl fi
dnl AM_CONDITIONAL(LEXDB, [test "$lexdb" = "yes"])
dnl CPPFLAGS="$saveCPPFLAGS"
dnl LDFLAGS="$saveLDFLAGS"



# --------------------------------------------------------------------
# YY-specific modes
# --------------------------------------------------------------------

AC_ARG_ENABLE(yy,
  AC_HELP_STRING([--disable-yy], [disable the YY extensions]),
  [case "${enableval}" in
     yes) yy=yes ;;
     no)  yy=no ;;
     *)   AC_MSG_ERROR(bad value ${enableval} for --enable-yy) ;;
   esac
  ],
  [yy=yes])
if test "$yy" = "yes"; then 
    AC_DEFINE(YY, [1], [Include some YY specific code])
fi
AM_CONDITIONAL(YY, [test "$yy" = "yes"])



# --------------------------------------------------------------------
# instantiation failure cache
# --------------------------------------------------------------------

AC_ARG_ENABLE(instfc,
  AC_HELP_STRING([--disable-instfc], [disable the instantiation failure cache]),
  [case "${enableval}" in
     yes) instfc=yes ;;
     no)  instfc=no ;;
     *)   AC_MSG_ERROR(bad value ${enableval} for --enable-instfc) ;;
   esac],
  [instfc=yes])
if test "$instfc" = "yes"; then 
    AC_DEFINE(INSTFC, [1], [Include instantiation failure cache code])
fi
AM_CONDITIONAL(INSTFC, [test "$instfc" = "yes"])



# --------------------------------------------------------------------
# quick check feature
# --------------------------------------------------------------------

AC_ARG_ENABLE(qccomp,
  [AC_HELP_STRING([--enable-qccomp], [enable recording of failures for quick check computation])],
  [case "${enableval}" in
     yes) qccomp="${enableval}" ;;
     no)  qccomp="${enableval}" ;;
     *)   AC_MSG_ERROR(bad value ${enableval} for --enable-qccomp) ;;
   esac
  ],
  [qccomp=no])
if test x$qccomp = xyes ; then
    AC_DEFINE(QC_PATH_COMP, [1], [Enable code for QuickCheck computation])
fi



# --------------------------------------------------------------------
# LUI
# --------------------------------------------------------------------

AC_ARG_ENABLE(lui,
  AC_HELP_STRING([--enable-lui], [enable LUI dumping]),
  [case "${enableval}" in
     yes) lui=yes ;;
     no)  lui=no ;;
     *)   AC_MSG_ERROR(bad value ${enableval} for --enable-lui) ;;
   esac],
  [lui=no])
if test "$lui" = "yes"; then 
    AC_DEFINE(LUI, [1], [Enable grammar dumping in LUI format])
fi
# no extra files, don't need Makefile conditional
# AM_CONDITIONAL(LUI, [test "$lui" = "yes"])



# --------------------------------------------------------------------
# external dictionary (???)
# --------------------------------------------------------------------

dnl What the *&^@~! is that?
dnl AC_ARG_ENABLE(external-dictionary,
dnl [AC_HELP_STRING([--enable-external-dictionary@<:@=DIR@:>@], [use an external dictionary (default=no)])],
dnl [case "${enableval}" in
dnl    yes) extdict=yes ;;
dnl    no)  extdict=no ;;
dnl    *)   AC_MSG_ERROR([bad value ${enableval} for --enable-external-dictionary])
dnl  esac
dnl  if test "$extdict" = "yes" ; then
dnl     AC_DEFINE(HAVE_EXTDICT, [1], [enable external dictionary])
dnl  fi
dnl ], [extdict=no])
dnl AM_CONDITIONAL(EXTDICT, [test "$extdict" = "yes"])



# --------------------------------------------------------------------
# robust processing
# --------------------------------------------------------------------

dnl robust processing code by frederik fouvry, not supported at the moment
dnl AC_ARG_ENABLE(rcu,
dnl [AC_HELP_STRING([--enable-rcu@<:@=ARG@:>@], [use the RCU extensions (default=no)])],
dnl [case "${enableval}" in
dnl    yes) rcu=yes ;;
dnl    no)  rcu=no ;;
dnl    *)   AC_MSG_ERROR([bad value ${enableval} for --enable-rcu])
dnl esac], [rcu=no])
dnl AM_CONDITIONAL(RCU, [test "$rcu" = "yes"])



# --------------------------------------------------------------------
# mmap allocator
# --------------------------------------------------------------------

AC_ARG_ENABLE(mmapalloc,
  [AC_HELP_STRING([--enable-mmapalloc], [enable mmap allocator])],
  [case "${enableval}" in
     yes) mmapalloc="${enableval}" ;;
     no)  mmapalloc="${enableval}" ;;
     *)   AC_MSG_ERROR(bad value ${enableval} for --enable-lui) ;;
   esac],
  [mmapalloc=no])
if test "$mmapalloc" = "yes" ; then
    AC_FUNC_MMAP
fi



# --------------------------------------------------------------------
# Summary
# --------------------------------------------------------------------

echo
echo "============================================================="
echo "Boost (required for flop)                       $boost"
echo "tsdb (system benchmarking)                      $tsdb"
echo "Embeddable Common-Lisp processor (ECL)          $ecl"
echo "MRS code (requires ECL)                         $mrs"
echo "preprocessor code (requires ECL)                $preproc"
echo "ICU (Unicode support)                           $icu"
echo "Xerces C++ (for XML input modes)                $xml"
echo "CppUnit (for unit testing)                      $cppunit"
#echo "External SQL dictionary                         $lexdb"
echo "Memory allocation with mmap                     $mmapalloc"
echo "Code for quick-check computation                $qccomp"
echo "============================================================="
echo



# --------------------------------------------------------------------
# Output
# --------------------------------------------------------------------

AC_SUBST(CHEAPLIBS)
AC_SUBST(CHEAPLDFLAGS)
AC_SUBST(CHEAPCPPFLAGS)

AC_SUBST(FLOPCPPFLAGS)

# Excluded doc and goofy
AC_CONFIG_FILES([Makefile
                 fspp/Makefile
                 cheap/Makefile
                 cheap/test/Makefile
                 common/Makefile
                 flop/Makefile
                 doxyconfig.cheap
                 doxyconfig.flop])

AC_OUTPUT

