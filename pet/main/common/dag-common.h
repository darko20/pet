/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* operations on dags shared between unifiers */

#ifndef _DAG_COMMON_H_
#define _DAG_COMMON_H_

#include "list-int.h"
#include "dumper.h"
#include "types.h"

struct dag_node;

extern struct dag_node *FAIL;

extern bool unify_wellformed;

extern struct dag_node **typedag;

extern vector<dag_node *> dyntypedag;  // dags for dynamic types

// cost of last unification - measured in number of nodes visited
extern int unification_cost;

//
// external (dumped) representation
//

struct dag_node_dump
{
  type_t type;
  short int nattrs;
};

struct dag_arc_dump
{
  short int attr;
  short int val;
};

void dump_node(dumper *, dag_node_dump *);
void undump_node(dumper *, dag_node_dump *);

void dump_arc(dumper *, dag_arc_dump *);
void undump_arc(dumper *, dag_arc_dump *);

//
// fixed arity representation of dags */
//

// is this an encoding that guarantees no casts?
extern bool dag_nocasts;

// gives featset id for each type
extern int *featset;

// describes features of one feature set
struct featsetdescriptor
{
  short int n;       // number of features
  short int *attr;   // feature id for each position
};

// total number of featsets
extern int nfeatsets;

// vector of descriptors for each feature set
extern featsetdescriptor *featsetdesc;

//
// constraint cache
//

struct constraint_info
{
  dag_node *dag;
  int gen;
  struct constraint_info *next;
};

extern constraint_info **constraint_cache;

void initialize_dags(int n);
void register_dag(int i, struct dag_node *dag);

struct dag_node *dag_undump(dumper *f);

struct dag_node *dag_get_attr_value(struct dag_node *dag, const char *attr);
struct dag_node *dag_create_attr_value(const char *attr, dag_node *val);

struct dag_node *dag_get_path_value(struct dag_node *dag, const char *path);
struct dag_node *dag_create_path_value(const char *path, int type);

struct dag_node *dag_get_path_value_l(struct dag_node *dag, list_int *path);
list<struct dag_node *> dag_get_list(struct dag_node* first);
dag_node *dag_listify_ints(list_int *);
struct dag_node *dag_nth_arg(struct dag_node *dag, int n);

//
// interface defined here, implementation in seperate files
//

void dag_initialize();

void dag_mark_coreferences(struct dag_node *dag);
void dag_print(FILE *f, struct dag_node *dag);
int dag_size(dag_node *dag);

void dag_init(dag_node *dag, int type);

int dag_type(struct dag_node *dag);
void dag_set_type(struct dag_node *dag, int s);
bool dag_framed(struct dag_node *dag);

struct dag_node *dag_get_attr_value(struct dag_node *dag, int attr);
bool dag_set_attr_value(struct dag_node *dag, int attr, dag_node *val);

struct dag_node *dag_full_copy(dag_node *dag);
struct dag_node *dag_unify(dag_node *root, dag_node *dag1, dag_node *dag2, list_int *del);
bool dags_compatible(struct dag_node *dag1, struct dag_node *dag2);

void dag_subsumes(dag_node *dag1, dag_node *dag2, bool &forward, bool &backward);

// makes dag totally wellformed
dag_node *dag_expand(dag_node *dag);

void dag_remove_arcs(struct dag_node *dag, list_int *del);

struct qc_node;

struct qc_node *dag_read_qc_paths(dumper *f, int limit, int &qc_len);
void dag_get_qc_vector(struct qc_node *, struct dag_node *dag, type_t *qc_vector);
void dag_qc_free();

#endif
