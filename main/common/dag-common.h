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

/** \file dag-common.h
 * Operations on dags shared between unifiers
 */

#ifndef _DAG_COMMON_H_
#define _DAG_COMMON_H_

#include "pet-config.h"
#include "list-int.h"
#include "dumper.h"
#include "types.h"

struct dag_node;

/** A dag node pointer representing unification failure */
extern struct dag_node *FAIL;

/** A flag to trigger the additional unification of typedags during the
 *  unification of two dags.
 *  If during unification the resulting dag node will get a more specific type
 *  than the two argument dags, the typedag of that type will also be unified
 *  with that node to check wellformedness, if this flag is set to \c true.
 */
extern bool unify_wellformed;

/** The array of typedags, i.e., the dags that correspond to the type's feature
 *  structure definition.
 */
extern dag_node **typedag;

/** cost of last unification - measured in number of nodes visited */
extern int unification_cost;

/** @name External (dumped) Representation
 */
/*@{*/
/** External dag node representation */
struct dag_node_dump
{
  /** The type id */
  int type;
  /** The number of attribute/value pairs of this node */
  short int nattrs;
};

/** External arc representation */
struct dag_arc_dump
{
  /** The attribute id */
  short int attr;
  /** The handle (number) of the dag node this arc points to */
  short int val;
};

void dump_node(dumper *, dag_node_dump *);
void undump_node(dumper *, dag_node_dump *);

void dump_arc(dumper *, dag_arc_dump *);
void undump_arc(dumper *, dag_arc_dump *);
/*@}*/

/** @name Fixed Arity Representation Of Dags
 */
/*@{*/
/** is this an encoding that guarantees no casts? */
extern bool dag_nocasts;

/** gives featset id for each type */
extern int *featset;

/** describes features of one feature set */
struct featsetdescriptor
{
  /** number of features */
  short int n;
  /** feature id for each position */
  short int *attr;
};

/** total number of featsets */
extern int nfeatsets;

/** vector of descriptors for each feature set */
extern featsetdescriptor *featsetdesc;
/*@}*/

/** @name Constraint cache
 * Cache copied typedags that have been used during wellformedness enforcement
 * but have been returned because the overall unification failed.
 */
/*@{*/
/** A single linked list of typedags and a generation counter to determine if
 *  this element of the list is used in the ongoing unification.
 */
struct constraint_info
{
  /** The typedag for this info */
  dag_node *dag;
  /** The generation counter indicating if this info is used */
  int gen;
  /** The list structure link */
  struct constraint_info *next;
};

/** The constraint cache: an array containing as many elements as there are
 *  static types
 */
extern constraint_info **constraint_cache;
/*@}*/

/** Create a new dag node with type \a s */
dag_node *new_dag(type_t s);

#ifdef DYNAMIC_SYMBOLS
/** For dynamic symbol, create a simple new typedag */
inline dag_node *type_dag(type_t type) {
  return is_static_type(type) ? typedag[type] : new_dag(type) ;
}
#else
/** Return the type dag for the static type */
inline dag_node *type_dag(type_t type) { return typedag[type]; }
//#define type_dag(type) typedag[type]
#endif

/** Initialize the typedag array to \a n dag node pointer of value \c NULL */
void initialize_dags(int n);
/** Set the typedag of type \a t to \a dag */
void register_dag(type_t t, dag_node *dag);

/** Undump a dag from \a f */
dag_node *dag_undump(dumper *f);

/** Look up the attribute \a attr in the arc list of \a dag.
 *  \return If \a attr is a known attribute and an arc with attribute \a attr
 *  was found, the value of that arc, \c FAIL otherwise. 
 */
dag_node *dag_get_attr_value(dag_node *dag, const char *attr);

/** Add a new arc with attribute \a attr to the dag \a val.
 *  \return The dag node the new arc points to, if \a attr is a known
 *          attribute, and has a valid appropriate type. The value of the new
 *          arc will be a copy of the typedag of the attribute's appropriate
 *          type.
 */
dag_node *dag_create_attr_value(const char *attr, dag_node *val);

/** Add a new arc with attribute \a attr to the dag \a val.
 *  \return The dag node the new arc points to, if \a attr is a known
 *          attribute, and has a valid appropriate type. The value of the new
 *          arc will be a copy of the typedag of the attribute's appropriate
 *          type.
 */
dag_node *dag_create_attr_value(attr_t attr, dag_node *val);

/** Try to follow the list of arcs specified in \a path, starting at \a dag
 *  \a path must to be a list of (known) attribute names, separated by dots.
 *  \return the dag node at the end of the path, if it exists, \c FAIL
 *          otherwise.
 */
dag_node *dag_get_path_value(dag_node *dag, const char *path);

/** Follow the \a path of attributes starting at \a dag and return the
 *  node at the end, if it exists, \c FAIL otherwise.
 */
dag_node *dag_get_path_value(dag_node *dag, list_int *path);

/** Create a minimal feature structure that contains the arc path
 *  \a path and where the node at the bottom has type \a type.
 *  \a path must to be a list of (known) attribute names, separated by dots.
 *  \return The root node of the new dag on success, \c FAIL otherwise
 */
dag_node *dag_create_path_value(const char *path, type_t type);

/** Create a minimal feature structure that contains the arc path \a path and
 * where the node at the bottom has type \a type. 
 * \a path must be a list of valid attribute IDs
 *  \return The root node of the new dag on success, \c FAIL otherwise
 */
dag_node *dag_create_path_value(list_int *path, type_t type);

/** Create a minimal feature structure that contains the arc path \a path and
 * where the node at the bottom has type \a type. 
 * \a path must be a list of valid attribute IDs
 *  \return The root node of the new dag on success, \c FAIL otherwise
 */
dag_node *dag_create_path_value(list_int *path, struct dag_node *dag);

/** Convert a dot-separated list of attributes into a list of attributes ids
 *  \return the pointer to the list if all attributes are known, \c NULL
 *          otherwise 
 */
list_int *path_to_lpath(const char *path);

/** Convert a list encoded in a feature structure into a STL list of dag
 *  nodes.
 */
std::list<dag_node *> dag_get_list(dag_node* first);

/** Build a feature structure list from \a l in reverse order.
 *  A list in feature structure encoding will be produced, with the \c FIRST
 *  arcs pointing to (empty) dags getting the types in l.
 *  \attention As already said, the FS list is in reverse order.
 */ 
dag_node *dag_listify_ints(list_int *l);

/**
 * Get the substructure under <attr>.REST*(n-1).FIRST if it exists, \c FAIL
 * otherwise, where \a attr is the code for the attribute <attr> .
 */
dag_node *dag_nth_element(struct dag_node *dag, int attr, int n);

/**
 * Get the substructure under <path>.REST*(n-1).FIRST if it exists, \c FAIL
 * otherwise, where \a path contains a list of the attribute's codes that
 * form the path <path>.
 */
dag_node *dag_nth_element(struct dag_node *dag, list_int *path, int n);

/** Get the substructure under ARGS.REST*(n-1).FIRST if it exists, \c FAIL
 *  otherwise. 
 */
dag_node *dag_nth_arg(dag_node *dag, int n);

/** \brief Set the \c visit field of every dag node in \a dag to the number of
 *  incoming arcs. Any node with a \c visit field of more than one is 
 *  `coreferenced'.
 */
void dag_mark_coreferences(dag_node *dag);

/** Print \a dag readably in OSF format to \a file */
void dag_print(FILE *f, dag_node *dag);

/** Return the dag size in number of nodes */
int dag_size(dag_node *dag);

/** Destructively remove all arcs emerging from \a dag that bear an attribute
 *  present in the \a del list
 */
void dag_remove_arcs(dag_node *dag, list_int *del);

/** @name Common Interface
 * A common dag interface. It is defined here, the implementation is in 
 * separate files.
 */
/*@{*/
/** Initialize dag storage. */
void dag_initialize();
/** Destroy dag storage at the end of the process. */
void dag_finalize();

/** \brief Initialize single dag node. Called during creation of a node. */
void dag_init(dag_node *dag, type_t type);

/*@{*/
/** Return the type of \a dag */
type_t dag_type(dag_node *dag);
/** Set the type of \a dag to \a s */
void dag_set_type(dag_node *dag, type_t s);
/*@}*/

/*@{*/
/** \brief Search in the arcs list of \a dag for an arc with attribute \a
 *  attr. Return the value of this arc if it exists, \c FAIL otherwise.
 */
dag_node *dag_get_attr_value(dag_node *dag, attr_t attr);
/**  \brief Search in the arcs list of \a dag for an arc with attribute \a
 *  attr. Set its value to \a val, if it exist.
 *
 * \return \c true, if an appropriate arc was found, \c false otherwise.
 */
bool dag_set_attr_value(dag_node *dag, attr_t attr, dag_node *val);
/*@}*/

/** Clone \a dag completely (make a deep copy) */
dag_node *dag_full_copy(dag_node *dag);

/** Unify \a dag2 into \a root at subdag \a dag1.  
 *
 * If the unification succeeds, return a (possibly partial, i.e., structure
 * sharing) copy of the result, omitting all arcs at the root node that bear
 * attributes contained in \a del.
 *
 * \param root The root node of the dag to unify into
 * \param dag1 The subdag node under \a root where the unification should be
 *             performed.
 * \param dag2 The dag to unify with the subdag of \a root
 * \param del  A list of attributes to specify the arcs to be deleted at the
 *             root node of the result.
 * \return A copy of the result, if the unificaton succeeds, \c FAIL otherwise.
 */
dag_node *
dag_unify(dag_node *root, dag_node *dag1, dag_node *dag2, list_int *del);


/** Unify \a arg into \a root at subdag under path \a path.  
 *
 * If the path is not (fully) present in the feature structure \a root, try to
 * add it. If the unification succeeds, return a (possibly partial, i.e.,
 * structure sharing) copy of the result.
 */
struct dag_node *
dag_unify(dag_node *root, dag_node *arg, list_int *path);


/** Return \c true if \a dag1 and \a dag2 are unifiable, \c false otherwise.
 *  \a dag1 and \a dag2 are not modified.
 */
bool dags_compatible(dag_node *dag1, dag_node *dag2);

/** Do a bidirectional subsumption test on \a dag1 and \a dag2.
 *  \return \c true in \a forward, if \a dag1 subsumes (is more general than)
 *          \a dag2, \c true in \a backward, if \a dag1 is subsumed by \a dag2
 */
void 
dag_subsumes(dag_node *dag1, dag_node *dag2, bool &forward, bool &backward);

/** Make \a dag totally wellformed.
 *
 *  At every node of \a dag, the node is unified with the typedag of its type.
 *  This typedag may be unfilled, and therefore it is possibly reconstructed by
 *  unifying the typedags of all of its supertypes.
 *
 *  \return A fully expanded copy of \a dag when all unifications succeed, \c
 *          FAIL otherwise.
 */
dag_node *dag_expand(dag_node *dag);

struct qc_node;

/** Read the quick check structure from \a f.
 *
 * The quick check structure ist a tree representing a list of paths. Every qc
 * node at the end of such a path bears a number representing the position of
 * the path in the list. The arc labels are all valid attribute IDs.
 *
 * \param f     The dumper to read from
 * \param limit Only store paths whose list position is not beyond \a limit. 
 *              Values below zero indicate that all stored paths should be
 *              taken .
 * \param qc_len Return the number of stored paths in this variable.
 * \return The quick check tree, and the number of paths it contains in \a
 *         qc_len 
 */
qc_node *dag_read_qc_paths(dumper *f, int limit, int &qc_len);

/** Fill the quick check vector \a qc_vector from \a dag, using the quick check
 *  tree under \a root.
 *
 * Recursively walk the quick check tree \a root in parallel to \a dag. When a
 * quick check node bearing a nonnegative number \c i is encountered, the dag
 * type of the current dag node is stored into the \a qc_vector at index
 * position \c i.
 * \pre \a qc_vector must be of appropriate length. This is not tested.
 * \return A quick check vector of \a dag in \a qc_vector.
 */
void dag_get_qc_vector(qc_node *root, dag_node *dag, type_t *qc_vector);

/** Release the global data structures related to the quick check tree */
void dag_qc_free();

#endif
