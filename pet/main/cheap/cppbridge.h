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

/* C++ bridge to make select PET functionality accessible from C for ECL */

#ifndef _CPPBRIDGE_H_
#define _CPPBRIDGE_H_

//
// The function in this module mainly forward from functions in petmrs.c into
// core PET functions, and vice versa.
//

#ifdef __cplusplus
extern "C" {
#endif

void
ecl_cpp_load_files(char *setting, char *basename);

int
pet_cpp_ntypes();

int
pet_cpp_lookup_type_code(const char *s);

char *
pet_cpp_lookup_type_name(int t);

int
pet_cpp_nattrs();

int
pet_cpp_lookup_attr_code(const char *s);

char *
pet_cpp_lookup_attr_name(int a);

int
pet_cpp_fs_valid_p(int fs);

int
pet_cpp_fs_path_value(int fs, int *path);

void
pet_cpp_fs_arcs(int fs, int **res);

int
pet_cpp_fs_type(int fs);

int
pet_cpp_subtype_p(int t1, int t2);

int
pet_cpp_glb(int t1, int t2);

int
pet_cpp_type_valid_p(int t);

#ifdef __cplusplus

string
ecl_cpp_extract_mrs(dag_node *d, char *mode);

}

#endif

#endif
