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

/* shared data structures, global variables and interface functions */

#ifndef _FLOP_H_
#define _FLOP_H_

#include <stdio.h>
#include <locale.h>

#include <LEDA/list.h>
#include <LEDA/map.h>
#include <LEDA/graph.h>
#include <LEDA/d_int_set.h>
#include <LEDA/h_array.h>
#include <LEDA/array.h>

#include <list>
#include <string>
#include <vector>
#include <map>

#include "utility.h"
#include "list-int.h"
#include "symtab.h"
#include "builtins.h"
#include "dag.h"
#include "types.h"
#include "lex-tdl.h"
#include "options.h"
#include "settings.h"
#include "grammar-dump.h"

#define FLOP_VERSION "FLO[P]P 1.0"

/***************************/
/* compile time parameters */
/***************************/

#define TDL_EXT ".tdl"
#define PRE_EXT ".pre"
#define VOC_EXT ".voc"
#define IRR_EXT ".tab"

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

extern char *global_inflrs;

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
extern list<class ff_entry> fullforms;
extern list<class irreg_entry> irregforms;

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

  char *printname; // name as defined in grammar, including capitalization etc

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

  char *inflr;

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

class ff_entry
{
 public:
  ff_entry(string pre, string af, string fo, int ip,
	   string fn = "unknown", int ln = 0) 
    : _preterminal(pre), _affix(af), _form(fo), _inflpos(ip),
    _fname(fn), _line(ln)
    {};
  
  ff_entry(const ff_entry &C)
    : _preterminal(C._preterminal), _affix(C._affix), _form(C._form),
    _inflpos(C._inflpos), _fname(C._fname), _line(C._line)
    {};

  ff_entry(string pre)
    : _preterminal(pre)
    {};

  ff_entry() {};

  void setdef(string fn, int ln)
    {
      _fname = fn; _line = ln;
    }

  const string& key() { return _preterminal; }

  void dump(dumper *f);

  friend int compare(const ff_entry &, const ff_entry &);

  friend ostream& operator<<(ostream& O, const ff_entry& C); 
  friend istream& operator>>(istream& I, ff_entry& C); 

private:
  string _preterminal;
  string _affix;
  
  string _form;

  int _inflpos;

  string _fname;
  int _line;
};

inline bool operator==(const ff_entry &a, const ff_entry &b)
{
  return compare(a,b) == 0;
}

vector<string> get_le_stems(dag_node *le);

class irreg_entry
{
 public:
  irreg_entry(string fo, string in, string st)
    : _form(fo), _infl(in), _stem(st) {}
  
  void dump(dumper *f);

 private:
  string _form, _infl, _stem;

};

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

char *add_inflr(char *old, char *add);

extern int Hash(const string &s);

/*** from full-form.cc ***/
void read_morph(string fname);
void read_irregs(string fname);

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
int create_grammar_info(char *name, char *grammar_version);

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

void unfill_types();

/*** reduction.cc ***/
void ACYCLIC_TRANSITIVE_REDUCTION(const leda_graph& G, leda_edge_array<bool>& in_reduction);

/*** dump.cc ***/
void dump_grammar(dumper *f, char *desc);

/*** dag-tdl.cc ***/
struct dag_node *dagify_tdl_term(struct conjunction *C, int type, int crefs);
void dagify_symtabs();
void dagify_types();

#endif
