/* -*- Mode: C++ -*- */
/** \file lexparser.h 
 * Input preprocessing and lexical parsing stage.
 */

#ifndef _LEXPARSER_H
#define _LEXPARSER_H

#include <vector>
#include <list>
#include <queue>
#include <string>
#include "input-modules.h"
#include "morph.h"

/** Call modules in a special chain of responsiblity.
 * The modules are an ordered list with increasing level. All lexica on the
 * same level are consulted in parallel, i.e., all results are merged into
 * the result list. Modules on a higher level (lower priority) are only
 * considered if no module on a lower level delivered a result.
 */
template <typename RET_TYPE, typename MOD_ITER> 
std::list<RET_TYPE>
call_resp_chain (MOD_ITER begin, MOD_ITER end
                 , const myString &str) {
  int level;
  std::list< RET_TYPE > res;
  MOD_ITER mod = begin;
  do {
    level = (*mod)->level();
    std::list<RET_TYPE> tmp = (**mod)(str);
    res.splice(res.end(), tmp);
    mod++;
  } while ((mod != end) && ((level == (*mod)->level()) || res.empty()));
  return res;
}


template < typename IM > 
void free_modules(std::list< IM * > &modules) {
  for(typename std::list< IM * >::iterator it = modules.begin();
      it != modules.end(); it++)
    delete *it;
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
  
  ~lex_parser() {
    free_modules(_tokenizers);
    free_modules(_taggers);
    free_modules(_ne_recogs);
    free_modules(_morphs);
    free_modules(_lexica);
  }
  
  /** Perform lexparser initializations.
   *  - Set carg modification (surface string) path
   */
  void init();

  /** Reset the lex parser's internal state prior to processing of the next
   *  input
   */
  void reset();

  /** Read the next input chunk from \a in into \a result and return \c true if
   *  the end of input/file has been reached, false otherwise.
   *
   *  The functionality is delegated to the registered tokenizer.
   */
  bool next_input(std::istream &in, std::string &result);

  /** Perform tokenization, named entity recognition, POS tagging and skipping
   *  of unknown tokens.
   *  \param input The input string, not necessarily a simple sentence.
   *  \param[in,out] inp_tokens The set of input tokens we get from input processing.
   *  \return The rightmost position of the parsing chart that has to be
   *  created. 
   */
  int process_input(std::string input, inp_list &inp_tokens);

  /** \brief Perform lexical processing. 
   *
   * Do lexicon access and morphology, complete active multi word lexemes, find
   * lexical gaps and add appropriate default lexicon entries (generics), if
   * possible. After that, prune the chart applying the specified chart
   * dependencies. 
   *
   * \param inp_tokens The input tokens coming from process_input
   * \param lex_exhaustive If true, do exhaustive lexical processing before
   *        looking for gaps and chart dependencies. This allows to catch more
   *        complex problems and properties.
   * \param FSAS the allocation state for the whole parse
   * \param errors a list of eventual errors ??? _fix_me_ if i'm sure
   */
  void lexical_processing(inp_list &inp_tokens, bool lex_exhaustive
                          , fs_alloc_state &FSAS, std::list<tError> &errors);

  /** Do a morphological analysis of \a form and return the results.
   *
   * The morphological analyzers are called in the same way as the lexical
   * \see get_lex_entries for an explanation.
   */
  std::list<tMorphAnalysis> morph_analyze(std::string form);
  
  /** Register different input modules in the lexical parser. */
  //@{
  void register_tokenizer(tTokenizer *m) {
    std::list<tTokenizer *> new_list(1, m) ;
    _tokenizers.merge(new_list, less_than_module());
  }
  
  void register_tagger(tPOSTagger *m) {
    std::list<tPOSTagger *> new_list(1, m);
    _taggers.merge(new_list, less_than_module());
  }

  void register_ne_recognizer(tNE_recognizer *m) {
    std::list<tNE_recognizer *> new_list(1, m);
    _ne_recogs.merge(new_list, less_than_module());
  }

  void register_morphology(tMorphology *m) {
    std::list<tMorphology *> new_list(1, m);
    _morphs.merge(new_list, less_than_module());
  }

  void register_lexicon(tLexicon *m) {
    std::list<tLexicon *> new_list(1, m);
    _lexica.merge(new_list, less_than_module());
  }
  //@}

private:
  /** Remove unrecognized tokens and close gaps that result from this
   * deletion or gaps in the input positions.
   *  \param tokens the list of input tokens.
   *  \param position_mapping if \c STANDOFF_COUNTS, the positions of the input
   *                tokens are counts rather than points (\c STANDOFF_POINTS),
   *                i.e., a word with length one has positions (j, j) rather
   *                than (j, j+1).
   *  \return the end position of the rightmost item.
   */
  int map_positions(inp_list &tokens, position_map position_mapping);

  /** The first stage of lexical processing, without chart dependencies and
   * unknown lex entries processing.
   *
   * Do lexicon access and morphology, complete active multi word lexemes
   *
   * \param inp_tokens The input tokens coming from process_input
   * \param lex_exhaustive If true, do exhaustive lexical processing before
   *        looking for gaps and chart dependencies. This allows to catch more
   *        complex problems and properties.
   * \param FSAS the allocation state for the whole parse
   * \param errors a list of eventual errors ??? _fix_me_ if i'm sure
   */
  void lexical_parsing(inp_list &inp_tokens, bool lex_exhaustive, 
                       fs_alloc_state &FSAS, std::list<tError> &errors);

  /** Check the chart dependencies.
   * Chart dependencies allow the user to express certain cooccurrence
   * restrictions for two items in the chart. A chart dependency is a pair of
   * feature-structure paths (say \e A and \e B), and is interpreted as follows:
   * if there is an item in the chart with path \e A, there must also be another
   * item in the chart with path \e B such that both types are compatible;
   * otherwise the item with path \e A will be deleted.
   * \param deps           setting containing a (flat) list of dependent pairs
   *                       of paths (in \a deps.values).
   * \param unidirectional specifies whether the pairs of paths in \a deps are
   *                       interpreted as unidirectional dependencies.
   * \param lex_exhaustive specifies whether we have performed an exhaustive
   *                       lexical processing before.
   */
  void dependency_filter(struct setting *deps, bool unidirectional
                         , bool lex_exhaustive);

  /** Add generic entries for uncovered input items in the traditional
   * scenario (as opposed to the chart mapping scenario), i.e. when
   * \c opt_default_les is \c true:
   * Basically, for each unknown token in the input all generic entries are
   * postulated. Optionally, there are two devices to filter out generic
   * entries: suffix-based and by virtue of POS tag information. Generic
   * entries that require a certain suffix (`generic-le-suffixes') only fire
   * if the input form has the suffix. If the input word has one more more
   * POS tags associated to it, these are looked up in the `posmapping' table:
   * this table is a list of pairs (tag, gle) where `gle' is the name of one
   * of the generic items in `generic-les'. A non-empty `posmapping' table will
   * filter all generic entries that are not explicitly licensed by a POS tag.
   */
  void add_generics(std::list<tInputItem *> &unexpanded);

  /** Add predicted entries for uncovered input items.  
   * This is only applied if the option \c opt_predict_les is \c
   * non-zero and \opt_default_les is false.
   */
  void add_predicts(std::list<tInputItem *> &unexpanded, inp_list &inp_tokens);

  /** Use the registered tokenizer(s) to tokenize the input string and put the
   *  result into \a tokens.
   *
   * At the moment, it seems unlikely that more than one tokenizer should be
   * registered. Nevertheless, the structure to have multiple tokenizers
   */
  void tokenize(std::string input, inp_list &tokens);

  /** Call the registered taggers which add their results to the individual
   *  \a tokens.
   */
  void tag(std::string input, inp_list &tokens);

  /** Call the registered NE recognizers which add their results to \a tokens.
   */
  void ne_recognition(std::string input, inp_list &tokens);

  /** Consider lexica, some parallel and some in a chain of responsibility.
   *
   * The lexica are an ordered list with increasing level. All lexica on the
   * same level are consulted in parallel, i.e., all results are merged into
   * the result list. Lexica on a higher level (lower priority) are only
   * considered if no lexicon on a lower level delivered a result.
   */
  std::list<lex_stem *> get_lex_entries(std::string stem);

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
  void add_surface_mod(const std::string &carg, modlist &mods);

  std::list<tTokenizer *> _tokenizers;
  std::list<tPOSTagger *> _taggers;
  std::list<tNE_recognizer *> _ne_recogs;
  std::list<tMorphology *> _morphs;
  std::list<tLexicon *> _lexica;

  std::queue<class lex_task *> _agenda;

  /** A "chart" of incomplete lexical items (stemming from multi word entries).
   * These have to be deleted after lexical processing, or, alternatively, when
   * new input is processed (preferable).
   */
  std::vector< std::list< tLexItem * > > _active_left;
  std::vector< std::list< tLexItem * > > _active_right;
  
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
  virtual ~lex_task() {}
  
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
  virtual ~lex_and_inp_task() {}

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
