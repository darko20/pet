/* PET
 * Platform for Experimentation with effficient HPSG processing Techniques
 * (C) 1999 - 2001 Ulrich Callmeier uc@coli.uni-sb.de
 */

/* shared data structures, global variables and interface functions */

#ifndef _FLOP_H_
#define _FLOP_H_

#include <stdio.h>

#include <LEDA/list.h>
#include <LEDA/map.h>
#include <LEDA/graph.h>
#include <LEDA/d_int_set.h>
#include <LEDA/h_array.h>
#include <LEDA/array.h>

#include <list>
#include <string>
#include <vector>

#include "utility.h"
#include "list-int.h"
#include "symtab.h"
#include "builtins.h"
#include "dag.h"
#include "types.h"
#include "lex-tdl.h"
#include "settings.h"

#define FLOP_VERSION "FLO[P]P 0.2"

/***************************/
/* compile time parameters */
/***************************/

#define TDL_EXT ".tdl"
#define PRE_EXT ".pre"
#define OSF_EXT ".osf"
#define FFLEX_EXT ".ffl"
#define VOC_EXT ".voc"
#define DUMP_EXT ".gram"

#define TABLE_SIZE 20
#define COREF_TABLE_SIZE 32
#define LIST_TABLE_SIZE 12
#define AV_TABLE_SIZE 8 // this grows dynamically
#define CONJ_TABLE_SIZE 12

/* fixed upper limit for tables used for elements of conjunctions,
   lists, avms etc. */

// macro to ease conversion from L E D A to STL - can be used like the
// forall macro from L E D A, but only on list<int> containers

#define forallint(V,L) V=L.front(); for(list<int>::iterator _iterator = L.begin(); _iterator != L.end(); ++_iterator,V=*_iterator)

/***************************************/
/* global data - variables & constants */
/***************************************/

/*** flop.cc ***/

enum TERM_TAG {NONE, TYPE, ATOM, STRING, COREF, FEAT_TERM, LIST, DIFF_LIST, TEMPL_PAR, TEMPL_CALL};

extern symtab<struct type *> types;
extern symtab<int> statustable;
extern symtab<struct templ *> templates;
extern symtab<int> attributes;

extern int allow_redefinitions; /* will be set from lexer */

extern FILE *fstatus, *ferr;

extern int *leaftype_order, *rleaftype_order;
extern int *leaftypeparent;

extern settings *flop_settings;

/*** lex-tdl.cc ***/

/* `lexical tie ins' */
extern int lisp_mode;      /* should lexer recognize lisp expressions */
extern int builtin_mode;   /* should lexer translate builtin names  */

/*** parse-tdl.cc ***/

extern int syntax_errors;

/*** types.cc ***/
extern GRAPH<int,int> hierarchy;
extern leda_map<int,leda_node> type_node;
extern int *apptype;

/*** full-form.cc ***/
class morph_entry;
extern list<morph_entry> vocabulary;

/**************************/
/* global data structures */
/**************************/


/*** main data structure to represent TDL terms - close to the BNF ***/

struct attr_val 
{
  char *attr;
  struct conjunction *val;
};

struct avm
{
  int n;
  int is_special;
  int allocated;
  struct attr_val **av;
};

struct tdl_list
{
  int difflist;
  int openlist;
  int dottedpair;
  
  int n;
  struct conjunction **list;

  struct conjunction *rest; // for dotted pair
};

struct term
{
  enum TERM_TAG tag;

  char *value; // interpretation dependent on type
  int type;

  int coidx; // coreference index
  
  struct tdl_list *L;

  struct avm *A;

  struct param_list *params; // for templ_call
};

struct conjunction
{
  int n;
  struct term **term;
};

struct type
{
  int id;

  int status; // index into statustable
  bool defines_status;
  int status_giver;
  
  int implicit;
  
  struct lex_location *def;

  struct coref_table *coref;
  struct conjunction *constraint;

  class bitcode *bcode;
  struct dag_node *thedag;

  bool tdl_instance;

  list_int *parents; /* parents specified in definition */
};

/*** representation / processing of templates ***/

struct param
{
  char *name;
  
  struct conjunction *value;
};

struct param_list
{
  int n;
  struct param **param;
};

struct templ
{
  char *name;

  int calls;

  struct param_list *params;
  
  struct conjunction *constraint;
  struct coref_table *coref;

  struct lex_location *loc;
};

struct coref_table
{
  int n;
  char **coref;
};

/*** full-form lexicon generation ***/

class morph_entry
{
  string preterminal;
  
  string form;
  string stem;
  string affix;

  int inflpos;
  int nstems;

  string fname;
  int line;

 public:
  morph_entry(string pre, string fo, string st, string af, int ip, int ns, string fn = "unknown", int ln = 0) 
    : preterminal(pre), form(fo), stem(st), affix(af), inflpos(ip), nstems(ns), fname(fn), line(ln) { };
  
  morph_entry(const morph_entry &C)
    : preterminal(C.preterminal), form(C.form), stem(C.stem), affix(C.affix), inflpos(C.inflpos),
      nstems(C.nstems), fname(C.fname), line(C.line) { };

  morph_entry(string pre)
    : preterminal(pre), form(), stem(), affix(), inflpos(0), nstems(0), fname(), line(0) {};

  morph_entry() {};

  void setdef(string fn, int ln)
    {
      fname = fn; line = ln;
    }

  const string& key() { return preterminal; }

  friend void dump_le(dumper *f, const morph_entry &);

  friend int compare(const morph_entry &, const morph_entry &);

  friend ostream& operator<<(ostream& O, const morph_entry& C); 
  friend istream& operator>>(istream& I, morph_entry& C); 
};

inline bool operator==(const morph_entry &a, const morph_entry &b)
{
  return compare(a,b) == 0;
}

vector<string> get_le_stems(dag_node *le);

/********************************************************/
/* global functions - the interface between the modules */
/********************************************************/

/*** from util.cc ***/

int strcount(char *s, char c);
/* counts occurences of .c. in .s. - .c. must be != 0 */

void indent (FILE *f, int nr);
/* prints .nr. blanks on .f. */

struct type *new_type(const string &name, bool is_inst, bool define = true);
/* allocates memory for new type - returns pointer to initialized struct */

extern int Hash(const string &s);

/*** from full-form.cc ***/
bool read_morph(string fname);

/*** from template.cc ***/
void expand_templates();

/*** from corefs.cc ***/
int add_coref(struct coref_table *co, char *name);
void find_corefs();

/*** from print-chic.cc ***/
void print_constraint(FILE *f, struct type *t, const string &name);

/*** from print-tdl.cc ***/
void write_pre_header(FILE *outf, char *outfname, char *fname, char *grammar_version);
void write_pre(FILE *f);
void tdl_print_constraint(FILE *f, int i);
int tdl_print_conjunction(FILE *f, int level, struct conjunction *C, int typenr);

/*** from terms.cc ***/
struct avm *new_avm();
struct tdl_list *new_list();
struct term *new_term();
struct attr_val *new_attr_val();
struct term *new_type_term(int id);
struct term *new_string_term(char *s);
struct term *new_int_term(int i);
struct term *new_avm_term();
struct conjunction *new_conjunction();
struct term *add_term(struct conjunction *C, struct term *T);
struct attr_val *add_attr_val(struct avm *A, struct attr_val *av);
struct conjunction *add_conjunction(struct tdl_list *L, struct conjunction *C);
struct conjunction *get_feature(struct avm *A, char *feat);
struct conjunction *add_feature(struct avm *A, char *feat);

int nr_avm_constraints(struct conjunction *C);
struct term *single_avm_constraint(struct conjunction *C);

struct conjunction *copy_conjunction(struct conjunction *C);

/*** from builtins.cc ***/
void initialize_builtins();
int create_grammar_info(char *name, char *grammar_version, const char *vocabulary);

/*** from parse-tdl.cc ***/
void tdl_start(int toplevel);

/*** hierarchy.cc ***/
bool process_hierarchy();

void register_type(int s);
void subtype_constraint(int t1, int t2);
void undo_subtype_constraints(int t);

list<int> immediate_subtypes(int t);
list<int> immediate_supertypes(int t);

/*** expand.cc ***/

bool compute_appropriateness();
bool apply_appropriateness();

bool delta_expand_types();
bool fully_expand_types();
bool process_instances();

void compute_maxapp();
void compute_feat_sets(bool minimal);

void shrink_types();

/*** reduction.cc ***/
void ACYCLIC_TRANSITIVE_REDUCTION(const leda_graph& G, leda_edge_array<bool>& in_reduction);

/*** dump.cc ***/
void dump_grammar(dumper *f, char *desc);

/*** dag-tdl.cc ***/
struct dag_node *dagify_tdl_term(struct conjunction *C, int type, int crefs);
void dagify_symtabs();
void dagify_types();

#endif
