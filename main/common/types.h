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

using namespace std;

#include "bitcode.h"
#include "builtins.h"
#include <vector>

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
typedef int attr_t;

#define T_BOTTOM -1       // failure of type unification

extern int nleaftypes, ntypes;
extern type_t first_leaftype;

extern int *typestatus;

extern char **typenames;
extern char **printnames; // preferred way to print a type name

#ifdef DYNAMIC_SYMBOLS
extern vector<string> dyntypename;
#endif
//
// attributes
//

extern int nattrs;
extern char **attrname;
extern int *attrnamelen; // for faster printing

extern type_t *apptype; // appropriate type for feature
extern type_t *maxapp; // maximal appropriate type under feature

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

#ifndef FLOP
// Immediate supertypes for each non-leaftype.
extern vector<list<int> > immediateSupertype;
#endif

void dump_hierarchy(dumper *f);

void undump_hierarchy(dumper *f);
void undump_tables(dumper *f);
void initialize_maxapp();

void free_type_tables();

/** Check the validity of type code \a a. */
inline bool is_type(type_t a)
{ 
  return a >= 0 && a < last_dynamic;
}

/** Return \c true if type code \a a is a type from the hierarchy and not a
 *  dynamic type.
 */
inline bool is_resident_type(type_t a)
{
  assert(a >= 0);  // save one test in production code
  return a < ntypes;
}

#ifdef DYNAMIC_SYMBOLS
/** Return \c true if type code \a a is a dynamic type. */
inline bool is_dynamic_type(type_t a)
{
  assert((a >= 0) && (a < last_dynamic));  // save two tests in production code
  return a >= ntypes;
}
#endif

inline bool dag_arc_valid(attr_t attr)
{ 
  return (attr <= nattrs);
}

int lookup_status(const char *s);
attr_t lookup_attr(const char *s);
type_t lookup_type(const char *s);

// code for use with dynamic types
#ifdef DYNAMIC_SYMBOLS
type_t lookup_symbol(const char *s);
type_t lookup_unsigned_symbol(unsigned int i);
void clear_dynamic_symbols () ;

inline const char *type_name(type_t type) {
  return is_resident_type(type) 
    ? typenames[type] : dyntypename[type - ntypes].c_str();
}
inline const char *print_name(type_t type) {
  return is_resident_type(type) 
    ? printnames[type] : dyntypename[type - ntypes].c_str();
}
#else
inline const char *type_name(type_t type) { return typenames[type]; }
inline const char *print_name(type_t type) { return printnames[type]; }
#endif

void dump_symbol_tables(dumper *f);
void undump_symbol_tables(dumper *f);
void undump_printnames(dumper *f);

void
undumpSupertypes(dumper *f);

bool core_subtype(type_t a, type_t b);
bool subtype(type_t a, type_t b);
#ifndef FLOP
void
subtype_bidir(type_t A, type_t B, bool &a, bool &b);
#endif
type_t glb(type_t a, type_t b);

type_t leaftype_parent(type_t t);

#ifndef FLOP
void prune_glbcache();
#endif

#endif
