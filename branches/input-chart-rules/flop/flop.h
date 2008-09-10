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

/** \file flop.h 
 * shared data structures, global variables and interface functions
 */

#ifndef _FLOP_H_
#define _FLOP_H_

#include "symtab.h"
#include "types.h"
#include <list>

/***************************/
/* compile time parameters */
/***************************/

/** Default extension of TDL files */
#define TDL_EXT ".tdl"
/** Default extension of syntactically preprocessed grammar files
 * (\c -pre option)
 */
#define PRE_EXT ".pre"
/** Default extension of full form files */
#define VOC_EXT ".voc"
/** Default extension of irregular form files */
#define IRR_EXT ".tab"

/** @name Limits
 * fixed upper limit for tables used for elements of conjunctions,
 * lists, avms etc. while reading TDL files
 */
/*@{*/
/** Table size for parameters and nesting of templates */
#define TABLE_SIZE 20
/** _fix_me_ Fixed size table for coreferences */
#define COREF_TABLE_SIZE 32
/** _fix_me_ Fixed size table for conjunctions */
#define LIST_TABLE_SIZE 12
/** Table size for attribute value lists: this grows dynamically */
#define AV_TABLE_SIZE 8
/** _fix_me_ Fixed size table for terms */
#define CONJ_TABLE_SIZE 20
/*@}*/


/// macro to ease conversion from L E D A to STL - can be used like the
/// forall macro from L E D A, but only on list<int> containers

#define forallint(V,L) V=L.front(); for(list<int>::iterator _iterator = L.begin(); _iterator != L.end(); ++_iterator,V=*_iterator)

/* Global Data: variables & constants */
/** flop.cc: kinds of terms */
enum TERM_TAG {NONE, TYPE, ATOM, STRING, COREF, FEAT_TERM, LIST, DIFF_LIST,
               TEMPL_PAR, TEMPL_CALL};

/** @name Symbol Maps
 * Global tables mapping strings to data structures
 */
/*@{*/
/** Map name to type */
extern symtab<struct type *> types;
/** Map name to type status */
extern symtab<int> statustable;
/** Map name to template */
extern symtab<struct templ *> templates;
/** Map name to attribute */
extern symtab<int> attributes;
/*@}*/

extern char *global_inflrs;

/** \brief If zero, type redefinitions will issue a warning. Will be set from
 * lexer
 */
extern int allow_redefinitions;

/** Output for status messages. */
extern FILE *fstatus;
/** Output for error messages. */
extern FILE *ferr;

/*@{*/
/** Tables containing a reordering function for types, from the old type id to
 *  the new one and the inverse .
 */
extern int *leaftype_order, *rleaftype_order;
/*@}*/
/** A table containing the parent of a leaf type, if greater than zero.
 *  \code leaftypeparent[type] < 0 \endcode is a predicate for testing
 *  if type is not a leaf type (after expansion).
 */
extern int *leaftypeparent;

/** The settings contained in flop.set and its includes. */
extern class settings *flop_settings;

/** lex-tdl.cpp : `lexical tie ins': should lexer recognize lisp expressions?
 */
extern int lisp_mode;
/* should lexer translate builtin names  */
//extern int builtin_mode;

/** from parse-tdl.cc: The number of syntax errors */
extern int syntax_errors;

/** Array \c apptype is filled with the maximally appropriate types, i.e., \c
 * apptype[j] is the first subtype of \c *top* that introduces feature \c j.
 * in \c types.cc 
 */
extern type_t *apptype;

/** @name full-form.cc */
/*@{*/
/** List of full form entries */
extern std::list<class ff_entry> fullforms;
/** List of irregular form entries */
extern std::list<class irreg_entry> irregforms;
/*@}*/

/**************************/
/* global data structures */
/**************************/

/* main data structures to represent TDL terms - close to the BNF */

/** An element of the attribute value list for TDL input */
struct attr_val 
{
  /** The attribute string */
  char *attr;
  /** The conjunction this arc points to */
  struct conjunction *val;
};

/** An attribute value matrix for TDL input */
struct avm
{
  /** The number of elements active in the \c attr_val list */
  int n;
  /** _fix_me_ This slot seems to be out of use. */
  int is_special;
  /** \brief The number of elements allocated in the \c attr_val list.
   *  This is always bigger than avm::n.
   */
  int allocated;
  /** A list of attribute value pairs */
  struct attr_val **av;
};

/** The internal representation of a TDL list definition for TDL input */
struct tdl_list
{
  /** When not zero, this is a difference list */
  int difflist;
  /** When not zero, this is an open list (the last \c REST does not point to a
   *  \c *NULL* value).
   */
  int openlist;
  /** When not zero, the last \c REST does not point node with type \c CONS or
   *  \c *NULL*, but to an ordinary AVM
   */
  int dottedpair;
  
  /** The number of elements in this list */
  int n;
  /** The members of the list itself */
  struct conjunction **list;

  /** The conjunction pointed to by the last \c REST arc for dotted pair */
  struct conjunction *rest;
};

/** A TDL feature term for TDL input */
struct term
{
  /** The type of the term */
  enum TERM_TAG tag;

  /** interpretation depens on the term type */
  char *value;
  /** the type id of the term */
  int type;

  /** coreference index */
  int coidx;
  
  /** contains the list if \c tag is one of \c LIST or \c DIFF_LIST */
  struct tdl_list *L;

  /** contains avm if \c tag is \c FEAT_TERM */
  struct avm *A;

  /** for template call (templ_call) */
  struct param_list *params;
};

/** Representation for a conjunction of terms for TDL input */
struct conjunction
{
  /** The number of terms in the \c term list */
  int n;
  /** List of terms */
  struct term **term;
};

/** Representation of a type definition. for TDL input */
struct type
{
  /** Numeric type id */
  int id;

  /** name as defined in grammar, including capitalization etc */
  char *printname;

  /** index into statustable */
  int status;

  /** @name status inheritance 
   * Old tdl mechanism of status inheritance. Only activated when the
   * command line option \c -propagate-status is used.
   * 
   * \todo should be made obsolete and removed in favor of the
   * \code :begin :type|:instance :status :foo \endcode mechanism
   */
  /*@{*/
  /** if \c true, this type bears a status it propagates to its subtypes */
  bool defines_status;
  /** type that determines the status of this type */
  int status_giver;
  /*@}*/

  /** This flag is \c true if there was no definition for this type yet, but it
   *  was introduced because it occured on the right hand side of some other
   *  type's definition.
   */
  int implicit;
  
  /** Location of this type's definition (file name, line number, etc.) */
  struct lex_location *def;

  /** Table of coreferences for this type definition */
  struct coref_table *coref;
  /** Internal representation of this type's TDL definition */
  struct conjunction *constraint;

  /** The bitcode representing this type in the hierarchy */
  class bitcode *bcode;
  /** The dag resulting from the type definition */
  struct dag_node *thedag;

  /** \c true if this type is a TDL instance, which means that the type in the
   *  root node of the dag is not the same as the name of the type.
   */
  bool tdl_instance;

  /** The regular inflection rule string associated with this type */
  char *inflr;

  /** parents specified in definition */
  struct list_int *parents;
};

/** @name Templates
 * representation / processing of templates for TDL input
 */
/*@{*/
/** A parameter name/value pair */
struct param
{
  char *name;
  
  struct conjunction *value;
};

/** A list of template parameters for TDL input */
struct param_list
{
  int n;
  struct param **param;
};

/** Internal representation of template definition for TDL input */
struct templ
{
  /** The name of the template */
  char *name;

  /** The number of times this template has been called */
  int calls;

  /** The list of formal parameters of this template */
  struct param_list *params;
  
  /** Internal representation of the right side of the TDL definition */
  struct conjunction *constraint;
  /** Table mapping coreference names in this definition to numbers */
  struct coref_table *coref;

  /** Location of this template's definition (file name, line number, etc.) */
  struct lex_location *loc;
};

/** Table mapping coreference names to numbers for TDL input */
struct coref_table
{
  /** Number of elements in the table */
  int n;
  /** List of coreference names */
  char **coref;
};
/*@}*/

/** @name Full-Form Lexicon */
/*@{*/

/** full form entry with lexical type and inflection information */
class ff_entry
{
 public:
  /** Commonly used constructor.
   *  \param preterminal The type name corresponding to the base form
   *  \param affix       The inflection rule to apply
   *  \param form        The surface form
   *  \param inflpos     The inflected position (only relevant for multi word
   *                     entries 
   *  \param filename    The filename of the full form file currently read
   *  \param line        The line number of this full form definition
   */
  ff_entry(std::string preterminal, std::string affix, std::string form,
           int inflpos, std::string filename = "unknown", int line = 0) 
    : _preterminal(preterminal), _affix(affix), _form(form), _inflpos(inflpos),
    _fname(filename), _line(line)
    {};
  
  /** Copy constructor */
  ff_entry(const ff_entry &C)
    : _preterminal(C._preterminal), _affix(C._affix), _form(C._form),
    _inflpos(C._inflpos), _fname(C._fname), _line(C._line)
    {};

  /** Constructor setting only preterminal \a pre (type name of base form) */
  ff_entry(std::string pre)
    : _preterminal(pre)
    {};

  /** Default constructor */
  ff_entry() {};

  /** Set the location were this full form entry was specified (file name and
   *  line number).
   */
  void setdef(std::string fn, int ln)
    {
      _fname = fn; _line = ln;
    }

  /** Return the key (type name) of this full form entry */
  const std::string& key() { return _preterminal; }

  /** Dump entry in a binary representation to \a f */
  void dump(dumper *f);

  /** Lexically compare the preterminal strings (type names of base forms) */
  friend int compare(const ff_entry &, const ff_entry &);

  /** Readable representation of full form entry for debugging */
  friend std::ostream& operator<<(std::ostream& O, const ff_entry& C); 
  /** Input full form from stream, the entries have to look like this:
   * \verbatim {"rot-att", "rote", NULL, "ax-pos-e_infl_rule", 0, 1}, ... \endverbatim
   */
  friend std::istream& operator>>(std::istream& I, ff_entry& C); 

private:
  std::string _preterminal;
  std::string _affix;
  
  std::string _form;

  int _inflpos;

  std::string _fname;
  int _line;
};

/** Two full forms are equal if they have the same preterminal? */
inline bool operator==(const ff_entry &a, const ff_entry &b)
{
  return compare(a,b) == 0;
}

/* Extract the stem(s) from a dag (using the  path setting) */
// No implementation available
//vector<std::string> get_le_stems(dag_node *le);

/** Representation of an irregular entry */
class irreg_entry
{
 public:
  /** Constructor.
   * \param fo The surface form
   * \param in The affix rule
   * \param st The base form
   */
  irreg_entry(std::string fo, std::string in, std::string st)
    : _form(fo), _infl(in), _stem(st) {}
  
  /** Dump entry in a binary representation to \a f */
  void dump(dumper *f);

 private:
  std::string _form, _infl, _stem;

};
/*@}*/

/// Grammar properties - these are dumped into the binary representation
extern std::map<std::string, std::string> grammar_properties;

/********************************************************/
/* global functions - the interface between the modules */
/********************************************************/

/** @name util.cc */
/*@{*/
int strcount(char *s, char c);
/** counts occurences of \a c in \a s - \a c must be != 0. */

void indent (FILE *f, int nr);
/** prints \a nr blanks on \a f */

struct type *new_type(const std::string &name, bool is_inst, bool define = true);
/** allocates memory for new type - returns pointer to initialized struct */

/** Register a new builtin type with name \a name */
int new_bi_type(const char *name);

/** A special form of strcat which returns a new string containing 
 *  \a old + \a add.
 *  \a old may be NULL, then this is equal to \code strdup(add) \endcode
 */
char *add_inflr(char *old, char *add);

/** A hash function for strings */
extern int Hash(const std::string &s);
/*@}*/

/** @name full-form.cc */
/*@{*/
/** read full_form entries from file with name \a fname */
void read_morph(std::string fname);
/** read irregular entries from file with name \a fname */
void read_irregs(std::string fname);
/*@}*/

/** from template.cc: expand the template calls in all type definitions. */
void expand_templates();

/** @name corefs.cc */
/*@{*/
/** add coref \a name to the table \a co */
int add_coref(struct coref_table *co, char *name);
/** Start a new coref table domain for type addenda by making the old names
 *  different from any new names by adding the coref index to each name
 *  in the table
 */
void new_coref_domain(struct coref_table *co);
/** `unify' multiple coreferences in conjunctions
    (e.g. [ #1 & #2 ] is converted to [ #1_2 ]). */
void find_corefs();
/*@}*/

/** from print-chic.cc: Print the skeleton of \a t in CHIC format to \a f */
void print_constraint(FILE *f, struct type *t, const std::string &name);

/** @name print-tdl.cc */
/*@{*/
/** Print header comment for preprocessed file.
 * \param outf The file stream of the generated file
 * \param outfname The name of the generated file
 * \param fname The name of the source file
 * \param gram_version The version string of the grammar
 */
void 
write_pre_header(FILE *outf, const char *outfname, const char *fname
                 , const char *gram_version);
/** Write preprocessed source to file \a f */
void write_pre(FILE *f);
/*@}*/

/** @name terms.cc : Create internal representations from TDL expressions. */
/*@{*/
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
/*@}*/

/** from builtins.cc: Initialize the built-in data type \c BI_TOP */
void initialize_builtins();

/** from parse-tdl.cc: start parsing a new TDL statement.
 * \param toplevel if \c false, the statement to parse is embedded in some
 * begin ... end environment.
 */
void tdl_start(int toplevel);


/** @name expand.cc */
/*@{*/

/** For every feature, compute the maximal type that introduces it.
 *
 * \return \c false if a feature gets introduced at two incompatible types,
 * which is an error, \c true if all appropriate types could be computed. The
 * array \c apptype is filled with the maximally appropriate types, i.e., \c
 * apptype[j] is the first subtype of \c *top* that introduces feature \c j.
 */
bool compute_appropriateness();
/** Apply the appropriateness conditions recursively on each dag in the
 *  hierarchy.
 * In every subdag of a dag, unify the type of the subdag with all appropriate
 * types induced by the features of that subdag. If this unification fails, the
 * type definition is inconsistent.
 * \return \c true if all dags are consistent with the appropriateness
 * conditions required by the features, \c false otherwise.
 */
bool apply_appropriateness();

/** Do delta expansion: unify all supertype constraints into a type skeleton.
 *
 * Excluded from this step are all pseudo types (\see pseudo_type). Instances
 * are only expanded if either the option 'expand-all-instances' is active or
 * the instance does not have a status that is mentioned in the 'dont-expand'
 * setting in 'flop.set'.
 * \return \c true, if there were no inconsistent type definitions found, 
 *         \c false otherwise. 
 */
bool delta_expand_types();
/**
 * Expand the feature structure constraints of each type after delta expansion.
 * 
 * The algorithm is as follows: Build a graph of types such that a link
 * from type t to type s exists if the feature structure skeleton of type 
 * s uses type t in some substructure. If this graph is acyclic, expand the
 * feature structure constraints fully in topological order over this graph.
 * Otherwise, there are illegal cyclic type dependencies in the definitions 
 *
 * \param full_expansion if \c true, expand all dags, even those which have
 *                       (currently) no arcs
 * \return true if the definitions are all OK, false otherwise
 */
bool fully_expand_types(bool full_expansion);
bool process_instances();

/** Compute the number of features introduced by each type and the maximal
 * appropriate type per feature.
 *
 * The maximal appropriate type \c t for feature \c f is the type that is
 * supertype of every dag \c f points to. It is defined by the type of the
 * subdag \c f points to where \c f is introduced.
 */
void compute_maxapp();
/** Compute featconfs and featsets for fixed arity encoding. */
void compute_feat_sets(bool minimal);

/** Unfill all type dags in the hierarchy.
 *
 * Remove all features that are not introduced by the root dag and where the
 * subdag they point to
 *
 * - has no arcs (which may be due to recursive unfilling)
 * - is not coreferenced
 * - its type is the maximally appropriate type for the feature
 *
 * The unfilling algorithm deletes all edges that point to nodes with maximally
 * underspecified information, i.e. whose type is the maximally appropriate
 * type under the attribute and whose substructures are also maximally
 * underspecified. This results in nodes that contain some, but not all of the
 * edges that are appropriate for the node's type. There are in fact partially
 * unfilled fs nodes.
 */
void unfill_types();

/**
 * Recursively unify the feature constraints of the type of each dag node into
 * the node
 *
 * \param dag Pointer to the current dag
 * \param full If \c true, expand subdags without arcs too
 * \result A path to the failure point, if one occured, NULL otherwise
 */
list_int *fully_expand(struct dag_node *dag, bool full);
/*@}*/

/** dump.cc: Dump the whole grammar to a binary data file.
 * \param f low-level dumper class
 * \param desc readable description of the current grammar
 */
void dump_grammar(dumper *f, const char *desc);

/** @name dag-tdl.cc */
/*@{*/
/** Build the compact symbol tables that will be dumped later on and are used
 *  by cheap.
 */
void dagify_symtabs();
/** Convert the internal representations of the TDL type definitions into
 *  dags. 
 */
void dagify_types();
/*@}*/

/** flop.cc: Tell the user how much memory was used up to point \a where */
void
mem_checkpoint(const char *where);

#endif
