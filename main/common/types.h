/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
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
// `NO_STATUS', `ATOM_STATUS', `RULE_STATUS' and `LEXENTRY_STATUS'.

#define NO_STATUS 0
#define ATOM_STATUS 1
#define RULE_STATUS 2
#define LEXENTRY_STATUS 3

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


extern int nleaftypes, ntypes;
extern int first_leaftype;

extern int *typestatus;

extern char **typenames;

//
// attributes
//

extern int nattrs;
extern char **attrname;
extern int *attrnamelen; // for faster printing

extern int *apptype; // appropriate type for feature

// 
// codes: these are used to represent subtype relationships
// for proper types

void initialize_codes(int n);
void resize_codes(int n);
void register_codetype(const bitcode &b, int i);
void register_typecode(int i, bitcode *b);
int lookup_code(const bitcode &b);

void dump_hierarchy(dumper *f);

void undump_hierarchy(dumper *f);
void undump_tables(dumper *f);

int lookup_type(const char *s);
int lookup_attr(const char *s);

void dump_symbol_tables(dumper *f);
void undump_symbol_tables(dumper *f);

bool core_subtype(int a, int b);
bool subtype(int a, int b);
int glb(int a, int b);

int leaftype_parent(int t);

inline bool is_type(int a)
{
  return a >= 0 && a < ntypes;
}

#ifndef FLOP
void prune_glbcache();
#endif

#endif
