/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
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

/* types and attributes */

#ifndef _TYPES_H_
#define _TYPES_H_

#include "bitcode.h"
#include "builtins.h"

// types, attributes, and status are represented by (small) integers

//
// status
//

// A status value is stored for each type. predefined status values are
// `NO_STATUS' and `ATOM_STATUS'.

#define NO_STATUS 0
#define ATOM_STATUS 1

// number of status values
extern int nstatus;

// names of status values, [0 .. nstatus[
extern char **statusnames;

//
// types
//

// the universe of types [0 .. ntypes[ is split into two consecutive,
// disjunct ranges:
//  1) proper types  [ 0 .. first_leaftype [ 
//  2) leaf types    [ first_leaftype .. ntypes [
// proper types have a full bitcode representation, for leaf types only the
// parent type is stored. a status value is stored for each type.

typedef int type_t;

extern int nleaftypes, ntypes;
extern type_t first_leaftype;

extern int *typestatus;

extern char **typenames;
extern char **printnames; // preferred way to print a type name

//
// attributes
//

extern int nattrs;
extern char **attrname;
extern int *attrnamelen; // for faster printing

extern type_t *apptype; // appropriate type for feature

// name of dynamic sort
extern vector<const char *> dynsortname;
extern type_t last_dynamic;

// 
// codes: these are used to represent subtype relationships
// for proper types

void initialize_codes(int n);
void resize_codes(int n);
void register_codetype(const bitcode &b, type_t i);
void register_typecode(type_t i, bitcode *b);
type_t lookup_code(const bitcode &b);

void dump_hierarchy(dumper *f);

void undump_hierarchy(dumper *f);
void undump_tables(dumper *f);

void free_type_tables();

int lookup_status(const char *s);
int lookup_attr(const char *s);
type_t lookup_type(const char *s);

type_t lookup_symbol(const char *s);
void clear_dynamic_symbols () ;

void dump_symbol_tables(dumper *f);
void undump_symbol_tables(dumper *f);
void undump_printnames(dumper *f);

bool core_subtype(type_t a, type_t b);
bool subtype(type_t a, type_t b);
type_t glb(type_t a, type_t b);

type_t leaftype_parent(type_t t);

inline bool is_type(type_t a)
{
  return a >= 0 && a < ntypes;
}

#ifndef FLOP
void prune_glbcache();
#endif

#endif
