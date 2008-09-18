/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2003 Ulrich Callmeier uc@coli.uni-sb.de
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/** \file petecl.h
 * ECL integration.
 *
 * Since ECL (and ecl.h in particular) doesn't compile as C++ we
 * cannot use ECL datatypes or functions from C++ modules. On the
 * other hand, we cannot call C++ functions from C (there's nothing
 * like an extern "C++" directive), so our setup is a little involved.
 *
 * The modules petecl and petmrs are written in C. They use ECL
 * functions and datatypes, and have access to PET data and
 * functionality by means of functions contained in the cppbridge
 * module. cppbridge is written in C++ and exports a number of extern
 * "C" functions. It includes standard PET headers, and uses their
 * functionality to implement the extern "C" functions.
 */


#ifndef _PETECL_H_
#define _PETECL_H_

#include "pet-config.h"

#ifdef HAVE_ECL_ECL_H
#include <ecl/ecl.h>
#endif
#ifdef HAVE_ECL_H
#include <ecl.h>
#endif

#ifndef HAVE_MAKE_STRING_COPY
#ifdef HAVE_MAKE_BASE_STRING_COPY
#define make_string_copy make_base_string_copy
#endif
#endif
#ifndef HAVE_MAKE_SIMPLE_STRING
#ifdef HAVE_MAKE_SIMPLE_BASE_STRING
#define make_simple_string make_simple_base_string 
#endif
#endif
#ifndef HAVE_AREF1
#ifdef HAVE_ECL_AREF1
#define aref1 ecl_aref1 
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \brief ECL initialization function. Boots the ECL engine.
 */
int ecl_initialize(int argc, char **argv);

/** Load a lisp file with the given name using the ECL interpreter. */
void ecl_load_lispfile(const char *s);

/** Create a C string from a Lisp string */
char * ecl_decode_string(cl_object x);

/** Turn a Lisp array of int into a vector of int values, terminated with -1
 * \attn the caller is responsible for freeing the result of this function
 */
int * ecl_decode_vector_int(cl_object x);

/** Evaluate the given string \a form in the lisp listener
 * \pre \a form must be a valid s-expression
 * \return the object returned by the evaluation
 */
cl_object ecl_eval_sexpr(char *form);

#ifdef __cplusplus
}
#endif
#endif
