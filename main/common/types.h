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

/** \file types.h
 * Types and attributes.
 */

#ifndef _TYPES_H_
#define _TYPES_H_

#include "pet-config.h"
#include "builtins.h"
#include <cassert>
#include <vector>
#include <string> 
#include <boost/lexical_cast.hpp>

// types, attributes, and status are represented by (small) integers

/** @name Status
 * A status value is stored for each type. predefined status values
 * are `NO_STATUS' and `ATOM_STATUS'.  
 */
/*@{*/

#define NO_STATUS 0
#define ATOM_STATUS 1

/// number of status values
extern int nstatus;

/// names of status values, [0 .. nstatus[
extern char **statusnames;
/*@}*/

/** @name Types
 * 
 * Types that are defined in the grammar (including automatically constructed
 * glb-types) are called static types. Types that are added at run-time are
 * called dynamic types. Types with exactly one parent and without any
 * substructure are called leaf types. Dynamic types are sub-types of
 * BI_STRING, and are therefore always leaf types.
 * 
 * The universe of static types \f$ [0 \ldots nstatictypes[ \f$ is split into two
 * consecutive, disjunct ranges:
 *   -# proper types        \f$ [ 0 \ldots first_leaftype [ \f$
 *   -# (static) leaf types \f$ [ first_leaftype \ldots nstatictypes [ \f$
 * Proper types have a full bitcode representation; for leaf types only the
 * parent type is stored. A status value is stored for each static type.
 * 
 */
/*@{*/

typedef int type_t;
typedef int attr_t;

#define T_BOTTOM ((type_t) -1)       /// failure of type unification

/** The number of static leaf types */
extern int nstaticleaftypes;
/** The number of all static types */
extern int nstatictypes;
/** The id of the first leaf type */
extern type_t first_leaftype;
/** The number of all known types. All type id's have to be smaller than
 *  this. If dynamic types are not used, this is equal to nstatictypes. */
extern type_t ntypes;

/** A vector of status values (for each static type) */
extern int *typestatus;

/** An internal name, differing from the name given in the grammar e.g. for
 *  instances, which are prepended with a '$' character, and strings, which
 *  are enclosed in double quotes.
 *  \see type_name()
 */
extern std::vector<std::string> typenames;
/** preferred way to print a type name
 *  \see print_name()
 */
extern std::vector<std::string> printnames;
/*@}*/

/** @name Attributes */
/*@{*/
/** The number of attributes (fixed by the grammar) */
extern int nattrs;
/** The vector of attribute names */
extern char **attrname;
/** The lenghts of the attribute names for faster printing */
extern int *attrnamelen;

/** appropriate type for feature, i.e., the topmost type that introduces a
 * feature
 */
extern type_t *apptype;
/** maximal appropriate type under feature, i.e., the most general type that a
 * node under this feature may bear.
 */
extern type_t *maxapp;
/*@}*/

/** @name Codes
 * these are used to represent subtype relationships for proper types.
 */
/*@{*/
/** Allocate space for \a n codes and initialize global data structures */
void initialize_codes(int n);
/** Increase the size of all codes to \a n bits */
void resize_codes(int n);
/** Tie a code to its type id */
void register_codetype(const class bitcode &b, type_t i);
/** Tie the type id to the bitcode */
void register_typecode(type_t i, class bitcode *b);
/** Return the type id of code \a b */
type_t lookup_code(const class bitcode &b);
/*@}*/

#ifndef FLOP
#include <list>

/** Immediate supertypes for each non-leaftype. */
extern std::vector< std::list<int> > immediateSupertype;

#else // FLOP
/*@{*/
/** Tables containing a reordering function for types, from the new (cheap) 
 *  type id to the old (flop) one and the inverse.
 */
extern int *cheap2flop, *flop2cheap;
/*@}*/
#endif

/** Dump the hierarchy to \a f in binary format (bitcodes and leaftypeparents).
 */
void dump_hierarchy(class dumper *f);

/** Load the hierarchy from \a f in binary format (bitcodes and
 *  leaftypeparents).
 */
void undump_hierarchy(class dumper *f);
/** Load the type-to-featureset mappings and the feature sets for fixed arity
 *  encoding, as well as the table of appropriate types for all attributes.
 */
void undump_tables(class dumper *f);
/** Initialize the table for the maximal appropriate type under a feature by
 *  consulting the type dag of the appropriate type.
 * \todo this has got nothing to do with the `pure' type hierarchy and should
 * go to where the dag(s) hierarchy is initialized/undumped
 */
void initialize_maxapp();

/** Free tables for names, status, codes etc. of types and attributes. */
void free_type_tables();

int lookup_status(const char *s);
/** Get the attribute id for attribute name \a s, or -1, if it does not
    exist */
attr_t lookup_attr(const char *s);

/** Get the type id for type name \a name or return -1 if it does not exist. */
type_t lookup_type(const std::string &name);
/** Get the type id for type name \a name, registering a new type for unknown
 *  string types (if dynamic types are enabled, i.e. DYNAMIC_TYPES is defined),
 *  or return -1 if it does not exist.
 */
type_t retrieve_type(const std::string &name);
/** Get the type id for a string instance \a s.
 *  The corresponding type name for \a s is embedded in double quotes.
 *  If dynamic types are enabled (i.e. DYNAMIC_TYPES is defined) and there is
 *  no type for string \a s, register it as a new dynamic type. 
 */
inline type_t retrieve_string_instance(const std::string &s) {
  return retrieve_type('"' + s + '"');
}
/** Get the type id for the string representation of int \a i
 *  (all ints are stored as subtypes of BI_STRING).
 *  If dynamic types are enabled (i.e. DYNAMIC_TYPES is defined) and there is
 *  no type for int \a i, register it as a new dynamic type. 
 */
inline type_t retrieve_string_instance(int i) {
  return retrieve_type('"' + boost::lexical_cast<std::string>(i) + '"');
}

#ifdef DYNAMIC_SYMBOLS
/** Register a dynamic type.
 * \pre a type with typename \a name has not been registered
 * \pre typename \a name denotes a string
 * \see retrieve_type()
 */
type_t register_dynamic_type(const std::string &name);

/** Clear all dynamic types (might be called for each new parse) */
void clear_dynamic_types();
#endif

/** Get the type name of a type */
inline std::string get_typename(type_t type) {
  return typenames[type];
}
/** Get the print name of a type */
inline std::string get_printname(type_t type) {
  return printnames[type];
}
/** Get the type name of a type
 *  \deprecated use get_typename()
 */
inline const char *type_name(type_t type) {
  return typenames[type].c_str();
}
/** Get the print name of a type
 *  \deprecated use get_printname()
 */
inline const char *print_name(type_t type) {
  return printnames[type].c_str();
}

/** Dump information about the number of status values, leaftypes, types, and
 *  attributes as well as the tables of status value names, type names and
 *  their status, and attribute names.
 */
void dump_symbol_tables(class dumper *f);
/** Load information about the number of status values, leaftypes, types, and
 *  attributes as well as the tables of status value names, type names and
 *  their status, and attribute names.
 */
void undump_symbol_tables(class dumper *f);
/** Load the print names of types (not all differ from the internal names). */
void undump_printnames(class dumper *f);

#ifndef FLOP
/** Load a graph-based representation of the immediate supertypes from \a f */
void
undumpSupertypes(class dumper *f);

/** Return the list of immediate supertypes of proper type \a type.
 * \pre \a type must be a proper type.
 */
const std::list<type_t> &immediate_supertypes(type_t type);

/** Return the list of all supertypes of \a type including the type itself but
 * excluding \c *top*.
 */
const std::list<type_t> &all_supertypes(type_t type);
#endif

/** Return \c true if \a a is a subtype of \a b, computed using the
 *  bitcodes, which works only for proper types, not leaf types.
 */
bool core_subtype(type_t a, type_t b);
/** Return \c true if \a a is a subtype of \a b, for any two types \a a and \a
 *  b, no matter if static, dynamic, leaf or whatsoever.
 */
bool subtype(type_t a, type_t b);
#ifndef FLOP
/** Compute the subtype relations for \a a and \a b in parallel.
 * \a forward is set to \c true if \a a is subtype of \a b,
 * \a backward analogously.
 * 
 * \pre  \a a != \a b, \a a >= 0, \a b >= 0
 * \param[out] forward  value as returned by \a subtype(a, b)
 * \param[out] backward value as returned by \a subtype(b, a)
 */
void
subtype_bidir(type_t a, type_t b, bool &forward, bool &backward);
#endif

/** \brief Compute the greatest lower bound in the type hierarchy of \a a and 
 *  \a b. Return -1 if the types are incompatible.
 */
type_t glb(type_t a, type_t b);

/** Return the parent of \a t, if it is a leaf type, \a t itself otherwise */
type_t leaftype_parent(type_t t);

#ifndef FLOP
/** Empty the glb/subtype cache to save space. */
void prune_glbcache();
#endif

/** Check the validity of type code \a a. */
inline bool is_type(type_t a) {
  return a >= 0 && a < ntypes;
}

/** Return \c true if type \a a is a proper type (i.e., a static non-leaf). */
inline bool is_proper_type(type_t a) {
  assert(is_type(a));
  return a < first_leaftype;
}

/** Return \c true if type \a a is a (static or dynamic) leaf type. */
inline bool is_leaftype(type_t a) {
  assert(is_type(a));
  return a >= first_leaftype;
}

/** Return \c true if type code \a a is a type from the hierarchy and not a
 *  dynamic type.
 */
inline bool is_static_type(type_t a) {
  assert(a >= 0);
  return a < nstatictypes;
}

/** Return \c true if type code \a a is a dynamic type. */
inline bool is_dynamic_type(type_t a) {
  assert(is_type(a));
  return a >= nstatictypes;
}

/** Return \c true if type code \a a is a string instance, i.e. a subtype of
 * string but not string itself. */
inline bool is_string_instance(type_t a) {
  return subtype(a, BI_STRING) && a != BI_STRING;
}

/** check the validity of the attribute \a attr */
inline bool is_attr(attr_t attr) {
  assert(attr >= 0);
  return (attr <= nattrs);
}

#endif
