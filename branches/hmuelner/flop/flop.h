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

#include <list>
#include <cstdio>
#include <vector>
#include "types.h"
#include "symtab.h"
#include "lex-io.h"

class settings;
struct Conjunction;
struct Tdl_list;
struct Term;
struct Param_list;
struct Coref_table;
struct Templ;
struct Type;
struct list_int;

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
/*@}*/

/* Global Data: variables & constants */
/** flop.cc: kinds of terms */
enum TERM_TAG {NONE, TYPE, ATOM, STRING, COREF, FEAT_TERM, LIST, DIFF_LIST,
TEMPL_PAR, TEMPL_CALL};

/** @name Symbol Maps
* Global tables mapping strings to data structures
*/
/*@{*/
/** Map name to type */
extern symtab<Type*> types;
/** Map name to type status */
extern symtab<int> statustable;
/** Map name to template */
extern symtab<Templ*> templates;
/** Map name to attribute */
extern symtab<int> attributes;
/*@}*/

extern std::string global_inflrs;

/** \brief If zero, type redefinitions will issue a warning. Will be set from
* lexer
*/
extern int allow_redefinitions;

/** A table containing the parent of a leaf type, if greater than zero.
*  \code leaftypeparent[type] < 0 \endcode is a predicate for testing
*  if type is not a leaf type (after expansion).
*/
extern std::vector<int> leaftypeparent;

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
extern std::vector<type_t> apptype;

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
struct Attr_val 
{
  /** The attribute string */
  std::string attr;
  /** The conjunction this arc points to */
  Conjunction *val;

  Attr_val() : attr(), val(NULL) {}
  Attr_val(const Attr_val& a);
  Attr_val& operator=(const Attr_val& a);
};

/** An attribute value matrix for TDL input */
struct Avm
{
  /** A list of attribute value pairs */
  std::vector<Attr_val*> av;
  /** The number of elements active in the \c Attr_val list */
  size_t n() const {return av.size(); }
  Avm() : av() {}
  Avm(const Avm& a);
  Avm& operator=(const Avm& a);
  void add_attr_val(Attr_val *a)
  {
    av.push_back(a);
  }
};

/** The internal representation of a TDL list definition for TDL input */
struct Tdl_list
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

  /** The members of the list itself */
  std::vector<Conjunction*> list;
  /** The number of elements in this list */
  size_t n() const { return list.size(); }

  /** The conjunction pointed to by the last \c REST arc for dotted pair */
  Conjunction *rest;

  Tdl_list() : difflist(0), openlist(0), dottedpair(0), list(), rest(NULL) {}
  Tdl_list(const Tdl_list& t);
  Tdl_list& operator=(const Tdl_list& t);
  void add_conjunction(Conjunction *c)
  {
    list.push_back(c);
  }};

  /** A TDL feature term for TDL input */
  struct Term
  {
    /** The type of the term */
    enum TERM_TAG tag;

    /** interpretation depens on the term type */
    std::string value;
    /** the type id of the term */
    int type;

    /** coreference index */
    int coidx;

    /** contains the list if \c tag is one of \c LIST or \c DIFF_LIST */
    Tdl_list *L;

    /** contains avm if \c tag is \c FEAT_TERM */
    Avm *A;

    /** for template call (templ_call) */
    Param_list *params;
    Term() : tag(NONE), value(), type(0), coidx(0), L(NULL), A(NULL), params(NULL) {}
    Term(const Term& t) ;
    Term& operator=(const Term& t) ;
  };

  /** Representation for a conjunction of terms for TDL input */
  struct Conjunction
  {
    /** List of terms */
    std::vector<Term*> term;
    /** The number of terms in the \c term list */
    size_t n() const { return term.size(); }
    Conjunction() : term() {}
    Conjunction(const Conjunction& c) : term(c.term) {}
    Conjunction& operator=(const Conjunction& c) { term = c.term; }
  };

  /** Representation of a type definition. for TDL input */
  struct Type
  {
    /** Numeric type id */
    int id;

    /** name as defined in grammar, including capitalization etc */
    std::string printname;

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
    bool implicit;

    /** Location of this type's definition (file name, line number, etc.) */
    Lex_location def;

    /** Table of coreferences for this type definition */
    Coref_table* coref;
    /** Internal representation of this type's TDL definition */
    Conjunction* constraint;

    /** The bitcode representing this type in the hierarchy */
    class bitcode *bcode;
    /** The dag resulting from the type definition */
    struct dag_node *thedag;

    /** \c true if this type is a TDL instance, which means that the type in the
    *  root node of the dag is not the same as the name of the type.
    */
    bool tdl_instance;

    /** The regular inflection rule string associated with this type */
    std::string inflr;

    /** parents specified in definition */
    std::vector<int> parents;

    Type() : id(0), printname(), status(NO_STATUS), defines_status(false),
      status_giver(0), implicit(false), def(), coref(NULL),
      constraint(NULL), bcode(NULL), thedag(NULL), tdl_instance(false),
      inflr(), parents() {}
    Type(const Type&);
    Type& operator=(const Type&);
  };

  /** @name Templates
  * representation / processing of templates for TDL input
  */
  /*@{*/
  /** A parameter name/value pair */
  struct Param
  {
    std::string name;
    Conjunction *value;

    Param() : name(), value(NULL) {};
    Param(const std::string& name) : name(name), value(NULL) {};
    Param(const Param&);
    Param& operator=(const Param&);
  };

  /** A list of template parameters for TDL input */
  struct Param_list
  {
    std::vector<Param*> param;
    size_t n() const { return param.size();}
  };

  /** Internal representation of template definition for TDL input */
  struct Templ
  {
    /** The name of the template */
    std::string name;

    /** The number of times this template has been called */
    int calls;

    /** The list of formal parameters of this template */
    Param_list *params;

    /** Internal representation of the right side of the TDL definition */
    Conjunction *constraint;
    /** Table mapping coreference names in this definition to numbers */
    Coref_table *coref;

    /** Location of this template's definition (file name, line number, etc.) */
    Lex_location loc;

    Templ() : name(), calls(0), params(NULL), constraint(NULL), coref(NULL), loc() {}
    Templ(const Templ&);
    Templ& operator=(const Templ&);
  };

  /** Table mapping coreference names to numbers for TDL input */
  struct Coref_table
  {
    /** List of coreference names */
    std::vector<std::string> coref;
    /** Number of elements in the table */
    size_t n() const { return coref.size();}
    Coref_table() : coref() {}
    Coref_table(const Coref_table& c) : coref(c.coref) {}
    Coref_table& operator=(const Coref_table& c) { coref = c.coref;}
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
    {}

    /** Copy constructor */
    ff_entry(const ff_entry &C)
      : _preterminal(C._preterminal), _affix(C._affix), _form(C._form),
      _inflpos(C._inflpos), _fname(C._fname), _line(C._line)
    {}

    /** Constructor setting only preterminal \a pre (type name of base form) */
    ff_entry(std::string pre)
      : _preterminal(pre)
    {}

    /** Default constructor */
    ff_entry() {}

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
    std::string _form;
    std::string _infl;
    std::string _stem;

  };
  /*@}*/

  /// Grammar properties - these are dumped into the binary representation
  extern std::map<std::string, std::string> grammar_properties;

  /********************************************************/
  /* global functions - the interface between the modules */
  /********************************************************/

  /** prints \a nr blanks on \a f */
  inline void indent (FILE *f, int nr) { fprintf (f, "%*s", nr, ""); }

  /** @name util.cc */
  /*@{*/

  Type *new_type(const std::string &name, bool is_inst, bool define = true);
  /** allocates memory for new type - returns pointer to initialized struct */

  /** Register a new builtin type with name \a name */
  int new_bi_type(const std::string &name);

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
  int add_coref(Coref_table *co, const std::string& name);
  /** Start a new coref table domain for type addenda by making the old names
  *  different from any new names by adding the coref index to each name
  *  in the table
  */
  void new_coref_domain(Coref_table *co);
  /** `unify' multiple coreferences in conjunctions
  (e.g. [ #1 & #2 ] is converted to [ #1_2 ]). */
  void find_corefs();
  /*@}*/

  /** from print-chic.cc: Print the skeleton of \a t in CHIC format to \a f */
  void print_constraint(FILE *f, Type *t, const std::string &name);

  /** @name print-tdl.cc */
  /*@{*/
  /** Print header comment for preprocessed file.
  * \param outf The file stream of the generated file
  * \param outfname The name of the generated file
  * \param fname The name of the source file
  * \param gram_version The version string of the grammar
  */
  void write_pre_header(FILE *outf, const std::string& outfname, const std::string& fname,
    const std::string& gram_version);
  /** Write preprocessed source to file \a f */
  void write_pre(FILE *f);
  /*@}*/

  /** @name terms.cc : Create internal representations from TDL expressions. */
  /*@{*/
  Term *new_type_term(int id);
  Term *new_avm_term();
  Term *add_term(Conjunction *C, Term *T);
  Conjunction *get_feature(Avm *A, const std::string& feat);
  Conjunction *add_feature(Avm *A, const std::string& feat); /// \todo unused function

  int nr_avm_constraints(Conjunction *C); /// \todo unused function

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
  * \param f low-level type_t class
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
