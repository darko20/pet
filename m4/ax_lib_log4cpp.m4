# SYNOPSIS
#
#   AX_LIB_LOG4CPP([minimum-version], [action-if-found], [action-if-not-found]
#                  [default-answer])
#
# DESCRIPTION
#
#   Tests for the presence of the LOG4CPP library (logging for c++)
#
#   If LOG4CPP (of the specified version) is found, the following variables are
#   defined as shell and autoconf output variables:
#
#     LOG4CPP_LIBS
#
#   and the following preprocessor define is set:
#
#     HAVE_LOG4CPP
#
# CHANGELOG
#
#   2009-07-28 by Peter Adolphs
#              - added default-answer argument to macro
#              - unset local variables
#   2008-09-19 by Bernd Kiefer
#
# COPYLEFT
#
#   Copyright (c) 2008 Bernd Kiefer
#
#   Copying and distribution of this file, with or without
#   modification, are permitted in any medium without royalty provided
#   the copyright notice and this notice are preserved.

AC_DEFUN([AX_LIB_LOG4CPP], [
  # define configure parameter
  unset ax_lib_log4cpp ax_lib_log4cpp_config ax_lib_log4cpp_path
  AC_ARG_WITH([log4cpp],
    [AC_HELP_STRING(
      [--with-log4cpp@<:@=ARG@:>@], dnl
      [use log4cpp library from a standard location (ARG=yes),
       from the specified location or log4cpp-config binary (ARG=<path>),
       or disable it (ARG=no)
       @<:@ARG=$4@:>@ ])],
    [case "${withval}" in
       yes) ax_lib_log4cpp="yes" ;;
       no)  ax_lib_log4cpp="no" ;;
       *)   ax_lib_log4cpp="yes"
            if test -d $withval ; then
              ax_lib_log4cpp_path="$withval/bin"
            elif test -x $withval ; then
              ax_lib_log4cpp_config=$withval
              ax_lib_log4cpp_path=$(dirname $ax_lib_log4cpp_config)
            else
              AC_MSG_ERROR([bad value ${withval} for --with-log4cpp ])
            fi ;;
     esac],
    [ax_lib_log4cpp="$4"])
  if test "x$ax_lib_log4cpp_path" = "x" ; then
    ax_lib_log4cpp_path=$PATH
  fi
  
  # check for log4cpp-config
  if test "x$ax_lib_log4cpp" = "xyes" ; then
    AC_PATH_PROG([ax_lib_log4cpp_config], [log4cpp-config], [""], [$ax_lib_log4cpp_path])
    if test "x$ax_lib_log4cpp_config" = "x" ; then
      ax_lib_log4cpp="no"
      AC_MSG_WARN([LOG4CPP library requested but log4cpp-config not found.])
    fi
  fi
  
  # check version
  if test "x$ax_lib_log4cpp" = "xyes" && test "x$1" != "x" ; then
    AC_MSG_CHECKING([whether LOG4CPP version >= $1])
    ax_lib_log4cpp_version=$($ax_lib_log4cpp_config --version)
    if test $(expr $ax_lib_log4cpp_version \>\= $1) = 1 ; then
      AC_MSG_RESULT([yes])
    else
      AC_MSG_RESULT([no])
      ax_lib_log4cpp="no"
    fi
  fi
  
  # get various flags and settings
  # cf. Autoconf manual:
  # LDFLAGS: options as `-s' and `-L' affecting only the behavior of the linker
  # LIBS: `-l' options to pass to the linker
  unset LOG4CPP_LIBS
  if test "x$ax_lib_log4cpp" = "xyes" ; then
    LOG4CPP_LIBS=$($ax_lib_log4cpp_config --libs)
  fi
  
  ax_lib_log4cpp_saved_LIBS=$LIBS
  LIBS="$LOG4CPP_LIBS $LIBS"
  
  # checking library functionality
  if test "x$ax_lib_log4cpp" = "xyes" ; then
    AC_REQUIRE([AC_PROG_CXX])
    AC_LANG_PUSH([C++])
    AC_MSG_CHECKING([LOG4CPP library usability])
    AC_LINK_IFELSE(
      [AC_LANG_PROGRAM(
        [[@%:@include <log4cpp/Category.hh>]],
        [[ log4cpp::Category &root = log4cpp::Category::getRoot(); ]])],
      [AC_MSG_RESULT([yes])],
      [AC_MSG_RESULT([no]) ; ax_lib_log4cpp="no"])
    AC_LANG_POP([C++])
  fi
  
  LIBS=$ax_lib_log4cpp_saved_LIBS
  
  # final actions
  AC_SUBST([LOG4CPP_LIBS])
  if test "x$ax_lib_log4cpp" = "xyes"; then
    # execute action-if-found (if any)
    AC_DEFINE(HAVE_LOG4CPP, [1], [define to 1 if LOG4CPP library is available])
    ifelse([$2], [], :, [$2])
  else
    # execute action-if-not-found (if any)
    ifelse([$3], [], :, [$3])
  fi
])

