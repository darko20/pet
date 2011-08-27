# SYNOPSIS
#
#   AX_LIB_XMLRPC_CXX([features], [action-if-found], [action-if-not-found])
#
# DESCRIPTION
#
#   Tests for the presence of the xmlrpc-c library with the requested features.
#
#   If xmlrpc-c is found, the following variables are
#   defined as shell and autoconf output variables:
#
#     XMLRPC_C_CPPFLAGS_SEARCHPATH
#     XMLRPC_C_LDFLAGS_SEARCHPATH
#     XMLRPC_C_LIBS
#
#   and the following preprocessor define is set:
#
#     HAVE_XMLRPC_C
#
# EXAMPLE
#
#   AX_LIB_XMLRPC_CXX([c++2 client])
#
# LAST MODIFICATION
#
#   2008-04-09
#
# COPYLEFT
#
#   Copyright (c) 2008 Peter Adolphs
#
#   Copying and distribution of this file, with or without
#   modification, are permitted in any medium without royalty provided
#   the copyright notice and this notice are preserved.

AC_DEFUN([AX_LIB_XMLRPC_C], [
  # define configure parameter
  unset ax_lib_xmlrpc_c_path ax_lib_xmlrpc_c_config
  AC_ARG_WITH([xmlrpc-c],
    [AC_HELP_STRING(
      [--with-xmlrpc-c@<:@=ARG@:>@], dnl
      [use xmlrpc-c library from a standard location (ARG=yes),
       from the specified location or xmlrpc-c-config binary (ARG=<path>),
       or disable it (ARG=no)
       @<:@ARG=yes@:>@ ])],
    [case "${withval}" in
       yes) ax_lib_xmlrpc_c="yes" ;;
       no)  ax_lib_xmlrpc_c="no" ;;
       *)   ax_lib_xmlrpc_c="yes"
            if test -d $withval ; then
              ax_lib_xmlrpc_c_path="$withval/bin"
            elif test -x $withval ; then
              ax_lib_xmlrpc_c_config=$withval
              ax_lib_xmlrpc_c_path=$(dirname $ax_lib_xmlrpc_c_config)
            else
              AC_MSG_ERROR([bad value ${withval} for --with-xmlrpc-c ])
            fi ;;
     esac],
    [ax_lib_xmlrpc_c="yes"])
  if test "x$ax_lib_xmlrpc_c_path" = "x" ; then
    ax_lib_xmlrpc_c_path=$PATH
  fi
  
  # check for xmlrpc-c-config
  if test "x$ax_lib_xmlrpc_c" = "xyes" ; then
    AC_PATH_PROG([ax_lib_xmlrpc_c_config], [xmlrpc-c-config], [""], [$ax_lib_xmlrpc_c_path])
    if test "x$ax_lib_xmlrpc_c_config" = "x" ; then
      ax_lib_xmlrpc_c="no"
      AC_MSG_WARN([xmlrpc-c library requested but xmlrpc-c-config not found.])
    fi
  fi
  
  # check whether the required features are supported
  if test "x$ax_lib_xmlrpc_c" = "xyes" ; then
  AC_MSG_CHECKING([whether xmlrpc-c supports the requested features])
    if $ax_lib_xmlrpc_c_config $1 >/dev/null 2>&1 ; then
      AC_MSG_RESULT([yes])
    else
      AC_MSG_RESULT([no])
      ax_lib_xmlrpc_c="no"
    fi
  fi
  
  # get various flags and settings
  # cf. Autoconf manual:
  # LDFLAGS: options as `-s' and `-L' affecting only the behavior of the linker
  # LIBS: `-l' options to pass to the linker
  unset XMLRPC_C_CPPFLAGS_SEARCHPATH
  unset XMLRPC_C_LDFLAGS_SEARCHPATH
  unset XMLRPC_C_LIBS
  if test "x$ax_lib_xmlrpc_c" = "xyes" ; then
    XMLRPC_C_CPPFLAGS_SEARCHPATH=$($ax_lib_xmlrpc_c_config $1 --cflags |
                sed -e 's/ /\n/g; s/^-[[^I]].*//mg; s/^ *\n//mg; s/\n/ /mg;')
    XMLRPC_C_LDFLAGS_SEARCHPATH=$($ax_lib_xmlrpc_c_config $1 --ldadd |
                sed -e 's/ /\n/g; s/^-[[^L]].*//mg; s/^ *\n//mg; s/\n/ /mg;')
    XMLRPC_C_LIBS=$($ax_lib_xmlrpc_c_config $1 --libs |
                sed -e 's/ /\n/g; s/^-[[^l]].*//mg; s/^ *\n//mg; s/\n/ /mg;')
  fi
  
  ax_lib_xmlrpc_c_saved_CPPFLAGS=$CPPFLAGS
  ax_lib_xmlrpc_c_saved_LDFLAGS=$LDFLAGS
  ax_lib_xmlrpc_c_saved_LIBS=$LIBS
  CPPFLAGS="$XMLRPC_C_CPPFLAGS_SEARCHPATH $CPPFLAGS"
  LDFLAGS="$XMLRPC_C_LDFLAGS_SEARCHPATH $LDFLAGS"
  LIBS="$XMLRPC_C_LIBS $LIBS"
  
  # checking library functionality
  if test "x$ax_lib_xmlrpc_c" = "xyes" ; then
    AC_REQUIRE([AC_PROG_CC])
    AC_LANG_PUSH([C])
    AC_MSG_CHECKING([xmlrpc-c library usability])
    AC_LINK_IFELSE(
      [AC_LANG_PROGRAM(
        [[@%:@include <xmlrpc-c/base.h>]],
        [[xmlrpc_env env; xmlrpc_env_init(&env);]])],
      [AC_MSG_RESULT([yes])],
      [AC_MSG_RESULT([no]) ; ax_lib_xmlrpc_c="no"])
    AC_LANG_POP([C])
  fi
  
  CPPFLAGS=$ax_lib_xmlrpc_c_saved_CPPFLAGS
  LDFLAGS=$ax_lib_xmlrpc_c_saved_LDFLAGS
  LIBS=$ax_lib_xmlrpc_c_saved_LIBS
  
  # final actions
  AC_SUBST([XMLRPC_C_CPPFLAGS_SEARCHPATH])
  AC_SUBST([XMLRPC_C_LDFLAGS_SEARCHPATH])
  AC_SUBST([XMLRPC_C_LIBS])
  if test "x$ax_lib_xmlrpc_c" = "xyes"; then
    # execute action-if-found (if any)
    AC_DEFINE(HAVE_XMLRPC_C, [1], [define to 1 if xmlrpc-c library is available])
    ifelse([$2], [], :, [$2])
  else
    # execute action-if-not-found (if any)
    ifelse([$3], [], :, [$3])
  fi
])

