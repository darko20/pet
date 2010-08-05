# Process this file with autoconf to produce a configure script.
# Use autoreconf -i && config.status --recheck after changing this file.

AC_PREREQ(2.59)
AC_INIT([PET], [0.99.14svn_cm], [Peter.Adolphs@dfki.de])
AC_REVISION($LastChangedRevision$)
AC_CONFIG_SRCDIR([pet-config.h.in])
AC_CONFIG_HEADERS([pet-config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.7 foreign])
dnl dejagnu dist-bzip2 no-dependencies


# ====================================================================
# Path settings
# ====================================================================

# Paths should be set using configure command-line options.

# Precious variables (values will be preserved by config.status --recheck)
AC_ARG_VAR([DELPHINHOME], [the root directory for the DELPH-IN installation])

# Try to find the LKB directory (unconditional of any other test)
LKBDIR=""
for dir in "$DELPHINHOME" "/usr/local/delphin" "$HOME/delphin" ; do
  test "x$dir" != "x" || continue
  AC_MSG_CHECKING([directory $dir/lkb presence])
  if test -d "$dir/lkb" ; then
    AC_MSG_RESULT([yes])
    LKBDIR="$dir/lkb"
    break
  else
    AC_MSG_RESULT([no])
  fi
done


# ====================================================================
# System-specific settings
# ====================================================================

# Check whether linux is 32 or 64 bit and create variable "OS" to point to
# correct library location.
# Code liberally borrowed from LKB install script ...
# Eric Nichols <eric-n@is.naist.jp> -- Jun. 18th, 2005

# apparently, (some) Debian installations come with an older uname(1), where
# `-i' is not available :-{.                                 (11-mar-05; oe)
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

# Determine system-specific flags for the shared library runtime path (rpath)
# mechanism. This macro is taken from the GNU gettext package.
# Called here explicitly to have the configure option --disable-rpath
# before the more pet-specific options.
AC_LIB_RPATH

case $(uname -s) in
   CYGWIN* )
     AC_DEFINE(PATH_SEP, "\\", [directory path separator char]) ;;
   * )
     AC_DEFINE(PATH_SEP, "/", [directory path separator char]) ;;
esac

# let's use the method given in LKB's install script to get the kernel version
# too ... Eric Nichols <eric-n@is.naist.jp> -- Jun. 19, 2005
KERNEL=$(uname -r)
case "${KERNEL}" in
  2.4*)
    AC_DEFINE([KERNEL_2_4],[1],[Current Linux kernel is 2.4.x]) ;;
  2.6*)
    AC_DEFINE([KERNEL_2_6],[1],[Current Linux kernel is 2.6.x]) ;;
  *)
    AC_MSG_WARN([Unknown Linux kernel version (check 'uname -r').]) ;;
esac


# ====================================================================
# Checks for programs.
# ====================================================================

AC_PROG_CXX
AC_PROG_CPP
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
# AC_PROG_RANLIB

# Check for doxygen to generate documentation
AC_PATH_PROG([DOXYGEN],[doxygen])
AM_CONDITIONAL([DOCBUILDERS], [ test "$DOXYGEN" != "" ])
if test "$DOXYGEN" = "" ; then
    AC_WARN([Cannot find doxygen. Disabling API doc targets.])
fi


# ====================================================================
# Checks for header files.
# ====================================================================

# enable assertions; introduces the `--disable-assert' option
m4_pattern_allow([AC_HEADER_ASSERT]) dnl aclocal shouldn't complain if missing
AC_HEADER_ASSERT

# defines STDC_HEADERS if there are C header files conforming to ANSI C89
AC_HEADER_STDC

# defines HAVE_SYS_WAIT_H if there is a Posix-compatible `sys/wait.h'
AC_HEADER_SYS_WAIT

# defines TIME_WITH_SYS_TIME if there are `time.h' and `sys/time.h'
AC_HEADER_TIME

# check for the presence of certain headers and raise an error if not
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h locale.h netdb.h netinet/in.h],,
                 [AC_MSG_ERROR("some system header file not found")])
AC_CHECK_HEADERS([sys/ioctl.h sys/param.h sys/socket.h sys/time.h unistd.h],,
                 [AC_MSG_ERROR("some system header file not found")])
AC_CHECK_HEADERS([stddef.h stdlib.h string.h strings.h],,
                 [AC_MSG_ERROR("some system header file not found")])

# hash_map and hash_set (C++)
AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([hash_map],,
   [AC_CHECK_HEADERS([ext/hash_map],,[AC_MSG_ERROR("No hash_map header found")],)])
AC_CHECK_HEADERS([hash_set],,
   [AC_CHECK_HEADERS([ext/hash_set],,[AC_MSG_ERROR("No hash_set header found")],)])
AC_LANG_POP([C++])


# ====================================================================
# Checks for typedefs, structures, and compiler characteristics.
# ====================================================================

AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_STRUCT_TM  dnl this macro is becoming obsolete
AC_CHECK_SIZEOF(int *)


# ====================================================================
# Checks for library functions.
# ====================================================================

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
AC_FUNC_WAIT3 dnl this macro is becoming obsolete
AC_CHECK_FUNCS([bzero dup2 gethostbyaddr gettimeofday inet_ntoa setlocale socket])
AC_CHECK_FUNCS([strcasecmp strchr strdup strerror strpbrk strrchr strtol])


# ====================================================================
# Checks for libraries.
# ====================================================================

AC_LANG_PUSH(C++)

# Check used and needed in linking?
AC_CHECK_LIB([stdc++], [main], , [AC_MSG_ERROR([Cannot find libstdc++])])

# search a lib that provides getopt_long_only (lib is added to LIBS if found)
AC_SEARCH_LIBS([getopt_long_only], [iberty], [],
               [AC_MSG_ERROR([No library containing getopt_long_only()], [1])])

# the following library checks are either not needed or performed elsewhere:
# FIXME: Replace `main' with a function in `-ldl':
#AC_CHECK_LIB([dl], [main])
# FIXME: Replace `main' with a function in `-liberty':
#AC_CHECK_LIB([iberty], [main])
# FIXME: Replace `main' with a function in `-lm':
#AC_CHECK_LIB([m], [main])
# FIXME: Replace `main' with a function in `-lpvm3':
#AC_CHECK_LIB([pvm3], [main])
# FIXME: Replace `main' with a function in `-lgmp':
# AC_CHECK_LIB([gmp], [main])


# ====================================================================
# Configurable features/libraries
# ====================================================================

# Features/packages header files and libraries need to be done with
# dependencies: only check if asked, and if not found switch off all
# related/dependent checks.
# IF requested AND libraries and headers OK; THEN add to configuration
# For libraries: it should be possible to choose for static ones

# Some autoconf macros change CPPFLAGS/CXXFLAGS/LDFLAGS/LIBS. Let's backup
# these variables in order to restore the defaults. (Optional libraries
# should compile without other optional libraries being present.)
savedCPPFLAGS=$CPPFLAGS
savedCXXFLAGS=$CXXFLAGS
savedLDFLAGS=$LDFLAGS
savedLIBS=$LIBS

# Define flop- and cheap-specific CPPFLAGS/CXXFLAGS/LDFLAGS/LIBS
# cf. Autoconf manual or ./configure --help for a description
unset FLOPCPPFLAGS FLOPCXXFLAGS FLOPLDFLAGS FLOPLIBS
unset CHEAPCPPFLAGS CHEAPCXXFLAGS CHEAPLDFLAGS CHEAPLIBS


# --------------------------------------------------------------------
# IBM ICU library.
# --------------------------------------------------------------------

AX_LIB_ICU([],
  [
    icu=yes
    export CHEAPCPPFLAGS="$ICU_CPPFLAGS_SEARCHPATH $CHEAPCPPFLAGS"
    export CHEAPLDFLAGS="$ICU_LDFLAGS_SEARCHPATH $CHEAPLDFLAGS"
    export CHEAPLIBS="$ICU_LIBS $CHEAPLIBS"
  ],
  [icu=no])
AM_CONDITIONAL(ICU, [test "x$icu" = "xyes"])


# --------------------------------------------------------------------
# Boost libraries
# --------------------------------------------------------------------

AX_BOOST_BASE([], [boost="yes"], [boost="no"])
AX_BOOST_REGEX([], [boost=no])
# AX_BOOST_FILESYSTEM([], [boost=no]) # maybe later
if test "$boost" = "yes" ; then
  CPPFLAGS="${CPPFLAGS} ${BOOST_CPPFLAGS}"
  AC_CHECK_HEADERS([boost/format.hpp \
                    boost/graph/adjacency_list.hpp \
                    boost/graph/topological_sort.hpp \
                    boost/lambda/lambda.hpp \
                    boost/lexical_cast.hpp \
                    boost/regex.hpp \
                    boost/tuple/tuple.hpp \
                    boost/tuple/tuple_comparison.hpp ],
                   [],
                   [ boost="no" ])
  if test "x$icu" = "xyes" ; then
    AC_CHECK_HEADERS([boost/regex/icu.hpp],
                     [],
                     [AC_MSG_WARN([Boost::Regex is not configured for ICU.
                                   No Unicode-support in regular expressions.])],
                     [])
  fi
  CPPFLAGS="${savedCPPFLAGS}"
fi
if test "$boost" = "yes" ; then
  FLOPCPPFLAGS="${BOOST_CPPFLAGS} ${FLOPCPPFLAGS}"
  FLOPLDFLAGS="${BOOST_LDFLAGS} ${FLOPLDFLAGS}"
  CHEAPCPPFLAGS="${BOOST_CPPFLAGS} ${CHEAPCPPFLAGS}"
  CHEAPLDFLAGS="${BOOST_LDFLAGS} ${CHEAPLDFLAGS}"
  CHEAPLIBS="${BOOST_REGEX_LIBS} ${CHEAPLIBS}"
else
  AC_MSG_ERROR([required Boost libraries are missing or incomplete])
fi


# --------------------------------------------------------------------
# log4cpp library.
# --------------------------------------------------------------------

AX_LIB_LOG4CPP([],
  [
    log4cpp=yes
    export FLOPLIBS="$LOG4CPP_LIBS $FLOPLIBS"
    export CHEAPLIBS="$LOG4CPP_LIBS $CHEAPLIBS"
  ],
  [log4cpp=no],
  [no])
AM_CONDITIONAL(LOG4CPP, [test "x$log4cpp" = "xyes"])


# --------------------------------------------------------------------
# XML support, i.e. Xerces C++ library.
# --------------------------------------------------------------------

XERCESCDIR=""
AC_ARG_WITH(xml,
  [AC_HELP_STRING([--with-xml@<:@=ARG@:>@],
    [use Xerces C++ library from a standard location (ARG=yes),
     from the specified location (ARG=<path>),
     or disable it (ARG=no).
     @<:@ARG=no@:>@ ])],
  [
    case "${withval}" in
        yes) xml="yes" ;;
        no)  xml="no" ;;
        *)   xml="yes"
             XERCESCDIR=${withval} ;;
    esac
  ],
  [xml="no"])

if test "x$xml" = "xyes" && test "x${XERCESCDIR}" != "x" ; then
  if test -d "${XERCESCDIR}" ; then
    LDFLAGS="-L${XERCESCDIR}/lib $LDFLAGS"
  else
    AC_MSG_WARN([directory ${XERCESCDIR} not found for --with-xml; DISABLED])
    xml="no"
  fi
fi
if test "x$xml" = "xyes" ; then
  LDFLAGS="$ICU_LDFLAGS_SEARCHPATH $LDFLAGS"
  LIBS="$ICU_LIBS $LIBS"
  AC_CHECK_LIB([xerces-c], [main], [ xml="yes" ],
    [
      xml="no"
      AC_MSG_WARN([Xerces C++ library not found for --with-xml; DISABLED])
    ])
fi
if test "x$xml" = "xyes" ; then
  CHEAPCPPFLAGS="-I${XERCESCDIR}/include $CHEAPCPPFLAGS"
  CHEAPLDFLAGS="-L${XERCESCDIR}/lib $CHEAPLDFLAGS"
  CHEAPLIBS="-lxerces-c $CHEAPLIBS"
  AC_DEFINE(HAVE_XML, [1], [Enable XML input modes, using Xerces C++ library])
fi
AM_CONDITIONAL(XML, [test "x$xml" = "xyes"])
CPPFLAGS=$savedCPPFLAGS
CXXFLAGS=$savedCXXFLAGS
LDFLAGS=$savedLDFLAGS
LIBS=$savedLIBS


# --------------------------------------------------------------------
# [incr tsdb()] library.
# --------------------------------------------------------------------

ITSDBDIR=$LKBDIR

AC_ARG_WITH(tsdb,
  [AC_HELP_STRING(
    [--with-tsdb@<:@=ARG@:>@],
    [build with @<:@incr tsdb()@:>@ from a standard DELPH-IN location (ARG=yes),
     from the specified location (ARG=<path>),
     or disable it (ARG=no)
     @<:@ARG=yes@:>@ ]) ],
  [case "${withval}" in
    yes) tsdb="yes" ;;
    no)  tsdb="no" ;;
    *)   tsdb="yes"
         ITSDBDIR=${withval};;
   esac],
  [tsdb="yes"])

check_tsdb() {
  AC_MSG_CHECKING([directory ${ITSDBDIR}/include presence])
  if test -d ${ITSDBDIR}/include ; then
    AC_MSG_RESULT([yes])
    CPPFLAGS="-I${ITSDBDIR}/include $CPPFLAGS"
    AC_CHECK_HEADERS([itsdb.h], [], [ return 1 ], [])
  else
    AC_MSG_RESULT([no])
    AC_MSG_WARN([directory ${ITSDBDIR}/include for --with-tsdb not found; DISABLED])
    return 1
  fi

  AC_MSG_CHECKING([directory ${ITSDBDIR}/lib/${OS} presence])
  if test -d ${ITSDBDIR}/lib/${OS} ; then
      AC_MSG_RESULT([yes])
      LDFLAGS="-L${ITSDBDIR}/lib/${OS} $LDFLAGS"
      export LDFLAGS
      AC_CHECK_LIB([itsdb], [capi_register], [ return 0 ],
        [
          AC_MSG_WARN([itsdb library not found for --with-tsdb; DISABLED])
          return 1
        ])
  else
      AC_MSG_RESULT([no])
      AC_MSG_WARN([directory ${ITSDBDIR}/lib/${OS} for --with-tsdb not found; DISABLED])
      return 1
  fi
}

if test x$tsdb = xyes ; then
  if check_tsdb ; then
    AC_DEFINE(TSDBAPI, [1], ["Define to 1 for @<:@incr tsdb()@:>@ support"])
    CHEAPLIBS="-litsdb $CHEAPLIBS"
    CHEAPCPPFLAGS="-I${ITSDBDIR}/include $CHEAPCPPFLAGS"
    CHEAPLDFLAGS="-L${ITSDBDIR}/lib/${OS} $CHEAPLDFLAGS"
  else
    AC_MSG_WARN([could not find a working @<:@incr tsdb()@:>@ installation])
    tsdb="no"
  fi
fi
AM_CONDITIONAL(TSDB, [test x$tsdb = xyes])
CPPFLAGS=$savedCPPFLAGS
CXXFLAGS=$savedCXXFLAGS
LDFLAGS=$savedLDFLAGS
LIBS=$savedLIBS


# --------------------------------------------------------------------
# Lisp code for MRS.
# --------------------------------------------------------------------

# Although not really dependent, the default MRSDIR is set to the value of
# the ITSDBDIR (be it determined automatically or provided by the user with
# the --with-tsdb option).
MRSDIR=$ITSDBDIR

AC_ARG_WITH(mrs,
  [AC_HELP_STRING(
    [--with-mrs@<:@=ARG@:>@],
    [build with MRS Lisp sources from a standard DELPH-IN location (ARG=yes),
     from the specified location (ARG=<path>),
     or disable it (ARG=no)
     @<:@ARG=yes@:>@])],
  [
    case "${withval}" in
        yes) mrs="yes" ;;
        no)  mrs="no" ;;
        *)   mrs="yes"
             MRSDIR=${withval};;
    esac
  ],
  [mrs="yes"])

if test "x$mrs" = xyes ; then
  MRSDIR=${MRSDIR%%/src}
  AC_MSG_CHECKING([${MRSDIR}/src/general/loadup.lisp presence])
  if test -f "${MRSDIR}/src/general/loadup.lisp" ; then
    AC_MSG_RESULT([yes])
    mrs="yes"
  else
    AC_MSG_RESULT([no])
    AC_MSG_WARN([no MRS code found in ${MRSDIR}; DISABLED])
    MRSDIR=""
    mrs="no"
  fi
else
  MRSDIR=""
fi


# --------------------------------------------------------------------
# Lisp code for preprocessor
# --------------------------------------------------------------------

# Although not really dependent, the default PREPROCDIR is set to the value of
# the ITSDBDIR (be it determined automatically or provided by the user with
# the --with-tsdb option).
PREPROCDIR=$ITSDBDIR

AC_ARG_WITH([preproc],
  [AC_HELP_STRING(
    [--with-preproc@<:@=ARG@:>@],
    [build with preprocessing Lisp sources from a standard DELPH-IN location (ARG=yes),
     from the specified location (ARG=<path>),
     or disable it (ARG=no)
     @<:@ARG=yes@:>@])],
  [
    case "${withval}" in
        yes) preproc="yes" ;;
        no)  preproc="no" ;;
        *)   preproc="yes"
             PREPROCDIR=${withval};;
    esac
  ],
  [ preproc="yes" ])

if test "x$preproc" = xyes ; then
  PREPROCDIR=${PREPROCDIR%%/src}
  AC_MSG_CHECKING([${PREPROCDIR}/src/general/loadup.lisp presence])
  if test -f "${PREPROCDIR}/src/general/loadup.lisp" ; then
    AC_MSG_RESULT([yes])
    preproc="yes"
  else
    AC_MSG_RESULT([no])
    AC_MSG_WARN([no preprocessor code found in ${PREPROCDIR}; DISABLED])
    PREPROCDIR=""
    preproc="no"
  fi
else
  PREPROCDIR=""
fi


# --------------------------------------------------------------------
# ECL (Embeddable Common Lisp).
# --------------------------------------------------------------------

AX_LIB_ECL([ecl="yes"], [ecl="no"])

# check which functions ECL provides:
if test "$ecl" = "yes" ; then
  CPPFLAGS="$ECL_CPPFLAGS_SEARCHPATH $CPPFLAGS"
  LDFLAGS="$ECL_LDFLAGS_SEARCHPATH $LDFLAGS"
  LIBS="$ECL_LIBS $LIBS"
  AC_CHECK_FUNCS([make_base_string_copy make_string_copy], [break], [])
  AC_CHECK_FUNCS([make_simple_base_string make_simple_string], [break], [])
  AC_CHECK_FUNCS([ecl_aref1 aref1], [break], [])
  AC_CHECK_TYPES([struct ecl_base_string, struct ecl_string], [], [],
    [@%:@ifdef HAVE_ECL_ECL_H
@%:@include <ecl/ecl.h>
@%:@endif
@%:@ifdef HAVE_ECL_H
@%:@include <ecl.h>
@%:@endif])
  CPPFLAGS=$savedCPPFLAGS
  LDFLAGS=$savedLDFLAGS
  LIBS=$savedLIBS
fi

# all tests for ECL components have been executed, draw conclusions
if test "x$ecl" = "xyes" ; then
  if test "x$mrs" = "xyes" ; then
    AC_DEFINE(HAVE_MRS, [1], [use ECL MRS library])
    ECLUSER="_mrs"
  fi
  if test "x$preproc" = "xyes"; then
    AC_DEFINE(HAVE_PREPROC, [1], [use ECL preprocessing library])
    ECLUSER=${ECLUSER}"_preproc"
  fi
  if test "x$ECLUSER" != "x" ; then
    AC_DEFINE(HAVE_ECL, [1], [Use ECL for MRS or preprocessing library])
    CHEAPCPPFLAGS="$ECL_CPPFLAGS_SEARCHPATH $CHEAPCPPFLAGS"
    CHEAPLDFLAGS="$ECL_LDFLAGS_SEARCHPATH $CHEAPLDFLAGS"
    CHEAPLIBS="$ECL_LIBS $CHEAPLIBS"
  else
    AC_MSG_NOTICE([DISABLING ECL since none of the Lisp code is requested or available.])
    ecl="no"
  fi
else
  preproc="no"
  mrs="no"
fi
AM_CONDITIONAL(ECL, [test x$ECLUSER != x])
AM_CONDITIONAL(ECLMRS, [test x$mrs = xyes])
AM_CONDITIONAL(ECLPREPROC, [test x$preproc = xyes])
AC_SUBST(MRSDIR)
AC_SUBST(PREPROCDIR)


# --------------------------------------------------------------------
# XMLRPC for C and C++ library (xmlrpc-c)
# --------------------------------------------------------------------

AX_LIB_XMLRPC_C([c++2 abyss-server], [xmlrpc_c="yes"], [xmlrpc_c="no"])
if test x$xmlrpc_c = xyes; then
  CHEAPCPPFLAGS="$XMLRPC_C_CPPFLAGS_SEARCHPATH $CHEAPCPPFLAGS"
  CHEAPLDFLAGS="$XMLRPC_C_LDFLAGS_SEARCHPATH $CHEAPLDFLAGS"
  CHEAPLIBS="$XMLRPC_C_LIBS $CHEAPLIBS"
fi
AM_CONDITIONAL(XMLRPC_C, [test x$xmlrpc_c = xyes])


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
  esac],
  [unifier=tomabechi])
# Should be done differently: now they are independent
AM_CONDITIONAL(TOMABECHI_UNIFIER, [test x$unifier = xtomabechi])
AM_CONDITIONAL(SIMPLE_UNIFIER, [test x$unifier = xsimple])


# --------------------------------------------------------------------
# external lexicon based on postgresql database
# --------------------------------------------------------------------

dnl PSQLDIR=""

dnl AC_ARG_WITH(lexdb,
dnl [AC_HELP_STRING([--with-lexdb@<:@=DIR@:>@], [use the postgres DB lexicon (postgres library and header files required) (default=yes)])],
dnl [
dnl case "${withval}" in
dnl     yes) lexdb="yes" ;;
dnl     no)  lexdb="no" ;;
dnl     *)   lexdb="yes"
dnl          PSQLDIR=${withval} ;;
dnl esac
dnl ], [ lexdb="yes" ])

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

dnl if test x$lexdb = xyes && check_psql ; then
dnl    if test x$PSQLDIR != x ; then
dnl       CHEAPCPPFLAGS="$CHEAPCPPFLAGS -I${PSQLDIR}/include"
dnl       CHEAPLDFLAGS="$CHEAPLDFLAGS -L${PSQLDIR}/lib"
dnl    fi
dnl    CHEAPLIBS="-lpq $CHEAPLIBS"
dnl    AC_DEFINE(HAVE_PSQL_LEXDB, [1], [use external postgres lexical database])
dnl else
dnl    lexdb="no"
dnl fi
dnl AM_CONDITIONAL(LEXDB, [test x$lexdb = xyes])
dnl CPPFLAGS="$savedCPPFLAGS"
dnl LDFLAGS="$savedLDFLAGS"


# --------------------------------------------------------------------
# YY-specific modes
# --------------------------------------------------------------------

AC_ARG_ENABLE(yy,
  [AC_HELP_STRING([--disable-yy], [disable the YY extensions])],
  [case "${enableval}" in
    yes) yy="yes" ;;
    no)  yy="no" ;;
    *)   AC_MSG_ERROR(bad value ${enableval} for --enable-yy) ;;
   esac ],
  [yy="yes"])
if test x$yy = xyes; then
  AC_DEFINE(YY, [1], [Include some YY specific code])
fi
AM_CONDITIONAL(YY, [test x$yy = xyes])


# --------------------------------------------------------------------
# instantiation failure cache
# --------------------------------------------------------------------

AC_ARG_ENABLE(instfc,
  [AC_HELP_STRING([--disable-instfc],
                  [disable the instantiation failure cache])],
  [case "${enableval}" in
    yes) instfc="yes" ;;
    no)  instfc="no" ;;
    *)   AC_MSG_ERROR(bad value ${enableval} for --enable-instfc) ;;
   esac],
 [instfc="yes"])
if test "x$instfc" = "xyes"; then
  AC_DEFINE(INSTFC, [1], [Include instantiation failure cache code])
fi
AM_CONDITIONAL(INSTFC, [test "x$instfc" = "xyes"])


# --------------------------------------------------------------------
# LUI
# --------------------------------------------------------------------

AC_ARG_ENABLE(lui,
  [AC_HELP_STRING([--enable-lui], [enable LUI dumping])],
  [case "${enableval}" in
    yes) lui="yes" ;;
    no)  lui="no" ;;
    *)   AC_MSG_ERROR(bad value ${enableval} for --enable-lui) ;;
  esac],
  [lui="no"])
if test "x$lui" = "xyes"; then
  AC_DEFINE(LUI, [1], [Enable grammar dumping in LUI format])
fi


# --------------------------------------------------------------------
# external dictionary
# --------------------------------------------------------------------

dnl AC_ARG_ENABLE(external-dictionary,
dnl   [AC_HELP_STRING([--enable-external-dictionary@<:@=DIR@:>@],
dnl                   [use an external dictionary (default=no)])],
dnl   [case "${enableval}" in
dnl     yes) extdict="yes" ;;
dnl     no)  extdict="no" ;;
dnl     *)   AC_MSG_ERROR([bad value ${enableval} for --enable-external-dictionary])
dnl    esac ],
dnl   [extdict="no"])
dnl if test "x$extdict" = "xyes" ; then
dnl   AC_DEFINE(HAVE_EXTDICT, [1], [enable external dictionary])
dnl fi
dnl AM_CONDITIONAL(EXTDICT, [test "x$extdict" = "xyes"])


# --------------------------------------------------------------------
# robust processing code by frederik fouvry, not supported at the moment
# --------------------------------------------------------------------

dnl AC_ARG_ENABLE(rcu,
dnl   [AC_HELP_STRING([--enable-rcu@<:@=ARG@:>@],
dnl                   [use the RCU extensions (default=no)])],
dnl   [case "${enableval}" in
dnl     yes) rcu="yes" ;;
dnl     no)  rcu="no" ;;
dnl     *)   AC_MSG_ERROR([bad value ${enableval} for --enable-rcu])
dnl    esac],
dnl   [rcu="no"])
dnl AM_CONDITIONAL(RCU, [test "x$rcu" = "xyes"])


# --------------------------------------------------------------------
# mmap allocator
# --------------------------------------------------------------------

AC_ARG_ENABLE(mmapalloc,
  [AC_HELP_STRING([--enable-mmapalloc], [enable mmap allocator])],
  [case "${enableval}" in
     yes) mmapalloc="${enableval}" ;;
     no)  mmapalloc="${enableval}" ;;
     *)   AC_MSG_ERROR(bad value ${enableval} for --enable-mmapalloc) ;;
   esac],
  [mmapalloc="no"])
if test "$mmapalloc" = "yes" ; then
  AC_FUNC_MMAP
fi


# --------------------------------------------------------------------
# Summary of optional packages/features
# --------------------------------------------------------------------

echo
echo "============================================================="
echo "@<:@incr tsdb()@:>@ support                           $tsdb"
echo "LKB MRS code (requires ECL)                     $mrs"
echo "LKB preprocessor code (requires ECL)            $preproc"
echo "Embeddable Common-Lisp processor (ECL)          $ecl"
echo "ICU library (Unicode support)                   $icu"
echo "log4cpp library (extended logging support)      $log4cpp"
echo "Xerces C++ (XML input modes)                    $xml"
echo "XML-RPC for C/C++ (XML-RPC server mode)         $xmlrpc_c"
echo "CppUnit (unit testing)                          $cppunit"
echo "Grammar dumping in LUI format                   $lui"
echo "Memory allocation with mmap                     $mmapalloc"
echo "============================================================="
echo


# ====================================================================
# Output
# ====================================================================

# Get additional linker options for the runtime path of the binaries
# (these options are system and compiler specific and can be disabled
# by the --disable-rpath option).
AC_LIB_LINKFLAGS_FROM_LIBS([FLOPLDADDVAR], [$FLOPLDFLAGS])
AC_LIB_LINKFLAGS_FROM_LIBS([CHEAPLDADDVAR], [$CHEAPLDFLAGS])
CHEAPLDFLAGS="$CHEAPLDFLAGS $CHEAPLDADDVAR"
CHEAPLDFLAGS="$CHEAPLDFLAGS $CHEAPLDADDVAR"

AC_SUBST(FLOPCPPFLAGS)
AC_SUBST(FLOPCXXFLAGS)
AC_SUBST(FLOPLDFLAGS)
AC_SUBST(FLOPLIBS)

AC_SUBST(CHEAPCPPFLAGS)
AC_SUBST(CHEAPCXXFLAGS)
AC_SUBST(CHEAPLDFLAGS)
AC_SUBST(CHEAPLIBS)

AC_CONFIG_FILES([Makefile
                 fspp/Makefile
                 cheap/Makefile
                 cheap/test/Makefile
                 common/Makefile
                 common/test/Makefile
                 flop/Makefile
                 doxyconfig.cheap
                 doxyconfig.flop])

AC_OUTPUT

