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

/** \file cppbridge.h
 * C++ bridge to make select PET functionality accessible from C for ECL.
 *
 * The function in this module mainly forward from functions in petmrs.c into
 * core PET functions, and vice versa.
 *
 */

#ifndef _CPPBRIDGE_H_
#define _CPPBRIDGE_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Load Lisp files specified by the \a setting, using the directory of
 *  \a basename.
 */
void
ecl_cpp_load_files(char *setting, char *basename);

/** Return the number of types in the grammar */
int
pet_cpp_ntypes();

/** Get the type code for the string \a s, or -1, if not present */
int
pet_cpp_lookup_type_code(const char *s);

/** Get the name for the type with id \a t */
char *
pet_cpp_lookup_type_name(int t);

/** Return the number of attributes in the grammar */
int
pet_cpp_nattrs();

/** Get the attribute code for the string \a s, or -1, if not present */
int
pet_cpp_lookup_attr_code(const char *s);

/**  Get the name for the attribute with id \a a */
char *
pet_cpp_lookup_attr_name(int a);

/** Is the feature structure for handle \a fs valid, i.e., not \c FAIL ? */
int
pet_cpp_fs_valid_p(int fs);

/** Return a handle for the substructure under \a fs when following \a path,
 *  or -1, if the path does not exist.
 */
int
pet_cpp_fs_path_value(int fs, int *path);

/** Return a list of pairs (attribute id, fs handle) in a single integer array,
 *  that represent the arcs list of \a fs. 
 */
void
pet_cpp_fs_arcs(int fs, int **res);

/** Return the top level type of \a fs, or -1 if \a fs is not valid. */
int
pet_cpp_fs_type(int fs);

/** Return a non-zero value if \a t1 is a subtype of \a t2 */
int
pet_cpp_subtype_p(int t1, int t2);

/** Return the glb of  \a t1 and \a t2, or -1 if it does not exist. */
int
pet_cpp_glb(int t1, int t2);

/** Is \a t a valid type id? */
int
pet_cpp_type_valid_p(int t);

#ifdef __cplusplus

/** Extract the MRS from the dag \a d printing it to the returned string.
 * The available \a mode values depend on the Lisp code.
 */
string
ecl_cpp_extract_mrs(dag_node *d, char *mode);

}

#endif

#endif
