/* -*- Mode: C++ -*- */
/** \file lexparser.h 
 * Input preprocessing and lexical parsing stage.
 */

#ifndef _LEXPARSER_H
#define _LEXPARSER_H

#include <vector>
#include <list>
#include <queue>
#include "input-modules.h"
#include "morph.h"

/** Call modules in a special chain of responsiblity.
 * The modules are an ordered list with increasing level. All lexica on the
 * same level are consulted in parallel, i.e., all results are merged into
 * the result list. Modules on a higher level (lower priority) are only
 * considered if no module on a lower level delivered a result.
 */
template <typename RET_TYPE, typename MOD_ITER> 
list<RET_TYPE>
call_resp_chain (MOD_ITER begin, MOD_ITER end
                 , const myString &str) {
  int level;
  list< RET_TYPE > res;
  MOD_ITER mod = begin;
  do {
    level = (*mod)->level();
    list<RET_TYPE> tmp = (**mod)(str);
    res.splice(res.end(), tmp);
    mod++;
  } while ((mod != end) && ((level == (*mod)->level()) || res.empty()));
  return res;
}


/** This object takes complete control and responsibility of input processing
 *  prior to syntactic processing.
 *
 * At the moment, it seems unlikely that more than one tokenizer, POS tagger or
 * named entity recognizer should be registered. Nevertheless, the structure to
 * have multiple instances of all of these is already there.
 *
 * The morphologies and lexica are organized as responsibility chains, \see
 * call_resp_chain for further explanations.
 */
class lex_parser {
  friend class lex_task;

public:
  lex_parser() : _maxpos(-1), _carg_path(NULL) { }
  
  /** Perform lexparser initializations.
   *  - Set carg modification (surface string) path
   */
  void init();

  /** Reset the lex parser's internal state prior to processing of the next
   *  input
   */
  void reset();

  /** Perform tokenization, named entity recognition, POS tagging and skipping
   *  of unknown tokens.
   *  \par input The input string, not necessarily a simple sentence.
   *  \return The rightmost position of the parsing chart that has to be
   *  created. 
   *  \retval inp_tokens The set of input tokens we get from input processing.
   */
  int process_input(string input, inp_list &inp_tokens);

  /** \brief Perform lexical processing. 
   *
   * Do lexicon access and morphology, complete active multi word lexemes, find
   * lexical gaps and add appropriate default lexicon entries (generics), if
   * possible. After that, prune the chart applying the specified chart
   * dependencies. 
   *
   * \par inp_tokens The input tokens coming from process_input
   * \par lex_exhaustive If true, do exhaustive lexical processing before
   *      looking for gaps and chart dependencies. This allows to catch more
   *      complex problems and properties.
   * \par FSAS the allocation state for the whole parse
   * \par errors a list of eventual errors ??? _fix_me_ if i'm sure
   */
  void lexical_processing(inp_list &inp_tokens, bool lex_exhaustive
                          , fs_alloc_state &FSAS, list<tError> &errors);
  
  /** Register different input modules in the lexical parser. */
  //@{
  void register_tokenizer(tTokenizer *m) {
    list<tTokenizer *> new_list(1, m) ;
    _tokenizers.merge(new_list, less_than_module());
  }
  
  void register_tagger(tPOSTagger *m) {
    list<tPOSTagger *> new_list(1, m);
    _taggers.merge(new_list, less_than_module());
  }

  void register_ne_recognizer(tNE_recognizer *m) {
    list<tNE_recognizer *> new_list(1, m);
    _ne_recogs.merge(new_list, less_than_module());
  }

  void register_morphology(tMorphology *m) {
    list<tMorphology *> new_list(1, m);
    _morphs.merge(new_list, less_than_module());
  }

  void register_lexicon(tLexicon *m) {
    list<tLexicon *> new_list(1, m);
    _lexica.merge(new_list, less_than_module());
  }
  //@}

private:
  /** Remove unrecognized tokens and close gaps that result from this
   *  deletion.
   * \ret the end position of the rightmost item.
   */
  int map_positions(inp_list &tokens, bool positions_are_counts);

  /** Check the chart dependencies.
   * Chart dependencies extract properties (types) from feature structures of
   * items under certain paths and require that other items exist, that 
   * have properties fulfilling these requirements.
   * \param deps depending on the parameter \c unidirectional, these are 
   */
  void dependency_filter(struct setting *deps, bool unidirectional);

  /** Add generic entries for uncovered input items.
   * This is only applied if the option \c opt_default_les is \c true.
   */
  void add_generics(list<tInputItem *> &unexpanded);

  /** Use the registered tokenizer(s) to tokenize the input string and put the
   *  result into \a tokens.
   *
   * At the moment, it seems unlikely that more than one tokenizer should be
   * registered. Nevertheless, the structure to have multiple tokenizers
   */
  void tokenize(string input, inp_list &tokens);

  /** Call the registered taggers which add their results to the individual
   *  \a tokens.
   */
  void tag(string input, inp_list &tokens);

  /** Call the registered NE recognizers which add their results to \a tokens.
   */
  void ne_recognition(string input, inp_list &tokens);

  /** Consider lexica, some parallel and some in a chain of responsibility.
   *
   * The lexica are an ordered list with increasing level. All lexica on the
   * same level are consulted in parallel, i.e., all results are merged into
   * the result list. Lexica on a higher level (lower priority) are only
   * considered if no lexicon on a lower level delivered a result.
   */
  list<lex_stem *> get_lex_entries(string stem);

  /** Do a morphological analysis of \a form and return the results.
   *
   * The morphological analyzers are called in the same way as the lexical
   * \see get_lex_entries for an explanation.
   */
  list<tMorphAnalysis> morph_analyze(string form);
  
  /** Add a new tLexItem to our internal chart if active and to the global
   * chart if passive.
   * If the item is active, create appropriate new tasks
   */
  void add(tLexItem *lex);

  /** Add the input item to the global chart and combine it with the
   * appropriate lexical units, eventually applying morphological processing
   * beforehand.
   *
   * If one of the lexical entries has more than one argument, i.e., if it is a
   * multi-word (MW) entry, the inflected argument must be the first to be
   * filled.
   */
  void add(tInputItem *inp);
  
  /** This function creates new tLexItems based on an input item, its
   *  corresponding lex_stem (lexicon entry) and morphological information.
   * Instantiate the \a stem, add the modifications \a mods and create a
   * new tLexItem with inflectional rules \a infl_rules, if this succeeds.
   */
  void combine(lex_stem *stem, tInputItem *i_item, const list_int *infl_rules
               , const modlist &mods);

  /** Add a fs modification for the surface form to the given item. This is
   *  only done for generic entries and input items whose HPSG class is
   *  pre-determined (which are presumably externally computed named entities).
   */
  void add_surface_mod(const string &carg, modlist &mods);

  list<tTokenizer *> _tokenizers;
  list<tPOSTagger *> _taggers;
  list<tNE_recognizer *> _ne_recogs;
  list<tMorphology *> _morphs;
  list<tLexicon *> _lexica;

  queue<class lex_task *> _agenda;

  /** A "chart" of incomplete lexical items (stemming from multi word entries).
   * These have to be deleted after lexical processing, or, alternatively, when
   * new input is processed (preferable).
   */
  vector< list< tLexItem * > > _active_left;
  vector< list< tLexItem * > > _active_right;
  
  /** The rightmost position in our private chart */
  int _maxpos;

  /** Feature structure modification path for surface (relation) specification
   *  in genenric entries.
   */
  char *_carg_path;
};


/** Virtual base class for tasks in the lexicon parser */
class lex_task {
public:
  /** Execute this task */
  virtual void execute(class lex_parser &parser) = 0;
protected:
  /** This function provides access to private functionality of lex_parser for
   *  all subclasses, namely the adding of new items.
   */
  void add(lex_parser &parser, tLexItem *new_item) {
    parser.add(new_item);
  }
};


/** A task combining a tInputItem and a tLexItem */
class lex_and_inp_task : public lex_task {
public:
  /** Constructor */
  lex_and_inp_task(tLexItem *lex, tInputItem *inp)
    : _lex_item(lex), _inp_item(inp) {};

  /** Combine a tInputItem with an active tLexItem. Create new tasks. */
  virtual void execute(class lex_parser &parser) {
    // the lex item already has its infl position filled (as long as the
    // MW extension infl AND keypos is not done)
    // check if the current orthography matches the one of the input item
    // and that the same combination was not executed before
    if (_lex_item->compatible(_inp_item)) {
      // add the lex item to the chart and create appropriate tasks
      add(parser, new tLexItem(_lex_item, _inp_item));
    }
  }

private:
  tLexItem *_lex_item;
  tInputItem *_inp_item;
};

/** Global lex_parser instance for input lexicon processing */
extern lex_parser &Lexparser;

#endif
